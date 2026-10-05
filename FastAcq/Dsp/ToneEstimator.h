#pragma once
//
// ToneEstimator.h -- precise single-tone frequency estimate of a ramp segment,
// independent of the display FFT settings (size, window, dB scale, zero
// padding). Largest power-of-two block of the segment (<= 65536 samples),
// periodic Hann window, double-precision FFT, linear magnitudes, then the
// Hann ratio estimator delta = (2a - 1) / (a + 1), a = larger neighbour /
// peak. On captured 1 MHz data this is within +/-0.4 Hz of a sine fit,
// where a log-parabolic interpolation of the float spectrum was biased by
// 20..50 Hz. Portable: no Windows/MFC dependencies.
//

#include <cstddef>
#include <cstdint>

namespace dsp {

struct ToneEstimate {
    bool   valid{false};
    double freqHz{0.0};
    double ampFs{0.0};     // amplitude relative to full scale (12-bit), 0..1
    size_t nUsed{0};       // FFT length actually used
};

// ADC codes (12-bit in the low bits of each uint16). nSamples >= 64.
ToneEstimate EstimateTone(const uint16_t* samples, size_t nSamples, double sampleRateHz);

// Real-valued input (already scaled / detrended), same algorithm.
ToneEstimate EstimateTone(const float* samples, size_t nSamples, double sampleRateHz);

} // namespace dsp
