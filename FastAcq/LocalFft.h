#pragma once
//
// LocalFft.h -- PC-side FFT computation for RAW-data mode.
// Radix-2 Cooley-Tukey FFT + window functions.
//

#include "pch.h"
#include <vector>
#include <complex>

enum class FftWindow { Rectangular = 0, Hann, Hamming, Blackman };

struct FftSettings {
    int       size{4096};           // FFT size (must be power of 2)
    FftWindow window{FftWindow::Hann};
    bool      logScale{false};      // display log magnitude
};

class LocalFft {
public:
    // Compute magnitude spectrum from uint16 raw samples.
    // Returns magnitude array of size fftSize/2.
    // freqResHz = sampleRateHz / fftSize  (returned via out param).
    static std::vector<float> Compute(const uint16_t* samples, size_t nSamples,
                                      uint32_t sampleRateHz,
                                      const FftSettings& cfg,
                                      float& outFreqResHz);

    // Estimate dominant frequency (Hz) by FFT peak.
    static float PeakFrequencyHz(const std::vector<float>& mag, float freqResHz);

private:
    static void   Radix2FFT(std::vector<std::complex<float>>& x);
    static float  WindowVal(FftWindow w, int i, int N);
};
