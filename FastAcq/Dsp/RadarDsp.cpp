#include "RadarDsp.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>

namespace dsp {

static constexpr double kPi = 3.14159265358979323846;

const char* RampShapeName(RampShape s)
{
    switch (s) {
    case RampShape::Triangle: return "Triangle (UP+DOWN)";
    case RampShape::Sawtooth: return "Sawtooth (UP only)";
    case RampShape::Single:   return "Single ramp (whole frame)";
    default:                  return "?";
    }
}

const char* TraceModeName(TraceMode m)
{
    switch (m) {
    case TraceMode::ClearWrite: return "Clear write";
    case TraceMode::Average:    return "Average";
    case TraceMode::MaxHold:    return "Max hold";
    case TraceMode::MinHold:    return "Min hold";
    default:                    return "?";
    }
}

// ---------------------------------------------------------------------------
double RadarDsp::RampSeconds(const RadarParams& p, RampShape shape, double chirpFreqHz,
                             size_t samplesPerFrame, double fsHz, int chirps)
{
    if (p.rampSec > 0.0) return p.rampSec;
    if (shape == RampShape::Single) {
        return (fsHz > 0.0) ? static_cast<double>(samplesPerFrame) / fsHz : 0.0;
    }
    if (chirpFreqHz > 0.0) {
        return (shape == RampShape::Triangle) ? 1.0 / (2.0 * chirpFreqHz) : 1.0 / chirpFreqHz;
    }
    if (fsHz > 0.0 && samplesPerFrame > 0) {
        const int M = chirps < 1 ? 1 : chirps;
        const double period = static_cast<double>(samplesPerFrame) / fsHz / M;
        return (shape == RampShape::Triangle) ? period / 2.0 : period;
    }
    return 0.0;
}

int RadarDsp::AutoDecimation(const RadarParams& p, double fsHz, double rampSec,
                             size_t rampLen, size_t guard, double maxRangeM)
{
    if (p.bandwidthHz <= 0.0 || rampSec <= 0.0 || fsHz <= 0.0 || maxRangeM <= 0.0) return 1;
    const double fbMax = 2.0 * p.bandwidthHz * maxRangeM / (kSpeedOfLight * rampSec);
    if (fbMax <= 0.0) return 1;
    int D = 1;
    const size_t usable = (rampLen > 2 * guard) ? rampLen - 2 * guard : 0;
    while (true) {
        const int next = D * 2;
        const double fsNext = fsHz / next;
        if (fsNext / 2.0 < 1.25 * fbMax) break;              // keep 25 % margin above f_b,max
        if (usable / static_cast<size_t>(next) < 64) break;   // keep at least 64 samples
        if (next > 4096) break;
        D = next;
    }
    return D;
}

DerivedValues ComputeDerived(const RadarParams& rp, const DspSettings& ds,
                             double fsHz, double chirpFreqHz, size_t samplesPerFrame,
                             double intervalMs, bool sendRaw, bool sendFft, size_t mcuFftSize)
{
    DerivedValues d;
    const int M = (ds.shape == RampShape::Single) ? 1 : (ds.chirpsInFrame < 1 ? 1 : ds.chirpsInFrame);
    d.rampSec   = RadarDsp::RampSeconds(rp, ds.shape, chirpFreqHz, samplesPerFrame, fsHz, M);
    d.periodSec = (ds.shape == RampShape::Triangle) ? 2.0 * d.rampSec : d.rampSec;

    size_t rampLen = 0;
    if (fsHz > 0.0 && d.rampSec > 0.0)
        rampLen = static_cast<size_t>(d.rampSec * fsHz + 0.5);
    else if (samplesPerFrame > 0)
        rampLen = samplesPerFrame / static_cast<size_t>(M) / ((ds.shape == RampShape::Triangle) ? 2 : 1);
    d.samplesPerRamp = rampLen;

    const size_t guard = static_cast<size_t>(rampLen * (ds.guardPct / 100.0));
    d.decimation = ds.decimation >= 1 ? ds.decimation
                 : RadarDsp::AutoDecimation(rp, fsHz, d.rampSec, rampLen, guard, ds.maxRangeM);
    d.fsEffHz = (d.decimation > 0) ? fsHz / d.decimation : fsHz;

    const size_t usable = (rampLen > 2 * guard) ? rampLen - 2 * guard : 0;
    d.samplesUsed = usable / static_cast<size_t>(d.decimation > 0 ? d.decimation : 1);
    const int zp = ds.zeroPad < 1 ? 1 : ds.zeroPad;
    d.fftSize = d.samplesUsed > 0 ? FftPlan::NextPow2(d.samplesUsed * static_cast<size_t>(zp)) : 0;
    d.binHz   = (d.fftSize > 0) ? d.fsEffHz / static_cast<double>(d.fftSize) : 0.0;

    if (rp.bandwidthHz > 0.0 && d.rampSec > 0.0) {
        d.beatPerMeterHz = 2.0 * rp.bandwidthHz / (kSpeedOfLight * d.rampSec);
        d.rangeResM      = kSpeedOfLight / (2.0 * rp.bandwidthHz);
        const double tObs = (rampLen > 0) ? d.rampSec * static_cast<double>(usable) / static_cast<double>(rampLen) : d.rampSec;
        d.rangeResEffM   = (tObs > 0.0) ? kSpeedOfLight / (2.0 * rp.bandwidthHz * (tObs / d.rampSec)) : 0.0;
        d.rangeMaxM      = (d.fsEffHz / 2.0) / d.beatPerMeterHz;
        d.rangeBinM      = d.binHz / d.beatPerMeterHz;
    }
    const double lambda = rp.LambdaM();
    if (lambda > 0.0 && d.periodSec > 0.0) {
        d.velResMps = lambda / (2.0 * M * d.periodSec);
        d.velMaxMps = lambda / (4.0 * d.periodSec);
    }
    if (intervalMs > 0.0) {
        double bytes = 56.0 + 4.0;
        if (sendRaw) bytes += 2.0 * static_cast<double>(samplesPerFrame);
        if (sendFft) bytes += 4.0 * static_cast<double>(mcuFftSize / 2);
        d.usbMBps = bytes / (intervalMs / 1000.0) / 1e6;
    }
    return d;
}

// ---------------------------------------------------------------------------
RadarDsp::RadarDsp() = default;

void RadarDsp::SetSettings(const DspSettings& s)
{
    const bool resetTrace = (s.trace != m_s.trace) || (s.window != m_s.window) ||
                            (s.zeroPad != m_s.zeroPad) || (s.decimation != m_s.decimation) ||
                            (s.guardPct != m_s.guardPct) || (s.shape != m_s.shape) ||
                            (s.chirpsInFrame != m_s.chirpsInFrame) || (s.useMcuFft != m_s.useMcuFft) ||
                            (s.maxRangeM != m_s.maxRangeM);
    m_s = s;
    if (m_s.chirpsInFrame < 1) m_s.chirpsInFrame = 1;
    if (m_s.zeroPad < 1) m_s.zeroPad = 1;
    if (m_s.guardPct < 0.0f) m_s.guardPct = 0.0f;
    if (m_s.guardPct > 45.0f) m_s.guardPct = 45.0f;
    if (m_s.avgAlpha <= 0.0f) m_s.avgAlpha = 0.01f;
    if (m_s.avgAlpha > 1.0f) m_s.avgAlpha = 1.0f;
    if (resetTrace) {
        m_traceUp = TraceState{};
        m_traceDn = TraceState{};
        m_windows.clear();
    }
}

void RadarDsp::SetParams(const RadarParams& p)
{
    m_p = p;
}

void RadarDsp::ResetState()
{
    m_traceUp = TraceState{};
    m_traceDn = TraceState{};
    m_phasePrev = PhaseSample{};
    m_tracks.clear();
    m_nextTrackId = 1;
}

const std::vector<float>& RadarDsp::WindowFor(size_t n)
{
    auto it = m_windows.find(n);
    if (it == m_windows.end()) {
        auto w = MakeWindow(m_s.window, n, m_s.kaiserBeta);
        it = m_windows.emplace(n, std::move(w)).first;
    }
    return it->second;
}

const FirDecimator& RadarDsp::DecimatorFor(int factor)
{
    auto it = m_decimators.find(factor);
    if (it == m_decimators.end())
        it = m_decimators.emplace(factor, std::make_unique<FirDecimator>(factor)).first;
    return *it->second;
}

const RealFft& RadarDsp::RealFftFor(size_t n)
{
    auto it = m_rfft.find(n);
    if (it == m_rfft.end())
        it = m_rfft.emplace(n, std::make_unique<RealFft>(n)).first;
    return *it->second;
}

const FftPlan& RadarDsp::FftFor(size_t n)
{
    auto it = m_cfft.find(n);
    if (it == m_cfft.end())
        it = m_cfft.emplace(n, std::make_unique<FftPlan>(n)).first;
    return *it->second;
}

// ---------------------------------------------------------------------------
bool RadarDsp::ProcessSegment(const uint16_t* raw, size_t len, size_t guard, int D,
                              double fsHz, SegSpectrum& out, bool keepCplx)
{
    out.valid = false;
    if (len < 16 || fsHz <= 0.0) return false;

    // 1. ADC codes -> full-scale normalized float (12-bit mid-scale = 0).
    m_x.resize(len);
    for (size_t i = 0; i < len; ++i)
        m_x[i] = (static_cast<float>(raw[i] & 0x0FFFu) - 2048.0f) * (1.0f / 2048.0f);

    // 2. Decimate.
    const float* x = m_x.data();
    size_t n = len;
    if (D > 1) {
        DecimatorFor(D).Process(m_x.data(), len, m_xd);
        x = m_xd.data();
        n = m_xd.size();
    }
    const double fsEff = fsHz / D;

    // 3. Guard removal.
    const size_t g = guard / static_cast<size_t>(D);
    if (n <= 2 * g + 8) return false;
    x += g;
    n -= 2 * g;

    // 4. Mean / linear trend removal (least squares).
    double sumY = 0.0, sumTY = 0.0;
    const double tMid = 0.5 * static_cast<double>(n - 1);
    for (size_t i = 0; i < n; ++i) {
        sumY  += x[i];
        sumTY += (static_cast<double>(i) - tMid) * x[i];
    }
    const double mean  = sumY / static_cast<double>(n);
    double sumTT = 0.0;
    for (size_t i = 0; i < n; ++i) { const double t = static_cast<double>(i) - tMid; sumTT += t * t; }
    const double slope = (m_s.detrend && sumTT > 0.0) ? sumTY / sumTT : 0.0;

    // 5. Window + zero padding.
    const std::vector<float>& w = WindowFor(n);
    double wsum = 0.0;
    for (float v : w) wsum += v;
    const double cg = wsum / static_cast<double>(n);
    m_windowCg = cg;

    const int zp = m_s.zeroPad < 1 ? 1 : m_s.zeroPad;
    size_t nFft = FftPlan::NextPow2(n * static_cast<size_t>(zp));
    if (nFft < 4) nFft = 4;
    m_fftIn.assign(nFft, 0.0f);
    for (size_t i = 0; i < n; ++i) {
        const double t = static_cast<double>(i) - tMid;
        m_fftIn[i] = static_cast<float>((x[i] - mean - slope * t) * w[i]);
    }

    // 6. Real FFT.
    const RealFft& fft = RealFftFor(nFft);
    m_fftOut.resize(fft.Bins());
    fft.Forward(m_fftIn.data(), m_fftOut.data());

    // 7. Scale so that a full-scale sine gives |X| = 1 (0 dBFS).
    const float scale = static_cast<float>(2.0 / (static_cast<double>(n) * cg));
    const size_t bins = fft.Bins();
    out.power.resize(bins);
    if (keepCplx) out.cplx.resize(bins); else out.cplx.clear();
    for (size_t k = 0; k < bins; ++k) {
        const std::complex<float> v = m_fftOut[k] * scale;
        out.power[k] = std::norm(v);
        if (keepCplx) out.cplx[k] = v;
    }
    out.nUsed = n;
    out.nFft  = nFft;
    out.fsEff = fsEff;
    out.valid = true;
    return true;
}

void RadarDsp::FinishSpectrum(const std::vector<float>& powerIn, const SegSpectrum& ref,
                              TraceState* trace, bool updateState, RampSpectrum& out)
{
    const size_t bins = powerIn.size();
    out.valid     = ref.valid && bins > 2;
    if (!out.valid) return;
    out.freqResHz = ref.fsEff / static_cast<double>(ref.nFft);
    out.fsEffHz   = ref.fsEff;
    out.nSamples  = ref.nUsed;
    out.nFft      = ref.nFft;
    out.cplx      = ref.cplx;

    // Range-gain tilt (optional): power *= (f/f_ref)^(g/10), f_ref = f(maxRange).
    std::vector<float> power = powerIn;
    if (m_s.rangeGainDbPerDecade > 0.0f && m_p.bandwidthHz > 0.0) {
        const double rampSec = m_p.rampSec > 0.0 ? m_p.rampSec : 0.0;
        double fRef = 0.0;
        if (rampSec > 0.0)
            fRef = 2.0 * m_p.bandwidthHz * m_s.maxRangeM / (kSpeedOfLight * rampSec);
        if (fRef <= 0.0) fRef = ref.fsEff / 4.0;
        const double expo = m_s.rangeGainDbPerDecade / 10.0;
        for (size_t k = 1; k < bins; ++k) {
            const double f = k * out.freqResHz;
            power[k] = static_cast<float>(power[k] * std::pow(f / fRef, expo));
        }
    }

    // Trace mode across frames.
    if (trace && m_s.trace != TraceMode::ClearWrite) {
        if (!trace->valid || trace->power.size() != bins) {
            if (updateState) { trace->power = power; trace->valid = true; }
        } else {
            std::vector<float>& acc = trace->power;
            const float a = m_s.avgAlpha;
            switch (m_s.trace) {
            case TraceMode::Average:
                for (size_t k = 0; k < bins; ++k) acc[k] = (1.0f - a) * acc[k] + a * power[k];
                break;
            case TraceMode::MaxHold:
                for (size_t k = 0; k < bins; ++k) acc[k] = (std::max)(acc[k], power[k]);
                break;
            case TraceMode::MinHold:
                for (size_t k = 0; k < bins; ++k) acc[k] = (std::min)(acc[k], power[k]);
                break;
            default: break;
            }
            if (updateState) power = acc;
            else {
                // Viewing without touching state: show the combination but do not store it.
                std::vector<float> tmp = acc;
                for (size_t k = 0; k < bins; ++k) {
                    switch (m_s.trace) {
                    case TraceMode::Average: tmp[k] = (1.0f - a) * tmp[k] + a * power[k]; break;
                    case TraceMode::MaxHold: tmp[k] = (std::max)(tmp[k], power[k]); break;
                    case TraceMode::MinHold: tmp[k] = (std::min)(tmp[k], power[k]); break;
                    default: break;
                    }
                }
                power = tmp;
            }
        }
    }

    // dB conversion.
    out.db.resize(bins);
    for (size_t k = 0; k < bins; ++k) out.db[k] = PowerToDb(power[k]);

    // DC skip: window main lobe (in padded bins) + 1.
    const int zp = m_s.zeroPad < 1 ? 1 : m_s.zeroPad;
    size_t skip = m_s.skipDcBins > 0 ? static_cast<size_t>(m_s.skipDcBins)
                : static_cast<size_t>(std::ceil(WindowMainLobeBins(m_s.window, m_s.kaiserBeta) * zp)) + 1;
    if (skip >= bins - 2) skip = bins > 3 ? bins - 3 : 1;
    out.firstBin = skip;

    // Detector.
    DetectorOutput det;
    DetectorParams dp = m_s.detector;
    dp.guardCells = static_cast<int>(std::lround(dp.guardCells * static_cast<double>(zp)));
    dp.trainCells = static_cast<int>(std::lround(dp.trainCells * static_cast<double>(zp)));
    if (dp.guardCells < 1) dp.guardCells = 1;
    if (dp.trainCells < 2) dp.trainCells = 2;
    RunDetector(power, skip, bins - 1, dp, det);
    out.thrDb        = std::move(det.thresholdDb);
    out.noiseDb      = std::move(det.noiseDb);
    out.noiseFloorDb = det.globalNoiseDb;

    // Peaks.
    PeakParams pp;
    pp.minBin      = skip;
    pp.maxBin      = bins - 1;
    pp.maxPeaks    = m_s.maxPeaks;
    pp.minDistBins = m_s.minPeakDistBins > 0.0 ? m_s.minPeakDistBins
                   : WindowMainLobeBins(m_s.window, m_s.kaiserBeta) * zp;
    pp.interp      = m_s.interp;
    out.peaks = FindPeaks(out.db, out.thrDb, out.noiseDb,
                          out.cplx.empty() ? nullptr : &out.cplx, out.freqResHz, pp);
}

void RadarDsp::BuildMcuSpectrum(const ChirpFrame& f, RampSpectrum& out)
{
    out = RampSpectrum{};
    const size_t bins = f.fft.size();
    if (bins < 3) return;
    const uint32_t nfft = f.header.fft_size > 0 ? f.header.fft_size : static_cast<uint32_t>(bins * 2);
    // Assume unnormalized FFT of 12-bit codes: full-scale sine -> (N/2) * 2048.
    const double ref = (static_cast<double>(nfft) / 2.0) * 2048.0;
    std::vector<float> power(bins);
    for (size_t k = 0; k < bins; ++k) {
        double m = f.fft[k];
        if (!(m == m) || m < 0.0) m = 0.0;
        const double a = m / ref;
        power[k] = static_cast<float>(a * a);
    }
    SegSpectrum ref2;
    ref2.valid = true;
    ref2.nUsed = nfft;
    ref2.nFft  = nfft;
    ref2.fsEff = f.header.fft_freq_res_hz > 0.0f ? static_cast<double>(f.header.fft_freq_res_hz) * nfft
                                                 : (f.header.sample_rate_hz > 0 ? f.header.sample_rate_hz : m_fallbackFs);
    FinishSpectrum(power, ref2, nullptr, false, out);
}

// ---------------------------------------------------------------------------
void RadarDsp::BuildTargets(FrameResult& r, bool updateState)
{
    r.targets.clear();
    const double rangePerHz = r.rangePerHz;
    const double lambda     = r.lambdaM;
    auto rangeOf = [&](double fHz) {
        return rangePerHz > 0.0 ? rangePerHz * fHz - m_p.rangeOffsetM : 0.0;
    };

    if (r.shape == RampShape::Triangle && r.up.valid && r.down.valid) {
        const double gateHz = (lambda > 0.0) ? 2.0 * m_p.pairMaxVelocityMps / lambda
                                             : 10.0 * r.up.freqResHz;
        std::vector<bool> usedDn(r.down.peaks.size(), false);
        for (const Peak& pu : r.up.peaks) {
            int best = -1; double bestD = 1e300;
            for (size_t j = 0; j < r.down.peaks.size(); ++j) {
                if (usedDn[j]) continue;
                const double d = std::fabs(r.down.peaks[j].freqHz - pu.freqHz);
                if (d <= gateHz && d < bestD) { bestD = d; best = static_cast<int>(j); }
            }
            Target t;
            t.fUpHz = pu.freqHz;
            t.ampDb = pu.ampDb;
            t.snrDb = pu.snrDb;
            if (best >= 0) {
                usedDn[static_cast<size_t>(best)] = true;
                const Peak& pd = r.down.peaks[static_cast<size_t>(best)];
                t.fDnHz  = pd.freqHz;
                t.paired = true;
                t.rangeM = rangeOf(0.5 * (pu.freqHz + pd.freqHz));
                t.velocityMps = (lambda > 0.0) ? lambda * (pd.freqHz - pu.freqHz) / 4.0 : 0.0;
                t.ampDb = 0.5f * (pu.ampDb + pd.ampDb);
                t.snrDb = (std::min)(pu.snrDb, pd.snrDb);
            } else {
                t.rangeM = rangeOf(pu.freqHz);
            }
            r.targets.push_back(t);
        }
        for (size_t j = 0; j < r.down.peaks.size(); ++j) {
            if (usedDn[j]) continue;
            const Peak& pd = r.down.peaks[j];
            Target t;
            t.fDnHz  = pd.freqHz;
            t.rangeM = rangeOf(pd.freqHz);
            t.ampDb  = pd.ampDb;
            t.snrDb  = pd.snrDb;
            r.targets.push_back(t);
        }
    } else if (r.up.valid) {
        for (const Peak& pu : r.up.peaks) {
            Target t;
            t.fUpHz  = pu.freqHz;
            t.rangeM = rangeOf(pu.freqHz);
            t.ampDb  = pu.ampDb;
            t.snrDb  = pu.snrDb;
            r.targets.push_back(t);
        }
    }

    std::sort(r.targets.begin(), r.targets.end(),
              [](const Target& a, const Target& b) { return a.ampDb > b.ampDb; });
    if (m_s.maxPeaks > 0 && static_cast<int>(r.targets.size()) > m_s.maxPeaks)
        r.targets.resize(static_cast<size_t>(m_s.maxPeaks));

    // Simple nearest-neighbour tracking for stable IDs.
    if (!m_s.trackTargets) return;
    const double rangeBin = (rangePerHz > 0.0 && r.up.valid) ? rangePerHz * r.up.freqResHz : 0.0;
    const double gate = (std::max)(3.0 * rangeBin, 0.25);
    std::vector<bool> trackUsed(m_tracks.size(), false);
    std::vector<Track> updated;
    for (Target& t : r.targets) {
        const double key = rangePerHz > 0.0 ? t.rangeM : (t.fUpHz > 0.0 ? t.fUpHz : t.fDnHz);
        int best = -1; double bestD = 1e300;
        for (size_t i = 0; i < m_tracks.size(); ++i) {
            if (trackUsed[i]) continue;
            const double d = std::fabs(m_tracks[i].rangeM - key);
            const double g = rangePerHz > 0.0 ? gate : (std::max)(3.0 * r.up.freqResHz, 1.0);
            if (d <= g && d < bestD) { bestD = d; best = static_cast<int>(i); }
        }
        if (best >= 0) {
            trackUsed[static_cast<size_t>(best)] = true;
            Track tr = m_tracks[static_cast<size_t>(best)];
            tr.rangeM   = 0.5 * (tr.rangeM + key);
            tr.velocity = t.velocityMps;
            tr.missed   = 0;
            tr.age++;
            t.id = tr.id;
            updated.push_back(tr);
        } else {
            Track tr{ m_nextTrackId, key, t.velocityMps, 0, 1 };
            t.id = tr.id;
            if (updateState) m_nextTrackId++;
            updated.push_back(tr);
        }
    }
    if (updateState) {
        for (size_t i = 0; i < m_tracks.size(); ++i) {
            if (trackUsed[i]) continue;
            Track tr = m_tracks[i];
            if (++tr.missed <= 5) updated.push_back(tr);
        }
        m_tracks = std::move(updated);
    }
}

void RadarDsp::BuildRangeDoppler(const std::vector<SegSpectrum>& chirps, FrameResult& r)
{
    r.rd = RangeDopplerMap{};
    const size_t M = chirps.size();
    if (M < 2 || !chirps[0].valid) return;
    const size_t bins = chirps[0].cplx.size();
    if (bins < 3) return;
    for (const auto& c : chirps) if (!c.valid || c.cplx.size() != bins) return;

    const double freqRes = chirps[0].fsEff / static_cast<double>(chirps[0].nFft);
    // Limit range extent to the range of interest.
    size_t nRange = bins;
    if (r.rangePerHz > 0.0 && m_s.maxRangeM > 0.0) {
        const double fMax = m_s.maxRangeM / r.rangePerHz;
        size_t kMax = static_cast<size_t>(fMax / freqRes) + 2;
        if (kMax < nRange) nRange = kMax;
    }
    if (nRange > 4096) nRange = 4096;
    if (nRange < 2) return;

    size_t nD = FftPlan::NextPow2(M);
    if (nD < 8) nD = 8;
    const FftPlan& plan = FftFor(nD);
    const std::vector<float> wm = MakeWindow(WindowType::Hann, M);
    double wsum = 0.0; for (float v : wm) wsum += v;
    const float scale = static_cast<float>(1.0 / (wsum > 0.0 ? wsum : 1.0));

    r.rd.nRange   = static_cast<int>(nRange);
    r.rd.nDoppler = static_cast<int>(nD);
    r.rd.db.assign(nRange * nD, -300.0f);
    r.rd.freqResHz = freqRes;
    r.rd.rangeBinM = r.rangePerHz > 0.0 ? r.rangePerHz * freqRes : 0.0;
    const double tPri = r.periodSec;
    r.rd.velBinMps = (r.lambdaM > 0.0 && tPri > 0.0) ? r.lambdaM / (2.0 * nD * tPri) : 0.0;
    r.rd.velMaxMps = (r.lambdaM > 0.0 && tPri > 0.0) ? r.lambdaM / (4.0 * tPri) : 0.0;

    std::vector<std::complex<float>> col(nD);
    float maxDb = -300.0f, minDb = 300.0f;
    for (size_t k = 0; k < nRange; ++k) {
        std::complex<float> mean(0.0f, 0.0f);
        for (size_t m = 0; m < M; ++m) mean += chirps[m].cplx[k];
        mean *= (1.0f / static_cast<float>(M));
        for (size_t m = 0; m < nD; ++m) {
            if (m < M) {
                std::complex<float> v = chirps[m].cplx[k];
                if (m_s.mti) v -= mean;
                col[m] = v * (wm[m] * scale);
            } else {
                col[m] = std::complex<float>(0.0f, 0.0f);
            }
        }
        plan.Forward(col.data());
        // Row r <-> velocity (r - nD/2) * velBin, positive = approaching.
        // An approaching target has a NEGATIVE slow-time frequency (phase
        // 4*pi*R/lambda decreases), so row r maps to FFT bin (nD/2 - r) mod nD.
        for (size_t i = 0; i < nD; ++i) {
            const size_t src = (nD + nD / 2 - i) % nD;
            const float p = std::norm(col[src]);
            const float dbv = PowerToDb(p);
            r.rd.db[i * nRange + k] = dbv;
            if (k >= 1) {
                if (dbv > maxDb) { maxDb = dbv; r.rd.peakRange = static_cast<int>(k); r.rd.peakDoppler = static_cast<int>(i); }
                if (dbv < minDb) minDb = dbv;
            }
        }
    }
    r.rd.peakDb = maxDb;
    r.rd.maxDb  = maxDb;
    r.rd.minDb  = minDb;
}

void RadarDsp::TrackPhase(FrameResult& r, bool updateState)
{
    r.phase = PhaseSample{};
    if (!r.up.valid || r.up.cplx.empty()) return;
    int bin = m_s.phaseTrackBin;
    if (bin < 0) {
        if (r.up.peaks.empty()) return;
        bin = static_cast<int>(r.up.peaks[0].bin);
    }
    if (bin <= 0 || static_cast<size_t>(bin) >= r.up.cplx.size()) return;

    PhaseSample ps;
    ps.valid    = true;
    ps.bin      = bin;
    ps.freqHz   = bin * r.up.freqResHz;
    ps.phaseRad = std::arg(r.up.cplx[static_cast<size_t>(bin)]);
    ps.displacementMm = m_phasePrev.valid ? m_phasePrev.displacementMm : 0.0;
    if (m_phasePrev.valid && m_phasePrev.bin == bin && r.lambdaM > 0.0) {
        double d = ps.phaseRad - m_phasePrev.phaseRad;
        while (d >  kPi) d -= 2.0 * kPi;
        while (d < -kPi) d += 2.0 * kPi;
        ps.displacementMm += r.lambdaM * d / (4.0 * kPi) * 1000.0;
    } else if (!(m_phasePrev.valid && m_phasePrev.bin == bin)) {
        ps.displacementMm = 0.0;
    }
    r.phase = ps;
    if (updateState) m_phasePrev = ps;
}

// ---------------------------------------------------------------------------
FrameResult RadarDsp::Process(const ChirpFrame& f, bool updateState)
{
    const auto t0 = std::chrono::steady_clock::now();
    FrameResult r;
    r.seq         = f.seq;
    r.frameId     = f.header.frame_id;
    r.timestampMs = f.header.timestamp_ms;
    r.rxTickMs    = f.rx_tick_ms;
    r.fsHz        = f.header.sample_rate_hz > 0 ? static_cast<double>(f.header.sample_rate_hz) : m_fallbackFs;
    r.chirpFreqHz = f.header.chirp_freq_hz;
    r.shape       = m_s.shape;
    r.chirps      = (m_s.shape == RampShape::Single) ? 1 : m_s.chirpsInFrame;
    r.samplesPerFrame = f.raw.size();
    r.lambdaM     = m_p.LambdaM();
    r.rangeOffsetM = m_p.rangeOffsetM;

    const size_t n = f.raw.size();
    r.rampSec   = RampSeconds(m_p, m_s.shape, r.chirpFreqHz, n, r.fsHz, r.chirps);
    r.periodSec = (m_s.shape == RampShape::Triangle) ? 2.0 * r.rampSec : r.rampSec;
    r.rangePerHz = (m_p.bandwidthHz > 0.0 && r.rampSec > 0.0)
                 ? kSpeedOfLight * r.rampSec / (2.0 * m_p.bandwidthHz) : 0.0;

    if (m_s.useMcuFft || (n == 0 && !f.fft.empty())) {
        BuildMcuSpectrum(f, r.up);
        r.fromMcuFft = true;
        r.valid = r.up.valid;
        if (r.valid) {
            // UP/DOWN pairing is meaningless for a whole-frame MCU spectrum.
            RampShape saved = r.shape;
            r.shape = RampShape::Sawtooth;
            BuildTargets(r, updateState);
            r.shape = saved;
            TrackPhase(r, updateState);
        }
        r.processingMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
        return r;
    }

    if (n < 32) return r;

    const int    M         = r.chirps < 1 ? 1 : r.chirps;
    const size_t periodLen = n / static_cast<size_t>(M);
    const size_t rampLen   = (m_s.shape == RampShape::Triangle) ? periodLen / 2 : periodLen;
    if (rampLen < 16) return r;
    const size_t guard = static_cast<size_t>(rampLen * (m_s.guardPct / 100.0f));
    const int D = m_s.decimation >= 1 ? m_s.decimation
                : AutoDecimation(m_p, r.fsHz, r.rampSec, rampLen, guard, m_s.maxRangeM);
    r.decimation = D;

    // Segments for the waveform display.
    for (int m = 0; m < M; ++m) {
        const size_t base = static_cast<size_t>(m) * periodLen;
        auto addRamp = [&](size_t start, size_t len, Segment::Kind kind) {
            if (guard > 0) r.segments.push_back({ start, guard, Segment::Guard, m });
            r.segments.push_back({ start + guard, len - 2 * guard, kind, m });
            if (guard > 0) r.segments.push_back({ start + len - guard, guard, Segment::Guard, m });
        };
        addRamp(base, rampLen, Segment::Up);
        if (m_s.shape == RampShape::Triangle)
            addRamp(base + rampLen, periodLen - rampLen, Segment::Down);
    }

    // Per-chirp spectra, non-coherent average over chirps.
    std::vector<SegSpectrum> upChirps(static_cast<size_t>(M));
    std::vector<SegSpectrum> dnChirps;
    if (m_s.shape == RampShape::Triangle) dnChirps.resize(static_cast<size_t>(M));

    std::vector<float> upPower, dnPower;
    bool upOk = true, dnOk = (m_s.shape == RampShape::Triangle);
    for (int m = 0; m < M; ++m) {
        const size_t base = static_cast<size_t>(m) * periodLen;
        SegSpectrum& su = upChirps[static_cast<size_t>(m)];
        if (!ProcessSegment(f.raw.data() + base, rampLen, guard, D, r.fsHz, su, true)) { upOk = false; break; }
        if (upPower.empty()) upPower = su.power;
        else for (size_t k = 0; k < upPower.size() && k < su.power.size(); ++k) upPower[k] += su.power[k];
        if (m_s.shape == RampShape::Triangle) {
            SegSpectrum& sd = dnChirps[static_cast<size_t>(m)];
            if (!ProcessSegment(f.raw.data() + base + rampLen, periodLen - rampLen, guard, D, r.fsHz, sd, m == 0)) { dnOk = false; }
            else {
                if (dnPower.empty()) dnPower = sd.power;
                else for (size_t k = 0; k < dnPower.size() && k < sd.power.size(); ++k) dnPower[k] += sd.power[k];
            }
        }
    }
    if (upOk) {
        if (M > 1) for (float& v : upPower) v /= static_cast<float>(M);
        FinishSpectrum(upPower, upChirps[0], &m_traceUp, updateState, r.up);
    }
    if (dnOk && !dnPower.empty()) {
        if (M > 1) for (float& v : dnPower) v /= static_cast<float>(M);
        FinishSpectrum(dnPower, dnChirps[0], &m_traceDn, updateState, r.down);
    }
    r.valid = r.up.valid;
    if (!r.valid) return r;

    BuildTargets(r, updateState);
    if (M > 1) BuildRangeDoppler(upChirps, r);
    TrackPhase(r, updateState);

    r.processingMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    return r;
}

} // namespace dsp
