#pragma once
//
// PeakFinder.h -- local-maximum detection above a threshold with sub-bin
// frequency interpolation (log-parabolic, Jacobsen, Candan, Quinn).
//

#include <complex>
#include <cstddef>
#include <vector>

namespace dsp {

enum class PeakInterp {
    None = 0,
    ParabolicDb,   // parabola through dB values (good for any window)
    Jacobsen,      // complex-bin estimator (rectangular window)
    Candan,        // Jacobsen with bias correction (rectangular window)
    Quinn2,        // Quinn's second estimator (rectangular window)
    Count
};

const char* PeakInterpName(PeakInterp t);

struct Peak {
    size_t bin{0};        // integer bin of the local maximum
    double fracBin{0.0};  // interpolated bin position
    double freqHz{0.0};   // fracBin * freqRes
    float  ampDb{0.0f};   // interpolated amplitude (dB)
    float  snrDb{0.0f};   // ampDb - local noise estimate
};

struct PeakParams {
    size_t     minBin{1};
    size_t     maxBin{0};          // exclusive; 0 = all
    int        maxPeaks{10};
    double     minDistBins{2.0};   // minimum separation between reported peaks
    PeakInterp interp{PeakInterp::ParabolicDb};
};

// db: spectrum in dB; thrDb: per-bin threshold (+inf = never); noiseDb: per-bin noise (dB).
// cplx: optional complex spectrum (same length) for the complex estimators.
std::vector<Peak> FindPeaks(const std::vector<float>& db,
                            const std::vector<float>& thrDb,
                            const std::vector<float>& noiseDb,
                            const std::vector<std::complex<float>>* cplx,
                            double freqResHz,
                            const PeakParams& p);

// Sub-bin offset in [-0.5, 0.5] of the peak at bin k.
double InterpolateOffset(PeakInterp method,
                         const std::vector<float>& db,
                         const std::vector<std::complex<float>>* cplx,
                         size_t k);

} // namespace dsp
