#pragma once
//
// Rejection.h -- post-detection filters on the peak list of one ramp:
//   * harmonics of a stronger peak (mixer / ADC nonlinearity puts copies of a
//     strong beat tone at 2f, 3f, ...),
//   * known spurious frequencies (clock leakage, supply spurs, a fixed
//     reflection), given as a list of centre +/- half width in Hz,
//   * peaks below a minimum SNR.
// Portable: no Windows/MFC dependencies.
//

#include "PeakFinder.h"

#include <string>
#include <utility>
#include <vector>

namespace dsp {

struct SpurBand {
    double centerHz{0.0};
    double halfWidthHz{0.0};
};

struct RejectionParams {
    bool   harmonics{false};        // drop peaks at k * f of a stronger peak (k = 2..maxHarmonic)
    int    maxHarmonic{5};
    double harmonicTolBins{1.5};    // tolerance per harmonic order, in bins
    float  harmonicMinDropDb{6.0f}; // the harmonic must be at least this much weaker
    float  minSnrDb{0.0f};          // 0 = off
    std::vector<SpurBand> spurs;    // ignored frequency bands
};

struct RejectionStats {
    int harmonics{0};
    int spurs{0};
    int lowSnr{0};
    int Total() const { return harmonics + spurs + lowSnr; }
};

// peaks must be sorted by amplitude, descending (as FindPeaks returns them).
void RejectPeaks(std::vector<Peak>& peaks, double freqResHz, const RejectionParams& p, RejectionStats& stats);

// "1000000:2000, 2.5e6:1000" -> bands (centre Hz : half width Hz; the half
// width may be omitted, then defaultHalfWidthHz is used). Separators , ; space.
std::vector<SpurBand> ParseSpurList(const std::string& text, double defaultHalfWidthHz);
std::string FormatSpurList(const std::vector<SpurBand>& spurs);

} // namespace dsp
