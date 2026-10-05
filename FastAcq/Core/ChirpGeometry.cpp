#include "ChirpGeometry.h"

#include <algorithm>

namespace core {

template <class T> static T ClampT(T v, T lo, T hi) { return v < lo ? lo : (v > hi ? hi : v); }

ChirpGeometry ComputeChirpGeometry(const ChirpParams& p)
{
    ChirpGeometry g;
    uint32_t periodTicks, riseTicks;
    if (p.riseUs && p.fallUs) {
        g.rampMode  = true;
        periodTicks = (p.riseUs + p.fallUs) * (kChirpTim7ClkHz / 1000000u);
        riseTicks   = p.riseUs * (kChirpTim7ClkHz / 1000000u);
    } else {
        if (p.freqHz < kChirpFreqMinHz || p.freqHz > kChirpFreqMaxHz) return g;
        periodTicks = kChirpTim7ClkHz / p.freqHz;
        riseTicks   = periodTicks / 2u;
    }
    if (periodTicks < kChirpTim7ClkHz / kChirpFreqMaxHz || periodTicks > kChirpTim7ClkHz / kChirpFreqMinHz)
        return g;

    uint32_t tps = (periodTicks + kChirpTriangleMax - 1u) / kChirpTriangleMax;
    if (tps < kChirpMinTicksPerPt) tps = kChirpMinTicksPerPt;
    tps = (tps + 3u) & ~3u;
    uint32_t len = (periodTicks + tps / 2u) / tps;
    len = ClampT<uint32_t>(len, 2u, kChirpTriangleMax);
    uint32_t rise = static_cast<uint32_t>(
        (static_cast<uint64_t>(len) * riseTicks + periodTicks / 2u) / periodTicks);
    rise = ClampT<uint32_t>(rise, 1u, len - 1u);

    g.valid           = true;
    g.tableLen        = len;
    g.riseLen         = rise;
    g.ticksPerSample  = tps;
    g.periodTicks     = len * tps;
    g.samplesPerChirp = g.periodTicks / 4u;           // ADC = TIM7 / 4
    g.riseSamples     = (rise * tps) / 4u;
    g.riseUs          = static_cast<double>(rise) * tps / (kChirpTim7ClkHz / 1e6);
    g.fallUs          = static_cast<double>(len - rise) * tps / (kChirpTim7ClkHz / 1e6);
    g.periodUs        = g.riseUs + g.fallUs;
    g.freqHz          = 1e6 / g.periodUs;

    const uint64_t burst = static_cast<uint64_t>(g.samplesPerChirp) * (std::max)(1u, p.burst);
    g.burstSamplesNeeded = burst;
    g.samplesPerBurst = static_cast<uint32_t>((std::min<uint64_t>)(burst, kChirpCaptureMax));
    uint32_t tgt = ((g.samplesPerBurst + kChirpDmaChunk - 1u) / kChirpDmaChunk) * kChirpDmaChunk;
    if (p.samplesOvr)
        tgt = (((std::min)(p.samplesOvr, kChirpCaptureMax) + kChirpDmaChunk - 1u) / kChirpDmaChunk) * kChirpDmaChunk;
    tgt = (std::min)(tgt, (kChirpCaptureMax / kChirpDmaChunk) * kChirpDmaChunk);
    g.captureTarget = tgt;
    g.chunks        = tgt / kChirpDmaChunk;

    const double usPerSample = 1e6 / static_cast<double>(kChirpAdcHz);
    g.burstUs           = static_cast<double>(burst) * usPerSample;
    g.captureUs         = static_cast<double>(tgt) * usPerSample;
    g.chirpsCaptured    = static_cast<double>(tgt) / static_cast<double>(g.samplesPerChirp);
    g.fitsInCapture     = (burst <= tgt);
    g.clippedByOverride = (p.samplesOvr != 0) && (burst > tgt) &&
                          (burst <= (kChirpCaptureMax / kChirpDmaChunk) * kChirpDmaChunk);
    return g;
}

} // namespace core
