#include "RadarPlanner.h"

#include <algorithm>
#include <cmath>

namespace core {

namespace {
constexpr double kC = 299792458.0;
constexpr int    kDacMax = 4095;

int DacOfVtune(double v, double v0, double vFull)
{
    const double code = (v - v0) / (vFull - v0) * kDacMax;
    return std::clamp(static_cast<int>(std::lround(code)), 0, kDacMax);
}
double VtuneOfDac(int code, double v0, double vFull) { return v0 + (vFull - v0) * code / static_cast<double>(kDacMax); }
} // namespace

bool PlanResult::Has(PlanNote n) const { return std::find(notes.begin(), notes.end(), n) != notes.end(); }

PlanResult PlanRadar(const PlanInput& in)
{
    PlanResult r;
    if (!(in.rMaxM > 0.0) || !(in.rMinM >= 0.0) || !(in.rMinM < in.rMaxM) || !(in.fbMaxHz > 0.0) ||
        !(in.fsHz > 0.0) || !(in.vtuneAtDacFullV > in.vtuneAtDac0V) || !in.curve.Valid())
        return r;

    // 1. Tuning-voltage window -> DAC codes (what the MCU can actually produce).
    const double lo = std::clamp(in.vLowV, in.vtuneAtDac0V, in.vtuneAtDacFullV);
    const double hi = std::clamp(in.vHighV, in.vtuneAtDac0V, in.vtuneAtDacFullV);
    if (std::fabs(lo - in.vLowV) > 1e-9 || std::fabs(hi - in.vHighV) > 1e-9) r.notes.push_back(PlanNote::VtuneClamped);
    int off = DacOfVtune(std::min(lo, hi), in.vtuneAtDac0V, in.vtuneAtDacFullV);
    int top = DacOfVtune(std::max(lo, hi), in.vtuneAtDac0V, in.vtuneAtDacFullV);
    if (top <= off) { if (off >= kDacMax) off = kDacMax - 1; top = off + 1; }
    r.dacOffset    = off;
    r.dacAmplitude = top - off;
    r.vLowV  = VtuneOfDac(off, in.vtuneAtDac0V, in.vtuneAtDacFullV);
    r.vHighV = VtuneOfDac(top, in.vtuneAtDac0V, in.vtuneAtDacFullV);

    const VcoSweep sw = ComputeVcoSweep(in.curve, r.vLowV, r.vHighV);
    if (!sw.valid || !(sw.bandwidthHz > 0.0)) return r;
    if (sw.outOfTable) r.notes.push_back(PlanNote::VtuneOutsideCurve);
    r.fStartHz = sw.fStartHz; r.fStopHz = sw.fStopHz; r.f0Hz = sw.f0Hz;
    r.bandwidthHz = sw.bandwidthHz; r.nonlinearityPct = sw.nonlinearityPct;
    if (sw.nonlinearityPct > 10.0) r.notes.push_back(PlanNote::SweepNonlinear);
    const double B = r.bandwidthHz, f0 = r.f0Hz, lambda = kC / f0;

    // 2. Slope from the beat-frequency band: the farthest target must stay
    //    at or below fbMax (slope <= sMax); the nearest one should stay at or
    //    above fbMin (slope >= sMin). The ratio f_b(rMin)/f_b(rMax) = rMin/rMax
    //    is fixed, so the band is placed with equal relative margins to both
    //    limits (geometric mean): as far from DC as from the ADC ceiling.
    //    With fbMin = 0 only the ceiling counts.
    const double sMax = in.fbMaxHz * kC / (2.0 * in.rMaxM);
    const double sMin = (in.rMinM > 0.0 && in.fbMinHz > 0.0) ? in.fbMinHz * kC / (2.0 * in.rMinM) : 0.0;
    if (sMin > sMax * (1.0 + 1e-9)) r.notes.push_back(PlanNote::BeatBandConflict);
    double S = (sMin > 0.0 && sMin < sMax) ? std::sqrt(sMin * sMax) : sMax;
    double T = B / S;

    // 3. Chirp repetition rate the MCU accepts (symmetric triangle: period 2T).
    double f = 1.0 / (2.0 * T);
    if (f < kChirpFreqMinHz) { f = kChirpFreqMinHz; r.notes.push_back(PlanNote::ChirpFreqClampedLow); }
    if (f > kChirpFreqMaxHz) { f = kChirpFreqMaxHz; r.notes.push_back(PlanNote::ChirpFreqClampedHigh); }
    r.chirpFreqHz = static_cast<uint32_t>(std::lround(f));
    r.chirpFreqHz = std::clamp(r.chirpFreqHz, kChirpFreqMinHz, kChirpFreqMaxHz);
    T = 1.0 / (2.0 * r.chirpFreqHz);
    S = B / T;
    r.rampSec = T; r.slopeHzPerS = S;

    // 4. Beat tones of the band edges.
    r.beatPerMeterHz = 2.0 * B / (kC * T);
    r.fbAtRMinHz = r.beatPerMeterHz * in.rMinM;
    r.fbAtRMaxHz = r.beatPerMeterHz * in.rMaxM;

    // 5. Velocity: UP/DOWN pairing and, when a finer step is wanted, a burst.
    const double Tp = 2.0 * T;
    r.velStepPairMps   = lambda / (4.0 * T);
    r.velMaxPairMps    = r.fbAtRMinHz * lambda / 2.0;
    r.velMaxDopplerMps = lambda / (4.0 * Tp);
    if (in.rMinM > 0.0 && in.vMaxMps > r.velMaxPairMps) r.notes.push_back(PlanNote::DopplerExceedsBeatAtRmin);

    uint32_t N = 1;
    if (in.vMinMps > 0.0 && r.velStepPairMps > in.vMinMps) {
        const double need = lambda / (2.0 * Tp * in.vMinMps);
        N = static_cast<uint32_t>(std::max(2.0, std::ceil(need - 1e-9)));
    }
    ChirpParams cp;
    cp.freqHz = r.chirpFreqHz; cp.amplitude = static_cast<uint32_t>(r.dacAmplitude);
    cp.offset = static_cast<uint32_t>(r.dacOffset); cp.burst = 1; cp.intervalMs = in.intervalMs;
    const ChirpGeometry g1 = ComputeChirpGeometry(cp);
    if (!g1.valid || g1.samplesPerChirp == 0) return r;
    const uint32_t usableCapture = (kChirpCaptureMax / kChirpDmaChunk) * kChirpDmaChunk;
    const uint32_t maxN = std::max<uint32_t>(1u, usableCapture / g1.samplesPerChirp);
    if (N > maxN) { N = maxN; r.notes.push_back(PlanNote::BurstLimitedByCapture); }
    N = std::min<uint32_t>(N, 65535u);
    r.burst = N;
    cp.burst = N;
    r.geometry = ComputeChirpGeometry(cp);
    r.velStepDopplerMps = (N > 1) ? lambda / (2.0 * N * Tp) : r.velStepPairMps;
    if (in.vMinMps > 0.0 && std::min(r.velStepPairMps, r.velStepDopplerMps) > in.vMinMps * 1.01)
        r.notes.push_back(PlanNote::VelocityStepNotReached);
    const double burstSec = r.geometry.valid ? r.geometry.burstUs * 1e-6 : 0.0;
    r.frameRateHz = (burstSec + in.intervalMs * 1e-3 > 0.0) ? 1.0 / (burstSec + in.intervalMs * 1e-3) : 0.0;

    // 6. Decimation: keep fs_eff / 2 at least 25 % above the farthest beat
    //    tone and at least 64 samples per ramp (same rule as the auto mode).
    const size_t rampLen = static_cast<size_t>(T * in.fsHz + 0.5);
    const size_t guard   = static_cast<size_t>(rampLen * (std::clamp(in.guardPct, 0.0, 45.0) / 100.0));
    const size_t usable  = (rampLen > 2 * guard) ? rampLen - 2 * guard : 0;
    int D = 1;
    while (true) {
        const int next = D * 2;
        if (in.fsHz / next / 2.0 < 1.25 * r.fbAtRMaxHz) break;
        if (usable / static_cast<size_t>(next) < 64) break;
        if (next > 4096) break;
        D = next;
    }
    r.decimation = D;
    r.fsEffHz = in.fsHz / D;
    r.samplesPerRamp = rampLen;
    r.samplesUsed = usable / static_cast<size_t>(D);
    r.rangeResM = kC / (2.0 * B);
    r.rangeMaxM = (r.fsEffHz / 2.0) / r.beatPerMeterHz;

    // 7. Processing suggestions.
    r.pairGateMps      = std::max(1.0, in.vMaxMps * 1.5);
    r.rangeOfInterestM = in.rMaxM * 1.2;
    r.valid = true;
    return r;
}

} // namespace core
