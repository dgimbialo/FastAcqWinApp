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
    int       size{16384};          // FFT size (must be power of 2); higher = better freq resolution
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

    // Frequency (Hz) of the dominant tone in a raw sample segment, with
    // sub-bin accuracy. Uses its own FFT; independent of FftSettings.
    static double EstimateToneHz(const uint16_t* samples, size_t nSamples, uint32_t sampleRateHz);

private:
    static void   Radix2FFT(std::vector<std::complex<float>>& x);
    static float  WindowVal(FftWindow w, int i, int N);
};
