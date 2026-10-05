#pragma once
//
// Window.h -- spectral window functions with gain/bandwidth figures.
//

#include <cstddef>
#include <vector>

namespace dsp {

enum class WindowType {
    Rectangular = 0,
    Hann,
    Hamming,
    Blackman,
    BlackmanHarris4,
    Kaiser,
    FlatTop,
    Count
};

const char* WindowName(WindowType t);

// Symmetric window of length n. kaiserBeta is used by Kaiser only.
std::vector<float> MakeWindow(WindowType t, size_t n, float kaiserBeta = 9.0f);

struct WindowInfo {
    double coherentGain{1.0};   // sum(w)/n  (amplitude scaling of a sinusoid)
    double enbwBins{1.0};       // equivalent noise bandwidth in bins
    double mainLobeBins{1.0};   // distance from peak to first null, in (unpadded) bins
};

// Figures of merit for a given window instance.
WindowInfo AnalyzeWindow(const std::vector<float>& w, WindowType t, float kaiserBeta = 9.0f);

// Approximate main-lobe half width (peak to first null) in bins.
double WindowMainLobeBins(WindowType t, float kaiserBeta = 9.0f);

// Modified Bessel function of the first kind, order 0.
double BesselI0(double x);

} // namespace dsp
