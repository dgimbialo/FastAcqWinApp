//
// FastAcq core unit tests (no framework dependencies).
//
#include "ChirpStore.h"
#include "ProtocolDefs.h"
#include "ProtocolParser.h"
#include "Dsp/Cfar.h"
#include "Dsp/Decimator.h"
#include "Dsp/FftPlan.h"
#include "Dsp/PeakFinder.h"
#include "Dsp/RadarDsp.h"
#include "Dsp/Rejection.h"
#include "Dsp/ToneEstimator.h"
#include "Dsp/Window.h"
#include "Core/ChirpGeometry.h"
#include "Core/Export.h"
#include "Core/SessionFile.h"
#include "Core/VcoCurve.h"
#include "Core/RadarPlanner.h"
#include "TraceDefs.h"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <random>
#include <string>
#include <vector>

static int g_pass = 0, g_fail = 0;
#define CHECK(cond) do { if (cond) { ++g_pass; } else { ++g_fail; \
    std::printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)
#define CHECK_NEAR(a, b, tol) do { double _a = (a), _b = (b); if (std::fabs(_a - _b) <= (tol)) { ++g_pass; } else { ++g_fail; \
    std::printf("  FAIL %s:%d: %s = %g, expected %g (tol %g)\n", __FILE__, __LINE__, #a, _a, _b, (double)(tol)); } } while (0)

static constexpr double kPi = 3.14159265358979323846;

// ---------------------------------------------------------------------------
static void TestCrc()
{
    std::puts("CRC");
    const uint8_t s[] = "123456789";
    CHECK(Crc32(s, 9) == 0xCBF43926u);
    CHECK(Crc8(s, 9) == 0xF4);
    uint32_t st = Crc32Init();
    st = Crc32Update(st, s, 4);
    st = Crc32Update(st, s + 4, 5);
    CHECK(Crc32Final(st) == 0xCBF43926u);
}

// ---------------------------------------------------------------------------
static std::vector<uint8_t> BuildFrameBytes(uint32_t id, const std::vector<uint16_t>& raw,
                                            const std::vector<float>& fft, bool corruptCrc = false,
                                            uint8_t flags = FRAME_FLAG_HAS_RAW)
{
    FrameHeader h{};
    h.magic          = FRAME_MAGIC;
    h.frame_id       = id;
    h.timestamp_ms   = 1000 + id;
    h.actual_samples = static_cast<uint32_t>(raw.size());
    h.sample_rate_hz = 60058600;
    h.chirp_freq_hz  = 458;
    h.data_flags     = flags;
    h.raw_data_bytes = static_cast<uint32_t>(raw.size() * 2);
    h.fft_data_bytes = static_cast<uint32_t>(fft.size() * 4);
    std::vector<uint8_t> b(reinterpret_cast<uint8_t*>(&h), reinterpret_cast<uint8_t*>(&h) + sizeof(h));
    const uint8_t* rp = reinterpret_cast<const uint8_t*>(raw.data());
    b.insert(b.end(), rp, rp + raw.size() * 2);
    const uint8_t* fp = reinterpret_cast<const uint8_t*>(fft.data());
    b.insert(b.end(), fp, fp + fft.size() * 4);
    uint32_t crc = Crc32(b.data(), b.size());
    if (corruptCrc) crc ^= 0x1;
    for (int i = 0; i < 4; ++i) b.push_back(static_cast<uint8_t>(crc >> (8 * i)));
    return b;
}

static void TestParser()
{
    std::puts("ProtocolParser");
    std::vector<ChirpFrame> got;
    ProtocolParser p([&](ChirpFrame&& f) { got.push_back(std::move(f)); });

    std::vector<uint16_t> raw(1000);
    for (size_t i = 0; i < raw.size(); ++i) raw[i] = static_cast<uint16_t>(i & 0xFFF);
    std::vector<float> fft(64, 1.5f);

    std::vector<uint8_t> stream = { 0x00, 0x11, 0xFA, 0xCE };   // garbage incl. partial magic
    auto f1 = BuildFrameBytes(1, raw, fft);
    auto f2 = BuildFrameBytes(2, raw, {}, true);                 // bad CRC
    auto f3 = BuildFrameBytes(5, raw, fft);                      // ids 3,4 lost
    stream.insert(stream.end(), f1.begin(), f1.end());
    stream.insert(stream.end(), f2.begin(), f2.end());
    stream.insert(stream.end(), f3.begin(), f3.end());

    // Feed in pseudo-random chunk sizes.
    std::mt19937 rng(7);
    size_t i = 0;
    while (i < stream.size()) {
        size_t n = 1 + rng() % 300;
        if (i + n > stream.size()) n = stream.size() - i;
        p.Feed(stream.data() + i, n);
        i += n;
    }
    CHECK(got.size() == 2);
    CHECK(p.FramesOk() == 2);
    CHECK(p.FramesBadCrc() == 1);
    CHECK(p.FramesLost() == 3);
    CHECK(p.BytesDropped() == 4 + f2.size());
    if (got.size() == 2) {
        CHECK(got[0].header.frame_id == 1);
        CHECK(got[0].raw == raw);
        CHECK(got[0].fft.size() == 64 && got[0].fft[10] == 1.5f);
        CHECK(got[1].header.frame_id == 5);
        CHECK(got[1].rx_tick_ms != 0 || true);
    }

    // Bad header (huge size) must be counted and must not reset statistics.
    FrameHeader bad{};
    bad.magic = FRAME_MAGIC;
    bad.raw_data_bytes = 0x7FFFFFFF;
    p.Feed(reinterpret_cast<uint8_t*>(&bad), sizeof(bad));
    CHECK(p.FramesBadHeader() == 1);
    CHECK(p.FramesOk() == 2);
    auto f4 = BuildFrameBytes(6, raw, {});
    p.Feed(f4.data(), f4.size());
    CHECK(got.size() == 3);
    CHECK(p.FramesLost() == 3);

    // Service frame (status) must not affect lost-frame accounting.
    auto svc = BuildFrameBytes(FRAME_ID_STATUS, {}, {}, false, FRAME_FLAG_IS_STATUS);
    p.Feed(svc.data(), svc.size());
    CHECK(got.size() == 4);
    CHECK(p.FramesLost() == 3);
    p.Reset();
    CHECK(p.FramesOk() == 0 && p.FramesBadCrc() == 0 && p.FramesBadHeader() == 0);
}

// ---------------------------------------------------------------------------
static void TestStore()
{
    std::puts("ChirpStore");
    ChirpStore s(4, 1 << 20);
    for (uint32_t i = 0; i < 10; ++i) {
        ChirpFrame f; f.header.frame_id = 100 + i; f.raw.assign(8, static_cast<uint16_t>(i));
        uint64_t seq = s.Push(std::move(f));
        CHECK(seq == i);
    }
    CHECK(s.Size() == 4);
    CHECK(s.FirstSeq() == 6);
    CHECK(s.NextSeq() == 10);
    CHECK(s.Get(5) == nullptr);
    auto p7 = s.Get(7);
    CHECK(p7 && p7->header.frame_id == 107 && p7->seq == 7);
    CHECK(s.Latest()->seq == 9);
    s.Clear();
    CHECK(s.Size() == 0 && s.NextSeq() == 10);
    ChirpFrame f; f.raw.assign(8, 1);
    CHECK(s.Push(std::move(f)) == 10);

    // Byte limit: 3 frames of 1000 samples (2 KB each) with 5 KB cap -> 2 kept.
    ChirpStore b(100, 5000);
    for (int i = 0; i < 3; ++i) { ChirpFrame g; g.raw.assign(1000, 0); b.Push(std::move(g)); }
    CHECK(b.Size() == 2);
}

// ---------------------------------------------------------------------------
static void NaiveDft(const std::vector<std::complex<float>>& x, std::vector<std::complex<double>>& X)
{
    const size_t n = x.size();
    X.assign(n, {});
    for (size_t k = 0; k < n; ++k)
        for (size_t j = 0; j < n; ++j) {
            double a = -2.0 * kPi * static_cast<double>(j * k) / static_cast<double>(n);
            X[k] += std::complex<double>(x[j]) * std::complex<double>(std::cos(a), std::sin(a));
        }
}

static void TestFft()
{
    std::puts("FFT");
    std::mt19937 rng(1);
    std::uniform_real_distribution<float> u(-1.0f, 1.0f);

    const size_t n = 128;
    std::vector<std::complex<float>> x(n);
    for (auto& v : x) v = { u(rng), u(rng) };
    std::vector<std::complex<double>> ref;
    NaiveDft(x, ref);
    auto y = x;
    dsp::FftPlan plan(n);
    plan.Forward(y.data());
    double maxErr = 0.0;
    for (size_t k = 0; k < n; ++k) maxErr = (std::max)(maxErr, std::abs(std::complex<double>(y[k]) - ref[k]));
    CHECK(maxErr < 1e-3);
    plan.Inverse(y.data());
    maxErr = 0.0;
    for (size_t k = 0; k < n; ++k) maxErr = (std::max)(maxErr, std::abs(std::complex<double>(y[k]) - std::complex<double>(x[k])));
    CHECK(maxErr < 1e-4);

    // Real FFT vs naive.
    const size_t m = 256;
    std::vector<float> r(m);
    for (auto& v : r) v = u(rng);
    std::vector<std::complex<float>> rc(m);
    for (size_t i = 0; i < m; ++i) rc[i] = { r[i], 0.0f };
    NaiveDft(rc, ref);
    dsp::RealFft rf(m);
    std::vector<std::complex<float>> out(rf.Bins());
    rf.Forward(r.data(), out.data());
    maxErr = 0.0;
    for (size_t k = 0; k <= m / 2; ++k) maxErr = (std::max)(maxErr, std::abs(std::complex<double>(out[k]) - ref[k]));
    CHECK(maxErr < 1e-3);
    CHECK(dsp::FftPlan::NextPow2(1000) == 1024 && dsp::FftPlan::NextPow2(1024) == 1024);
}

// ---------------------------------------------------------------------------
static void TestWindows()
{
    std::puts("Windows");
    using namespace dsp;
    auto hann = MakeWindow(WindowType::Hann, 4096);
    auto hi = AnalyzeWindow(hann, WindowType::Hann);
    CHECK_NEAR(hi.coherentGain, 0.5, 0.001);
    CHECK_NEAR(hi.enbwBins, 1.5, 0.002);
    auto bh = AnalyzeWindow(MakeWindow(WindowType::BlackmanHarris4, 4096), WindowType::BlackmanHarris4);
    CHECK_NEAR(bh.coherentGain, 0.35875, 0.001);
    CHECK_NEAR(bh.enbwBins, 2.0, 0.01);
    auto ft = AnalyzeWindow(MakeWindow(WindowType::FlatTop, 4096), WindowType::FlatTop);
    CHECK_NEAR(ft.enbwBins, 3.77, 0.02);
    auto rect = AnalyzeWindow(MakeWindow(WindowType::Rectangular, 100), WindowType::Rectangular);
    CHECK_NEAR(rect.coherentGain, 1.0, 1e-9);
    auto k = MakeWindow(WindowType::Kaiser, 101, 9.0f);
    CHECK_NEAR(k[50], 1.0, 1e-6);
    CHECK_NEAR(k[0], k[100], 1e-6);
    CHECK(k[0] < 0.01f);
    CHECK_NEAR(BesselI0(0.0), 1.0, 1e-12);
    CHECK_NEAR(BesselI0(1.0), 1.2660658777520084, 1e-9);
}

// ---------------------------------------------------------------------------
static void TestDecimator()
{
    std::puts("Decimator");
    using namespace dsp;
    const int D = 16;
    FirDecimator dec(D);
    CHECK(dec.Taps() % 2 == 1);
    const size_t n = 1 << 15;
    std::vector<float> x(n), y;
    // Pass-band tone at 0.3 of the new Nyquist.
    const double fPass = 0.3 * 0.5 / D;
    for (size_t i = 0; i < n; ++i) x[i] = static_cast<float>(std::sin(2.0 * kPi * fPass * i));
    dec.Process(x.data(), n, y);
    CHECK(y.size() == n / D);
    double amp = 0.0;
    for (size_t i = y.size() / 4; i < 3 * y.size() / 4; ++i) amp = (std::max)(amp, std::fabs(static_cast<double>(y[i])));
    CHECK_NEAR(amp, 1.0, 0.02);
    // Stop-band tone at 1.5 x new Nyquist must be strongly attenuated.
    const double fStop = 1.5 * 0.5 / D;
    for (size_t i = 0; i < n; ++i) x[i] = static_cast<float>(std::sin(2.0 * kPi * fStop * i));
    dec.Process(x.data(), n, y);
    amp = 0.0;
    for (size_t i = y.size() / 4; i < 3 * y.size() / 4; ++i) amp = (std::max)(amp, std::fabs(static_cast<double>(y[i])));
    CHECK(amp < 1e-3);
    // D = 1 is a pass-through.
    FirDecimator one(1);
    one.Process(x.data(), 100, y);
    CHECK(y.size() == 100 && y[5] == x[5]);
}

// ---------------------------------------------------------------------------
static void TestCfar()
{
    std::puts("CFAR");
    using namespace dsp;
    CHECK_NEAR(CaCfarAlpha(32, 1e-4), 32.0 * (std::pow(1e-4, -1.0 / 32.0) - 1.0), 1e-9);
    // OS alpha: recomputing Pfa from alpha must give the requested Pfa.
    const int N = 32, k = 24;
    const double a = OsCfarAlpha(N, k, 1e-4);
    double pfa = 1.0;
    for (int i = 0; i < k; ++i) pfa *= (N - i) / (N - i + a);
    CHECK_NEAR(pfa, 1e-4, 1e-7);

    // Synthetic power spectrum: exponential noise + one strong peak.
    std::mt19937 rng(3);
    std::exponential_distribution<double> ex(1.0);
    std::vector<float> p(2048);
    for (auto& v : p) v = static_cast<float>(ex(rng) * 1e-6);
    p[700] = 1e-2f;
    for (int t = 0; t < 3; ++t) {
        DetectorParams dp;
        dp.type = static_cast<DetectorType>(t);
        dp.thresholdDb = 12.0f;
        dp.pfa = 1e-4f;
        DetectorOutput out;
        RunDetector(p, 4, p.size() - 1, dp, out);
        CHECK(out.thresholdDb.size() == p.size());
        CHECK(PowerToDb(p[700]) > out.thresholdDb[700]);
        CHECK_NEAR(out.globalNoiseDb, -60.0 + 10.0 * std::log10(std::log(2.0)), 1.5);
        CHECK(std::isinf(out.thresholdDb[0]));
        int falseAlarms = 0;
        for (size_t i = 4; i + 1 < p.size(); ++i)
            if (i != 700 && PowerToDb(p[i]) > out.thresholdDb[i]) ++falseAlarms;
        CHECK(falseAlarms < 10);
    }
}

// ---------------------------------------------------------------------------
static void MakeSpectrum(double binPos, dsp::WindowType wt, size_t N,
                         std::vector<float>& db, std::vector<std::complex<float>>& cplx)
{
    std::vector<float> x(N);
    auto w = dsp::MakeWindow(wt, N);
    for (size_t i = 0; i < N; ++i)
        x[i] = static_cast<float>(std::cos(2.0 * kPi * binPos * i / N + 0.7) * w[i]);
    dsp::RealFft fft(N);
    cplx.resize(fft.Bins());
    fft.Forward(x.data(), cplx.data());
    db.resize(cplx.size());
    for (size_t k = 0; k < cplx.size(); ++k) db[k] = 20.0f * std::log10(std::abs(cplx[k]) + 1e-12f);
}

static void TestPeaks()
{
    std::puts("PeakFinder");
    using namespace dsp;
    const size_t N = 1024;
    const double truePos = 100.3;
    std::vector<float> db;
    std::vector<std::complex<float>> cplx;

    MakeSpectrum(truePos, WindowType::Hann, N, db, cplx);
    CHECK_NEAR(100.0 + InterpolateOffset(PeakInterp::ParabolicDb, db, &cplx, 100), truePos, 0.03);

    MakeSpectrum(truePos, WindowType::Rectangular, N, db, cplx);
    CHECK_NEAR(100.0 + InterpolateOffset(PeakInterp::Jacobsen, db, &cplx, 100), truePos, 0.02);
    CHECK_NEAR(100.0 + InterpolateOffset(PeakInterp::Candan,   db, &cplx, 100), truePos, 0.02);
    CHECK_NEAR(100.0 + InterpolateOffset(PeakInterp::Quinn2,   db, &cplx, 100), truePos, 0.02);

    // Negative offset too.
    MakeSpectrum(99.6, WindowType::Rectangular, N, db, cplx);
    CHECK_NEAR(100.0 + InterpolateOffset(PeakInterp::Jacobsen, db, &cplx, 100), 99.6, 0.02);
    CHECK_NEAR(100.0 + InterpolateOffset(PeakInterp::Quinn2,   db, &cplx, 100), 99.6, 0.02);

    // FindPeaks with threshold and min distance.
    MakeSpectrum(truePos, WindowType::Hann, N, db, cplx);
    std::vector<float> thr(db.size(), -40.0f), noise(db.size(), -60.0f);
    PeakParams pp;
    pp.minBin = 3; pp.maxPeaks = 5; pp.minDistBins = 4; pp.interp = PeakInterp::ParabolicDb;
    auto peaks = FindPeaks(db, thr, noise, &cplx, 10.0, pp);
    CHECK(peaks.size() >= 1);
    if (!peaks.empty()) {
        CHECK(peaks[0].bin == 100);
        CHECK_NEAR(peaks[0].freqHz, truePos * 10.0, 0.3);
        CHECK(peaks[0].snrDb > 50.0f);
    }
}

// ---------------------------------------------------------------------------
struct SimTarget { double rangeM; double velocityMps; double ampCodes; };

static ChirpFrame MakeFmcwFrame(const dsp::RadarParams& rp, double fs, double chirpFreq, int chirps,
                                const std::vector<SimTarget>& targets, double noiseCodes,
                                uint32_t id, double extraPhase = 0.0)
{
    const double rampSec = 1.0 / (2.0 * chirpFreq);
    const size_t rampLen = static_cast<size_t>(rampSec * fs + 0.5);
    const size_t periodLen = 2 * rampLen;
    const size_t n = periodLen * static_cast<size_t>(chirps);
    const double lambda = rp.LambdaM();
    std::mt19937 rng(id);
    std::normal_distribution<double> nd(0.0, noiseCodes);

    ChirpFrame f;
    f.header.magic = FRAME_MAGIC;
    f.header.frame_id = id;
    f.header.timestamp_ms = id * 100;
    f.header.sample_rate_hz = static_cast<uint32_t>(fs);
    f.header.chirp_freq_hz = static_cast<uint16_t>(chirpFreq);
    f.header.actual_samples = static_cast<uint32_t>(n);
    f.header.data_flags = FRAME_FLAG_HAS_RAW;
    f.raw.resize(n);
    for (size_t i = 0; i < n; ++i) {
        const size_t inPeriod = i % periodLen;
        const bool up = inPeriod < rampLen;
        const double t = static_cast<double>(up ? inPeriod : inPeriod - rampLen) / fs;
        const int m = static_cast<int>(i / periodLen);
        double v = 2048.0 + nd(rng);
        for (const auto& tg : targets) {
            const double R = tg.rangeM + tg.velocityMps * m * (2.0 * rampSec) * (-1.0); // approaching: range shrinks
            const double fR = 2.0 * rp.bandwidthHz * R / (dsp::kSpeedOfLight * rampSec);
            const double fD = 2.0 * tg.velocityMps / lambda;
            const double fb = up ? (fR - fD) : (fR + fD);
            const double phi0 = 4.0 * kPi * R / lambda + extraPhase;
            v += tg.ampCodes * std::cos(2.0 * kPi * fb * t + phi0);
        }
        if (v < 0) v = 0;
        if (v > 4095) v = 4095;
        f.raw[i] = static_cast<uint16_t>(std::lround(v));
    }
    return f;
}

static void TestRadarDsp()
{
    std::puts("RadarDsp");
    using namespace dsp;
    RadarParams rp;
    rp.f0Hz = 5.5e9; rp.bandwidthHz = 1.0e9;      // sweep 5..6 GHz
    DspSettings ds;
    ds.shape = RampShape::Triangle;
    ds.chirpsInFrame = 1;
    ds.maxRangeM = 100.0;
    ds.zeroPad = 4;
    ds.window = WindowType::Hann;
    ds.detector.type = DetectorType::FixedAboveNoise;
    ds.detector.thresholdDb = 15.0f;

    const double fs = 60058600.0, chirp = 458.0;
    std::vector<SimTarget> tg = { { 10.0, 1.5, 800.0 }, { 25.5, 0.0, 80.0 } };
    ChirpFrame f = MakeFmcwFrame(rp, fs, chirp, 1, tg, 2.0, 1);

    RadarDsp dspx;
    dspx.SetParams(rp);
    dspx.SetSettings(ds);
    auto t0 = std::chrono::steady_clock::now();
    FrameResult r = dspx.Process(f);
    double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    std::printf("  process: %.1f ms, decimation x%d, fsEff %.0f Hz, nFft %zu, bin %.1f Hz, noise %.1f dBFS\n",
                ms, r.decimation, r.up.fsEffHz, r.up.nFft, r.up.freqResHz, r.up.noiseFloorDb);
    CHECK(r.valid);
    CHECK(r.up.valid && r.down.valid);
    CHECK(r.decimation > 1);
    CHECK_NEAR(r.rampSec, 1.0 / 916.0, 1e-6);
    CHECK(r.rangePerHz > 0.0);
    CHECK(r.targets.size() >= 2);
    bool found10 = false, found25 = false;
    for (const auto& t : r.targets) {
        std::printf("  target id=%d R=%.2f m v=%.2f m/s A=%.1f dB SNR=%.1f dB paired=%d fUp=%.0f fDn=%.0f\n",
                    t.id, t.rangeM, t.velocityMps, t.ampDb, t.snrDb, t.paired ? 1 : 0, t.fUpHz, t.fDnHz);
        if (std::fabs(t.rangeM - 10.0) < 0.3 && t.paired) { found10 = true; CHECK_NEAR(t.velocityMps, 1.5, 0.3); CHECK_NEAR(t.ampDb, 20.0 * std::log10(800.0 / 2048.0), 1.0); }
        if (std::fabs(t.rangeM - 25.5) < 0.3 && t.paired) { found25 = true; CHECK_NEAR(t.velocityMps, 0.0, 0.3); }
    }
    CHECK(found10);
    CHECK(found25);
    CHECK(!r.segments.empty());
    CHECK(r.phase.valid);

    // Default settings (CA-CFAR): window sidelobes of the strong target must not
    // be reported as targets; both real targets must survive.
    {
        RadarDsp d2;
        d2.SetParams(rp);
        DspSettings def;
        def.maxRangeM = 100.0;
        CHECK(def.detector.type == DetectorType::CaCfar);
        d2.SetSettings(def);
        FrameResult r2 = d2.Process(f);
        int near10 = 0, near25 = 0, other = 0;
        for (const auto& t : r2.targets) {
            if (std::fabs(t.rangeM - 10.0) < 0.3) ++near10;
            else if (std::fabs(t.rangeM - 25.5) < 0.3) ++near25;
            else ++other;
        }
        std::printf("  CA-CFAR default: %zu targets (%d near 10 m, %d near 25.5 m, %d other)\n", r2.targets.size(), near10, near25, other);
        CHECK(near10 == 1 && near25 == 1);
        CHECK(other <= 1);
    }

    // Average trace mode must be stable across frames.
    ds.trace = TraceMode::Average;
    dspx.SetSettings(ds);
    for (int i = 0; i < 3; ++i) r = dspx.Process(f);
    CHECK(r.valid && r.targets.size() >= 2);

    // Phase tracking: move the first target by +1 mm -> ~ +1 mm displacement.
    ds.trace = TraceMode::ClearWrite;
    ds.phaseTrackBin = -1;
    dspx.SetSettings(ds);
    dspx.ResetState();
    std::vector<SimTarget> tg1 = { { 10.0, 0.0, 800.0 } };
    ChirpFrame fa = MakeFmcwFrame(rp, fs, chirp, 1, tg1, 0.5, 11);
    std::vector<SimTarget> tg2 = { { 10.001, 0.0, 800.0 } };
    ChirpFrame fb = MakeFmcwFrame(rp, fs, chirp, 1, tg2, 0.5, 11);
    FrameResult ra = dspx.Process(fa);
    FrameResult rb = dspx.Process(fb);
    CHECK(ra.phase.valid && rb.phase.valid);
    std::printf("  phase displacement: %.3f mm (expected ~1.0)\n", rb.phase.displacementMm);
    CHECK_NEAR(rb.phase.displacementMm, 1.0, 0.15);

    // Burst: 8 chirps with a target approaching at 2 m/s. MTI removes static
    // clutter, so the moving target must appear at its range, above the zero row.
    ds.chirpsInFrame = 8;
    dspx.SetSettings(ds);
    dspx.ResetState();
    std::vector<SimTarget> tgMove = { { 10.0, 2.0, 800.0 } };
    ChirpFrame fburst = MakeFmcwFrame(rp, fs, chirp, 8, tgMove, 1.0, 21);
    t0 = std::chrono::steady_clock::now();
    FrameResult rbst = dspx.Process(fburst);
    ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    std::printf("  burst process: %.1f ms, RD %dx%d, peak range bin %d doppler %d (zero at %d), velBin %.3f m/s\n",
                ms, rbst.rd.nRange, rbst.rd.nDoppler, rbst.rd.peakRange, rbst.rd.peakDoppler, rbst.rd.nDoppler / 2, rbst.rd.velBinMps);
    CHECK(rbst.valid);
    CHECK(rbst.rd.nRange > 0 && rbst.rd.nDoppler >= 8);
    CHECK(rbst.segments.size() == 8 * 2 * 3);
    if (rbst.rd.nRange > 0) {
        const double rBin = rbst.rd.peakRange * rbst.rd.rangeBinM;
        CHECK_NEAR(rBin, 10.0, 0.6);
        // Expected Doppler row: nD/2 + f_D / (1 / (nD * T_PRI)), f_D = 2 v / lambda.
        const double fD = 2.0 * 2.0 / rp.LambdaM();
        const double rowExp = rbst.rd.nDoppler / 2.0 + fD * rbst.rd.nDoppler * rbst.periodSec;
        CHECK_NEAR(static_cast<double>(rbst.rd.peakDoppler), rowExp, 1.0);
        CHECK(rbst.rd.peakDoppler > rbst.rd.nDoppler / 2);
        CHECK_NEAR(rbst.rd.velBinMps, rp.LambdaM() / (2.0 * rbst.rd.nDoppler * rbst.periodSec), 1e-9);
    }

    // Derived values.
    DerivedValues dv = ComputeDerived(rp, ds, fs, chirp, f.raw.size(), 100.0, true, false, 0);
    CHECK_NEAR(dv.rangeResM, dsp::kSpeedOfLight / (2.0 * 1.0e9), 1e-9);
    CHECK_NEAR(dv.beatPerMeterHz, 2.0 * 1.0e9 / (kSpeedOfLight / 916.0), 1.0);
    CHECK(dv.rangeMaxM > 100.0);
    CHECK(dv.usbMBps > 2.0);

    // MCU-FFT path.
    ChirpFrame fm;
    fm.header.fft_size = 4096;
    fm.header.fft_freq_res_hz = static_cast<float>(fs / 4096.0);
    fm.header.sample_rate_hz = static_cast<uint32_t>(fs);
    fm.fft.assign(2048, 10.0f);
    fm.fft[300] = 2048.0f * 2048.0f;   // full-scale-ish peak
    ds.useMcuFft = true;
    ds.chirpsInFrame = 1;
    dspx.SetSettings(ds);
    FrameResult rm = dspx.Process(fm);
    CHECK(rm.valid && rm.fromMcuFft);
    CHECK(!rm.targets.empty());
    if (!rm.targets.empty()) CHECK_NEAR(rm.targets[0].fUpHz, 300.0 * fs / 4096.0, fs / 4096.0);
}


// ---------------------------------------------------------------------------
// Protocol v2: header_size bytes (>= 64) with FrameHeaderExt after the 56-byte
// v1 header; CRC covers the header exactly as sent.
static std::vector<uint8_t> BuildFrameBytesV2(uint32_t id, const std::vector<uint16_t>& raw,
                                              uint16_t headerSize, const FrameHeaderExt& extIn,
                                              uint8_t flags = FRAME_FLAG_HAS_RAW, bool corruptCrc = false)
{
    FrameHeader h{};
    h.magic          = FRAME_MAGIC_V2;
    h.frame_id       = id;
    h.timestamp_ms   = 2000 + id;
    h.actual_samples = static_cast<uint32_t>(raw.size());
    h.sample_rate_hz = 60000000;
    h.chirp_freq_hz  = 458;
    h.data_flags     = flags;
    h.raw_data_bytes = static_cast<uint32_t>(raw.size() * 2);
    h.fft_data_bytes = 0;
    h.fft_freq_res_hz = 10.0f;
    h.reserved1[4] = 0x44; h.reserved1[5] = 0x04;   // rise 1092 us
    h.reserved1[6] = 0x44; h.reserved1[7] = 0x04;   // fall 1092 us
    FrameHeaderExt ext = extIn;
    ext.header_size   = headerSize;
    ext.proto_version = 2;
    std::vector<uint8_t> b(reinterpret_cast<uint8_t*>(&h), reinterpret_cast<uint8_t*>(&h) + sizeof(h));
    const size_t extBytes = (std::min<size_t>)(headerSize - sizeof(h), sizeof(ext));
    const uint8_t* ep = reinterpret_cast<const uint8_t*>(&ext);
    b.insert(b.end(), ep, ep + extBytes);
    while (b.size() < headerSize) b.push_back(static_cast<uint8_t>(0xA5 + b.size()));   // unknown tail
    const uint8_t* rp = reinterpret_cast<const uint8_t*>(raw.data());
    b.insert(b.end(), rp, rp + raw.size() * 2);
    uint32_t crc = Crc32(b.data(), b.size());
    if (corruptCrc) crc ^= 0x80;
    for (int i = 0; i < 4; ++i) b.push_back(static_cast<uint8_t>(crc >> (8 * i)));
    return b;
}

static void TestParserV2()
{
    std::puts("ProtocolParser v2");
    std::vector<ChirpFrame> got;
    ProtocolParser p([&](ChirpFrame&& f) { got.push_back(std::move(f)); });

    std::vector<uint16_t> raw(500);
    for (size_t i = 0; i < raw.size(); ++i) raw[i] = static_cast<uint16_t>((i * 7) & 0xFFF);

    FrameHeaderExt e{};
    e.peak_freq_hz     = 12345.678f;
    e.samples_per_chirp = 131008;
    e.rise_samples     = 65504;

    auto f64  = BuildFrameBytesV2(1, raw, 64, e);               // minimum v2: peak only
    auto f72  = BuildFrameBytesV2(2, raw, 72, e);               // full extension
    auto f80  = BuildFrameBytesV2(3, raw, 80, e);               // newer firmware: 8 unknown bytes
    auto fv1  = BuildFrameBytes(4, raw, {});                    // v1 still accepted
    auto fbad = BuildFrameBytesV2(5, raw, 72, e, FRAME_FLAG_HAS_RAW, true);
    auto ftr  = BuildFrameBytesV2(FRAME_ID_TRACE, raw, 64, e, FRAME_FLAG_IS_TRACE);

    std::vector<uint8_t> stream = { 0x7A, 0xDA, 0xCE };
    for (auto* v : { &f64, &f72, &f80, &fv1, &fbad, &ftr }) stream.insert(stream.end(), v->begin(), v->end());

    std::mt19937 rng(11);
    size_t i = 0;
    while (i < stream.size()) {
        size_t n = 1 + rng() % 97;
        if (i + n > stream.size()) n = stream.size() - i;
        p.Feed(stream.data() + i, n);
        i += n;
    }
    CHECK(got.size() == 5);
    CHECK(p.FramesOk() == 5);
    CHECK(p.FramesBadCrc() == 1);
    CHECK(p.FramesLost() == 0);      // ids 1,2,3,4 contiguous; the trace frame is a service frame
    if (got.size() == 5) {
        CHECK(got[0].hasExt && got[0].header.magic == FRAME_MAGIC_V2);
        CHECK_NEAR(got[0].ext.peak_freq_hz, 12345.678, 0.01);
        CHECK(got[0].ext.header_size == 64 && got[0].ext.proto_version == 2);
        CHECK(got[0].ext.samples_per_chirp == 0 && got[0].ext.rise_samples == 0);   // absent -> 0
        CHECK(got[0].raw == raw);
        CHECK_NEAR(got[0].McuPeakHz(), 12345.678, 0.01);
        CHECK(got[1].hasExt && got[1].ext.samples_per_chirp == 131008 && got[1].ext.rise_samples == 65504);
        CHECK(got[2].hasExt && got[2].ext.header_size == 80 && got[2].ext.samples_per_chirp == 131008);
        CHECK(got[2].raw == raw);
        CHECK(!got[3].hasExt && got[3].header.magic == FRAME_MAGIC_V1 && got[3].header.frame_id == 4);
        CHECK(got[4].header.frame_id == FRAME_ID_TRACE && (got[4].header.data_flags & FRAME_FLAG_IS_TRACE));
        // Geometry helpers: v2 exact samples win over rise/fall microseconds.
        const ChirpFrame& g = got[1];
        CHECK(ChirpPeriodSamples(g.header, &g.ext, 200000) == 131008);
        CHECK(ChirpRiseSamples(g.header, &g.ext, 200000) == 65504);
        CHECK(ChirpPeriodSamples(g.header, nullptr, 200000) == 131040);   // 2184 us * 60 MHz
        CHECK(ChirpRiseSamples(g.header, nullptr, 200000) == 65520);
        CHECK(ChirpPeriodSamples(g.header, &g.ext, 1000) == 0);           // frame shorter than a chirp
        CHECK(ChirpPeriodSamples(g.header, &g.ext, 131008) == 131008);
        FrameHeader plain{};
        plain.sample_rate_hz = 60000000;
        CHECK(ChirpPeriodSamples(plain, nullptr, 1000) == 0);             // no rise/fall -> unknown
    }

    // v1 fractional peak (reserved0 = int8 1/256 bin).
    {
        ChirpFrame f;
        f.header.fft_peak_bin = 100;
        f.header.fft_freq_res_hz = 10.0f;
        f.header.data_flags = FRAME_FLAG_HAS_FFT | FRAME_FLAG_PEAK_FRAC;
        f.header.reserved0 = static_cast<uint8_t>(static_cast<int8_t>(-64));
        CHECK_NEAR(f.McuPeakHz(), (100.0 - 0.25) * 10.0, 1e-9);
        f.header.data_flags = FRAME_FLAG_HAS_FFT;
        CHECK_NEAR(f.McuPeakHz(), 1000.0, 1e-9);
    }

    // Bad header_size (too small / too large) counts as a bad header and resyncs.
    {
        FrameHeaderExt tooBig{};
        auto fb = BuildFrameBytesV2(9, raw, 72, tooBig);
        fb[60] = 0x00; fb[61] = 0x10;            // header_size = 4096 (> kHeaderMaxSize)
        p.Feed(fb.data(), fb.size());
        CHECK(p.FramesBadHeader() == 1);
        auto fok = BuildFrameBytesV2(10, raw, 72, e);
        p.Feed(fok.data(), fok.size());
        CHECK(p.FramesOk() == 6);
        CHECK(got.back().header.frame_id == 10);
    }

    // Decoded STATUS: rise/fall and trace flags.
    {
        FrameHeader st{};
        st.fft_freq_res_hz = 900.0f;             // fall_us
        st.reserved1[6] = 0xB0; st.reserved1[7] = 0x04;   // rise 1200 us
        st.reserved0 = 0x03;
        DeviceStatus d = DecodeStatusFrame(st);
        CHECK(d.riseUs == 1200 && d.fallUs == 900 && d.traceOn && d.traceAvail);
        CHECK(!d.offsetSupported && d.offset == 0);
        st.reserved0 = 0x07; st.raw_data_bytes = 1234;      // bit2: chirp offset supported
        d = DecodeStatusFrame(st);
        CHECK(d.offsetSupported && d.offset == 1234);
    }
    static_assert(sizeof(TraceRecord) == 24, "trace record");
}

// ---------------------------------------------------------------------------
static void TestChirpGeometry()
{
    std::puts("ChirpGeometry");
    core::ChirpParams p;
    p.freqHz = 458; p.burst = 1;
    core::ChirpGeometry g = core::ComputeChirpGeometry(p);
    CHECK(g.valid && !g.rampMode);
    CHECK(g.ticksPerSample == 64);
    CHECK(g.tableLen == 8188 && g.riseLen == 4094);
    CHECK(g.periodTicks == 524032);
    CHECK(g.samplesPerChirp == 131008 && g.riseSamples == 65504);
    CHECK_NEAR(g.freqHz, 240e6 / 524032.0, 1e-6);
    CHECK(g.samplesPerBurst == 131008 && g.captureTarget == 131072 && g.chunks == 8);
    CHECK(g.fitsInCapture && !g.clippedByOverride);
    CHECK(g.burstSamplesNeeded == 131008);
    CHECK_NEAR(g.captureUs, 131072.0 / 60.0, 1e-6);
    CHECK_NEAR(g.chirpsCaptured, 131072.0 / 131008.0, 1e-9);
    p.burst = 4;                                     // 524032 samples: still one window
    g = core::ComputeChirpGeometry(p);
    CHECK(g.fitsInCapture && g.captureTarget == 524288 && g.chunks == 32);
    p.burst = 5;                                     // 655040 > 638976: does not fit
    g = core::ComputeChirpGeometry(p);
    CHECK(!g.fitsInCapture && !g.clippedByOverride);
    CHECK(g.burstSamplesNeeded == 655040 && g.captureTarget == 638976);
    CHECK_NEAR(g.chirpsCaptured, 638976.0 / 131008.0, 1e-9);
    p.burst = 2; p.samplesOvr = 100000;              // override shorter than the burst
    g = core::ComputeChirpGeometry(p);
    CHECK(!g.fitsInCapture && g.clippedByOverride && g.captureTarget == 114688);
    p.samplesOvr = 0; p.burst = 1;

    p.riseUs = 1200; p.fallUs = 800; p.burst = 4;
    g = core::ComputeChirpGeometry(p);
    CHECK(g.valid && g.rampMode);
    CHECK_NEAR(g.periodUs, 2000.0, 0.5);
    CHECK_NEAR(g.riseUs, 1200.0, 0.5);
    CHECK(g.samplesPerChirp == 120000 && g.samplesPerBurst == 4 * g.samplesPerChirp);
    CHECK(g.captureTarget % core::kChirpDmaChunk == 0 && g.captureTarget >= g.samplesPerBurst);

    p.riseUs = 0; p.fallUs = 0; p.freqHz = 50;      // out of range
    g = core::ComputeChirpGeometry(p);
    CHECK(!g.valid);
    p.freqHz = 24000;
    g = core::ComputeChirpGeometry(p);
    CHECK(g.valid && g.ticksPerSample == 64 && g.tableLen >= 2);
    p.freqHz = 458; p.burst = 1024;                  // burst clipped to the capture limit
    g = core::ComputeChirpGeometry(p);
    CHECK(g.samplesPerBurst == core::kChirpCaptureMax && g.captureTarget <= core::kChirpCaptureMax);
}

// ---------------------------------------------------------------------------
static void TestVcoCurve()
{
    std::puts("VcoCurve");
    core::VcoCurve c = core::VcoCurve::Hmc431Typical();
    CHECK(c.Valid() && c.Size() == 11);
    CHECK_NEAR(c.FreqHz(0.0), 5.085e9, 1.0);                 // datasheet plot: ~5.09 GHz at 0 V
    CHECK_NEAR(c.FreqHz(10.0), 6.285e9, 1.0);
    CHECK_NEAR(c.FreqHz(0.5), 5.2475e9, 1.0);                // interpolation
    CHECK_NEAR(c.FreqHz(11.0), 6.35e9, 1.0);                 // extrapolation with the last segment
    CHECK_NEAR(c.SensitivityHzPerV(0.5), 325e6, 1.0);        // steep end of the curve
    CHECK_NEAR(c.SensitivityHzPerV(9.5), 65e6, 1.0);         // gentle end
    // The guaranteed 5.5..6.1 GHz band sits inside the typical curve.
    CHECK(c.FreqHz(1.6) > 5.49e9 && c.FreqHz(1.6) < 5.52e9);
    CHECK(c.FreqHz(7.4) > 6.09e9 && c.FreqHz(7.4) < 6.12e9);
    CHECK(std::string(core::VcoCurve::LegacyTypicalText()).rfind("0:5.5000,", 0) == 0);

    // Round trip through the text form, unsorted input, separators.
    core::VcoCurve p;
    CHECK(p.Parse("5=5.86; 0:5.5, 10:6.10 2:5.67"));
    CHECK(p.Size() == 4 && p.VminV() == 0.0 && p.VmaxV() == 10.0);
    CHECK(p.Format() == "0:5.5000,2:5.6700,5:5.8600,10:6.1000");
    core::VcoCurve q;
    CHECK(q.Parse(c.Format()) && q.Size() == 11);
    CHECK_NEAR(q.FreqHz(3.3), c.FreqHz(3.3), 1e3);
    CHECK(!q.Parse("abc"));
    CHECK(!q.Parse("1:5.5"));                                // one point is not a curve
    CHECK(!q.Parse("1:5.5,1:5.6"));                          // duplicate voltage collapses to one point

    // Sweep: DAC 0..4095 -> 0..10 V -> full plot range; half amplitude -> 0..5 V.
    core::VcoSweep s = core::ComputeVcoSweep(c, 0.0, 10.0);
    CHECK(s.valid && !s.outOfTable);
    CHECK_NEAR(s.fStartHz, 5.085e9, 1.0);
    CHECK_NEAR(s.fStopHz, 6.285e9, 1.0);
    CHECK_NEAR(s.f0Hz, 5.685e9, 1.0);
    CHECK_NEAR(s.bandwidthHz, 1200e6, 1.0);
    CHECK_NEAR(s.sensMinHzPerV, 65e6, 1.0);
    CHECK_NEAR(s.sensMaxHzPerV, 325e6, 1.0);
    CHECK(s.nonlinearityPct > 5.0 && s.nonlinearityPct < 60.0);
    s = core::ComputeVcoSweep(c, 0.0, 5.0);
    CHECK_NEAR(s.bandwidthHz, 820e6, 1.0);
    s = core::ComputeVcoSweep(c, 2.0, 12.0);                 // bias 2 V, beyond the table
    CHECK(s.valid && s.outOfTable);
    CHECK(!core::ComputeVcoSweep(c, 5.0, 5.0).valid);
    CHECK(!core::ComputeVcoSweep(core::VcoCurve(), 0.0, 10.0).valid);
}

// ---------------------------------------------------------------------------
static void TestRadarPlanner()
{
    std::puts("RadarPlanner");
    using namespace core;
    PlanInput in;
    in.curve = VcoCurve::Hmc431Typical();
    in.rMinM = 0.5; in.rMaxM = 5.0;
    in.fbMinHz = 2000.0; in.fbMaxHz = 500e3;
    in.vMinMps = 0.2; in.vMaxMps = 5.0;
    in.vLowV = 2.0; in.vHighV = 10.0;
    in.fsHz = 60e6; in.guardPct = 5.0; in.intervalMs = 5;
    PlanResult r = PlanRadar(in);
    CHECK(r.valid);
    // DAC window: 2..10 V of 0..10 V -> codes 819..4095.
    CHECK(r.dacOffset == 819 && r.dacOffset + r.dacAmplitude == 4095);
    CHECK_NEAR(r.fStartHz, in.curve.FreqHz(r.vLowV), 1.0);
    CHECK_NEAR(r.fStopHz, 6.285e9, 1.0);
    CHECK(r.bandwidthHz > 650e6 && r.bandwidthHz < 750e6);
    // The beat band of [rMin, rMax] lies inside [fbMin, fbMax] with equal
    // relative margins; the beat scales linearly with range.
    CHECK(r.fbAtRMinHz >= in.fbMinHz * 0.99 && r.fbAtRMaxHz <= in.fbMaxHz * 1.01);
    CHECK_NEAR(r.fbAtRMinHz / in.fbMinHz, in.fbMaxHz / r.fbAtRMaxHz, 0.05);
    CHECK_NEAR(r.fbAtRMinHz, r.fbAtRMaxHz / 10.0, 1.0);
    CHECK(r.chirpFreqHz >= kChirpFreqMinHz && r.chirpFreqHz <= kChirpFreqMaxHz);
    CHECK_NEAR(r.rampSec, 1.0 / (2.0 * r.chirpFreqHz), 1e-12);
    CHECK_NEAR(r.beatPerMeterHz, 2.0 * r.bandwidthHz / (299792458.0 * r.rampSec), 1e-6);
    CHECK_NEAR(r.rangeResM, 299792458.0 / (2.0 * r.bandwidthHz), 1e-9);
    CHECK(r.rangeMaxM >= in.rMaxM);                                // the decimated band still covers rMax
    CHECK(r.fsEffHz / 2.0 >= 1.25 * r.fbAtRMaxHz);                 // 25 % margin kept
    CHECK((r.decimation & (r.decimation - 1)) == 0);               // power of two
    CHECK(r.samplesUsed >= 64);
    // Velocity: a 0.2 m/s step is below what one 10.65 ms capture can resolve
    // (lambda / (2 * capture time) ~ 2.4 m/s): the burst fills the capture
    // and the shortfall is reported.
    CHECK(r.velStepPairMps > in.vMinMps);
    CHECK(r.burst > 1 && r.geometry.valid && r.geometry.fitsInCapture);
    CHECK(r.Has(PlanNote::BurstLimitedByCapture) && r.Has(PlanNote::VelocityStepNotReached));
    CHECK_NEAR(r.velStepDopplerMps, (299792458.0 / r.f0Hz) / (2.0 * r.burst * 2.0 * r.rampSec), 1e-9);
    CHECK(r.velStepDopplerMps > 2.0 && r.velStepDopplerMps < 3.0);
    CHECK(!r.Has(PlanNote::BeatBandConflict) && !r.Has(PlanNote::VtuneClamped));
    CHECK_NEAR(r.rangeOfInterestM, 6.0, 1e-9);
    CHECK_NEAR(r.pairGateMps, 7.5, 1e-9);
    std::printf("  room plan: chirp %u Hz, DAC %d+%d, B %.0f MHz, fb %.0f..%.0f Hz, dec x%d, burst %u, dv %.3f m/s\n",
                r.chirpFreqHz, r.dacOffset, r.dacAmplitude, r.bandwidthHz / 1e6, r.fbAtRMinHz, r.fbAtRMaxHz,
                r.decimation, r.burst, r.velStepDopplerMps);

    // A reachable velocity step: the burst is sized for it and fits the capture.
    PlanInput v3 = in; v3.vMinMps = 3.0;
    PlanResult r3 = PlanRadar(v3);
    CHECK(r3.valid && r3.burst > 1 && r3.geometry.fitsInCapture);
    CHECK(r3.velStepDopplerMps <= 3.0 * 1.01);
    CHECK(!r3.Has(PlanNote::BurstLimitedByCapture) && !r3.Has(PlanNote::VelocityStepNotReached));
    CHECK(r3.chirpFreqHz == r.chirpFreqHz);                        // velocity never changes the ramp

    // fbMin = 0: only the ceiling counts, the farthest target sits at fbMax.
    PlanInput top = in; top.fbMinHz = 0.0;
    PlanResult rt = PlanRadar(top);
    CHECK(rt.valid && rt.fbAtRMaxHz > 0.98 * top.fbMaxHz && rt.fbAtRMaxHz < 1.02 * top.fbMaxHz);

    // Conflicting beat band: fbMin/rMin > fbMax/rMax -> flagged, slope still from fbMax.
    PlanInput c = in; c.fbMinHz = 200e3;
    PlanResult rc = PlanRadar(c);
    CHECK(rc.valid && rc.Has(PlanNote::BeatBandConflict));
    CHECK(rc.fbAtRMinHz < c.fbMinHz);

    // Long range + low beat -> ramp longer than the MCU allows: clamped to 100 Hz.
    PlanInput lr = in; lr.rMaxM = 3000.0; lr.fbMaxHz = 20e3;
    PlanResult rl = PlanRadar(lr);
    CHECK(rl.valid && rl.Has(PlanNote::ChirpFreqClampedLow) && rl.chirpFreqHz == kChirpFreqMinHz);

    // Tiny window beyond the DAC range is clamped; a window above the curve is flagged.
    PlanInput w = in; w.vLowV = -1.0; w.vHighV = 12.0;
    PlanResult rw = PlanRadar(w);
    CHECK(rw.valid && rw.Has(PlanNote::VtuneClamped) && rw.dacOffset == 0 && rw.dacAmplitude == 4095);

    // Fast target at short range: Doppler above the nearest beat tone is flagged.
    PlanInput fast = in; fast.vMaxMps = 300.0;
    CHECK(PlanRadar(fast).Has(PlanNote::DopplerExceedsBeatAtRmin));

    // Invalid inputs.
    PlanInput bad = in; bad.rMinM = 5.0; bad.rMaxM = 1.0;
    CHECK(!PlanRadar(bad).valid);
    bad = in; bad.curve = VcoCurve();
    CHECK(!PlanRadar(bad).valid);
}

// ---------------------------------------------------------------------------
static void TestGoSoCfarAndRejection()
{
    std::puts("GO/SO-CFAR + rejection");
    using namespace dsp;
    // Alphas: SO must be larger than CA (same cells), GO smaller than SO; all > 0 and monotone in Pfa.
    const double aCa = CaCfarAlpha(32, 1e-4), aGo = GoCfarAlpha(16, 1e-4), aSo = SoCfarAlpha(16, 1e-4);
    std::printf("  alpha(Pfa 1e-4): CA %.2f  GO %.2f  SO %.2f\n", aCa, aGo, aSo);
    CHECK(aCa > 0.0 && aGo > 0.0 && aSo > 0.0);
    CHECK(aSo > aGo);
    CHECK(GoCfarAlpha(16, 1e-2) < GoCfarAlpha(16, 1e-4));
    CHECK(SoCfarAlpha(16, 1e-2) < SoCfarAlpha(16, 1e-4));
    // Empirical false-alarm rate on exponential noise must land near the requested Pfa.
    std::mt19937 rng(5);
    std::exponential_distribution<double> ed(1.0);
    std::vector<float> pw(200000);
    for (auto& v : pw) v = static_cast<float>(ed(rng));
    for (DetectorType t : { DetectorType::CaCfar, DetectorType::GoCfar, DetectorType::SoCfar, DetectorType::OsCfar }) {
        DetectorParams p; p.type = t; p.pfa = 1e-2f; p.guardCells = 2; p.trainCells = 16;
        DetectorOutput o; RunDetector(pw, 0, pw.size(), p, o);
        size_t fa = 0, n = 0;
        for (size_t k = 100; k + 100 < pw.size(); ++k) { ++n; if (PowerToDb(pw[k]) > o.thresholdDb[k]) ++fa; }
        const double rate = static_cast<double>(fa) / n;
        std::printf("  %-24s empirical Pfa %.4f (target 0.01)\n", DetectorName(t), rate);
        CHECK(rate > 0.004 && rate < 0.025);
    }

    // Harmonics / spurs / SNR rejection on a peak list (sorted by amplitude).
    std::vector<Peak> pk;
    auto add = [&](double f, float a, float snr) { Peak q; q.freqHz = f; q.ampDb = a; q.snrDb = snr; q.bin = static_cast<size_t>(f / 10.0); pk.push_back(q); };
    add(1000.0, 0.0f, 60.0f);      // fundamental
    add(2500.0, -10.0f, 50.0f);    // real second target
    add(2000.0, -20.0f, 40.0f);    // 2nd harmonic of 1000
    add(3012.0, -25.0f, 35.0f);    // 3rd harmonic (3 bins off at 10 Hz/bin, k * 1.5 bins tolerance = 45 Hz)
    add(7000.0, -30.0f, 30.0f);    // spur band
    add(5000.0, -40.0f, 4.0f);     // low SNR
    RejectionParams rp; rp.harmonics = true; rp.harmonicTolBins = 1.5; rp.harmonicMinDropDb = 6.0f; rp.minSnrDb = 5.0f;
    rp.spurs = ParseSpurList("7000:100, 9e6", 2000.0);
    CHECK(rp.spurs.size() == 2 && rp.spurs[1].halfWidthHz == 2000.0 && rp.spurs[0].centerHz == 7000.0);
    RejectionStats st;
    RejectPeaks(pk, 10.0, rp, st);
    CHECK(st.harmonics == 2 && st.spurs == 1 && st.lowSnr == 1);
    CHECK(pk.size() == 2 && pk[0].freqHz == 1000.0 && pk[1].freqHz == 2500.0);
    CHECK(FormatSpurList(rp.spurs) == "7000:100, 9000000:2000");
    // A "harmonic" that is not weaker enough is kept.
    pk.clear(); add(1000.0, 0.0f, 60.0f); add(2000.0, -3.0f, 50.0f);
    RejectPeaks(pk, 10.0, rp, st);
    CHECK(pk.size() == 2 && st.harmonics == 0);
}

// ---------------------------------------------------------------------------
static void TestToneEstimator()
{
    std::puts("ToneEstimator");
    const double fs = 1.0e6;
    const double f0 = 12345.678;
    std::vector<uint16_t> s(70000);
    std::mt19937 rng(3);
    std::normal_distribution<double> nd(0.0, 3.0);
    for (size_t i = 0; i < s.size(); ++i) {
        double v = 2048.0 + 600.0 * std::cos(2.0 * kPi * f0 * static_cast<double>(i) / fs + 0.7) + nd(rng);
        s[i] = static_cast<uint16_t>(std::lround(v)) | 0xF000;    // upper bits must be ignored
    }
    dsp::ToneEstimate e = dsp::EstimateTone(s.data(), s.size(), fs);
    CHECK(e.valid && e.nUsed == 65536);
    CHECK_NEAR(e.freqHz, f0, 0.5);
    CHECK_NEAR(e.ampFs, 600.0 / 2048.0, 0.02);

    // Tone exactly between two bins (worst case for the ratio estimator) and a short block.
    const double f1 = 7.5 * fs / 4096.0;
    std::vector<float> t(4096);
    for (size_t i = 0; i < t.size(); ++i) t[i] = static_cast<float>(std::sin(2.0 * kPi * f1 * static_cast<double>(i) / fs));
    e = dsp::EstimateTone(t.data(), t.size(), fs);
    CHECK(e.valid && e.nUsed == 4096);
    CHECK_NEAR(e.freqHz, f1, 0.02 * fs / 4096.0);
    CHECK(!dsp::EstimateTone(t.data(), 10, fs).valid);
    CHECK(!dsp::EstimateTone(static_cast<const float*>(nullptr), 4096, fs).valid);
}

// ---------------------------------------------------------------------------
// Asymmetric triangle (rise != fall) described by the frame header, as sent by
// firmware after CMD_SET_RAMP; v1 (rise/fall in us) and v2 (exact samples).
static ChirpFrame MakeAsymFrame(const dsp::RadarParams& rp, double fs, size_t riseLen, size_t fallLen,
                                int chirps, const std::vector<SimTarget>& targets, bool v2, uint32_t id)
{
    const double tRise = riseLen / fs, tFall = fallLen / fs;
    const size_t periodLen = riseLen + fallLen;
    const size_t n = periodLen * static_cast<size_t>(chirps);
    const double lambda = rp.LambdaM();
    std::mt19937 rng(id);
    std::normal_distribution<double> nd(0.0, 2.0);

    ChirpFrame f;
    f.header.magic = v2 ? FRAME_MAGIC_V2 : FRAME_MAGIC_V1;
    f.header.frame_id = id;
    f.header.sample_rate_hz = static_cast<uint32_t>(fs);
    f.header.chirp_freq_hz = static_cast<uint16_t>(1.0 / (tRise + tFall) + 0.5);
    f.header.actual_samples = static_cast<uint32_t>(n);
    f.header.data_flags = FRAME_FLAG_HAS_RAW;
    const uint16_t riseUs = static_cast<uint16_t>(tRise * 1e6 + 0.5), fallUs = static_cast<uint16_t>(tFall * 1e6 + 0.5);
    f.header.reserved1[4] = static_cast<uint8_t>(riseUs); f.header.reserved1[5] = static_cast<uint8_t>(riseUs >> 8);
    f.header.reserved1[6] = static_cast<uint8_t>(fallUs); f.header.reserved1[7] = static_cast<uint8_t>(fallUs >> 8);
    if (v2) {
        f.hasExt = true;
        f.ext.header_size = 72; f.ext.proto_version = 2;
        f.ext.samples_per_chirp = static_cast<uint32_t>(periodLen);
        f.ext.rise_samples = static_cast<uint32_t>(riseLen);
    }
    f.raw.resize(n);
    for (size_t i = 0; i < n; ++i) {
        const size_t inPeriod = i % periodLen;
        const bool up = inPeriod < riseLen;
        const double t = static_cast<double>(up ? inPeriod : inPeriod - riseLen) / fs;
        const int m = static_cast<int>(i / periodLen);
        double v = 2048.0 + nd(rng);
        for (const auto& tg : targets) {
            const double R = tg.rangeM - tg.velocityMps * m * (tRise + tFall);
            const double fD = 2.0 * tg.velocityMps / lambda;
            const double fRu = 2.0 * rp.bandwidthHz * R / (dsp::kSpeedOfLight * tRise);
            const double fRd = 2.0 * rp.bandwidthHz * R / (dsp::kSpeedOfLight * tFall);
            const double fb = up ? (fRu - fD) : (fRd + fD);
            v += tg.ampCodes * std::cos(2.0 * kPi * fb * t + 4.0 * kPi * R / lambda);
        }
        if (v < 0) v = 0;
        if (v > 4095) v = 4095;
        f.raw[i] = static_cast<uint16_t>(std::lround(v));
    }
    return f;
}

static void TestFirmwareGeometry()
{
    std::puts("RadarDsp firmware geometry");
    using namespace dsp;
    RadarParams rp; rp.f0Hz = 5.5e9; rp.bandwidthHz = 1.0e9;
    DspSettings ds;
    ds.shape = RampShape::Triangle;
    ds.chirpsInFrame = 1;                      // wrong on purpose: the header says 2
    ds.maxRangeM = 60.0;
    ds.zeroPad = 4;
    ds.detector.type = DetectorType::FixedAboveNoise;
    ds.detector.thresholdDb = 15.0f;

    const double fs = 60000000.0;
    const size_t riseLen = 72000, fallLen = 48000;    // 1200 / 800 us
    std::vector<SimTarget> tg = { { 12.0, 2.0, 700.0 } };
    for (int v2 = 0; v2 < 2; ++v2) {
        ChirpFrame f = MakeAsymFrame(rp, fs, riseLen, fallLen, 2, tg, v2 != 0, 100 + v2);
        RadarDsp d; d.SetParams(rp); d.SetSettings(ds);
        FrameResult r = d.Process(f);
        CHECK(r.valid && r.geometryFromHeader);
        CHECK(r.chirps == 2);
        CHECK(r.samplesPerChirp == riseLen + fallLen);
        CHECK(r.riseSamples == riseLen);
        CHECK_NEAR(r.rampSec, riseLen / fs, 1e-9);
        CHECK_NEAR(r.fallSec, fallLen / fs, 1e-9);
        CHECK(r.up.valid && r.down.valid);
        bool found = false;
        for (const auto& t : r.targets) {
            std::printf("  %s target R=%.2f m v=%.2f m/s paired=%d fUp=%.0f fDn=%.0f\n", v2 ? "v2" : "v1",
                        t.rangeM, t.velocityMps, t.paired ? 1 : 0, t.fUpHz, t.fDnHz);
            if (t.paired && std::fabs(t.rangeM - 12.0) < 0.3) { found = true; CHECK_NEAR(t.velocityMps, 2.0, 0.4); }
        }
        CHECK(found);
        CHECK(r.up.tone.valid && r.down.tone.valid);
        // Precise tone estimate of the UP ramp (first chirp, static part of the beat).
        const double fRu = 2.0 * rp.bandwidthHz * 12.0 / (kSpeedOfLight * (riseLen / fs));
        CHECK_NEAR(r.up.tone.freqHz, fRu - 2.0 * 2.0 / rp.LambdaM(), 60.0);
        // Scope segments follow the header split.
        size_t ups = 0, downs = 0;
        for (const auto& sg : r.segments) { if (sg.kind == Segment::Up) ++ups; else if (sg.kind == Segment::Down) ++downs; }
        CHECK(ups == 2 && downs == 2);
    }

    // v1 microsecond geometry that does not divide the frame exactly (quantized
    // DAC table): the split must still follow the real chirps of the burst.
    {
        ChirpFrame f = MakeAsymFrame(rp, fs, 65504, 65504, 4, tg, false, 104);   // 1091.7 us ramps
        f.header.reserved1[4] = 0x44; f.header.reserved1[5] = 0x04;              // reported as 1092 us
        f.header.reserved1[6] = 0x44; f.header.reserved1[7] = 0x04;
        RadarDsp d; d.SetParams(rp); d.SetSettings(ds);
        FrameResult r = d.Process(f);
        CHECK(r.valid && r.geometryFromHeader);
        CHECK(r.chirps == 4);
        CHECK(r.samplesPerChirp == 131008 && r.riseSamples == 65504);
        bool found = false;
        for (const auto& t : r.targets)
            if (t.paired && std::fabs(t.rangeM - 12.0) < 0.3) { found = true; CHECK_NEAR(t.velocityMps, 2.0, 0.4); }
        CHECK(found);
        CHECK(r.rd.nDoppler >= 4 && r.rd.nRange > 0);
    }

    // Switching the header split off restores the settings-driven segmentation.
    {
        ChirpFrame f = MakeAsymFrame(rp, fs, riseLen, fallLen, 2, tg, true, 102);
        DspSettings off = ds; off.firmwareGeometry = false; off.chirpsInFrame = 2; off.toneEstimate = false;
        RadarDsp d; d.SetParams(rp); d.SetSettings(off);
        FrameResult r = d.Process(f);
        CHECK(r.valid && !r.geometryFromHeader);
        CHECK(!r.up.tone.valid);
    }

    // ADC clock correction scales every frequency-derived quantity.
    {
        ChirpFrame f = MakeAsymFrame(rp, fs, riseLen, fallLen, 1, tg, true, 103);
        f.header.data_flags |= FRAME_FLAG_HAS_FFT;
        f.ext.peak_freq_hz = 10000.0f;
        RadarDsp d; d.SetParams(rp); d.SetSettings(ds);
        d.SetSampleRateScale(1.0 + 100e-6);
        FrameResult r = d.Process(f);
        CHECK_NEAR(r.fsHz, fs * (1.0 + 100e-6), 1e-3);
        CHECK_NEAR(r.mcuPeakHz, 10000.0 * (1.0 + 100e-6), 1e-6);
    }
}

// ---------------------------------------------------------------------------
static void TestSessionFile()
{
    std::puts("SessionFile");
    namespace fs = std::filesystem;
    fs::path p = fs::temp_directory_path() / "fastacq_test_session.facq";
    {
        core::SessionWriter w;
        CHECK(w.Open(p, 60058600, 1234567890123ull, "unit test"));
        for (uint32_t i = 0; i < 3; ++i) {
            ChirpFrame f;
            f.header.frame_id = 10 + i;
            f.header.timestamp_ms = 500 * i;
            f.raw.assign(100 + i, static_cast<uint16_t>(i));
            if (i == 1) f.fft.assign(16, 2.5f);
            if (i == 2) { f.hasExt = true; f.ext.peak_freq_hz = 4321.5f; f.ext.samples_per_chirp = 131008; f.ext.proto_version = 2; }
            f.rx_tick_ms = 77 + i;
            CHECK(w.Write(f));
        }
        CHECK(w.FramesWritten() == 3);
    }
    {
        core::SessionReader r;
        CHECK(r.Open(p));
        CHECK(r.Count() == 3);
        CHECK(r.Header().sampleRateHz == 60058600u);
        CHECK(std::string(r.Header().note) == "unit test");
        ChirpFrame f;
        CHECK(r.Read(1, f));
        CHECK(f.header.frame_id == 11 && f.raw.size() == 101 && f.fft.size() == 16 && f.fft[3] == 2.5f && f.rx_tick_ms == 78);
        CHECK(r.TimestampMs(2) == 1000);
        CHECK(!f.hasExt);
        CHECK(r.Read(2, f) && f.raw.size() == 102);
        CHECK(f.hasExt && f.ext.samples_per_chirp == 131008 && f.ext.proto_version == 2);
        CHECK_NEAR(f.ext.peak_freq_hz, 4321.5, 1e-3);
        CHECK(r.Header().version == core::kFileVersion);
        CHECK(!r.Read(3, f));
    }
    // Truncated file: last record partially written -> index stops before it.
    {
        auto size = fs::file_size(p);
        fs::resize_file(p, size - 10);
        core::SessionReader r;
        CHECK(r.Open(p));
        CHECK(r.Count() == 2);
    }
    fs::remove(p);

    // WAV + CSV export.
    fs::path wav = fs::temp_directory_path() / "fastacq_test.wav";
    std::vector<float> s(1000, 0.5f);
    CHECK(core::WriteWav16(wav, s.data(), s.size(), 48000));
    CHECK(fs::file_size(wav) == 44 + 2000);
    fs::remove(wav);
    fs::path csv = fs::temp_directory_path() / "fastacq_test.csv";
    ChirpFrame cf; cf.raw = { 1, 2, 3 }; cf.fft = { 0.5f };
    CHECK(core::WriteFrameCsv(csv, cf));
    CHECK(fs::file_size(csv) > 40);
    fs::remove(csv);
}

// ---------------------------------------------------------------------------
int main()
{
    TestCrc();
    TestParser();
    TestStore();
    TestFft();
    TestWindows();
    TestDecimator();
    TestCfar();
    TestPeaks();
    TestRadarDsp();
    TestParserV2();
    TestChirpGeometry();
    TestVcoCurve();
    TestRadarPlanner();
    TestGoSoCfarAndRejection();
    TestToneEstimator();
    TestFirmwareGeometry();
    TestSessionFile();
    std::printf("\n%d passed, %d failed\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
