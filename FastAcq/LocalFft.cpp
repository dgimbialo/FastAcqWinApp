#include "pch.h"
#include "LocalFft.h"

#define _USE_MATH_DEFINES
#include <cmath>
#include <algorithm>

// ---------------------------------------------------------------------------
static constexpr float kPi = 3.14159265358979323846f;

float LocalFft::WindowVal(FftWindow w, int i, int N)
{
    float t = static_cast<float>(i) / static_cast<float>(N - 1);
    switch (w) {
    case FftWindow::Hann:
        return 0.5f * (1.0f - cosf(2.0f * kPi * t));
    case FftWindow::Hamming:
        return 0.54f - 0.46f * cosf(2.0f * kPi * t);
    case FftWindow::Blackman:
        return 0.42f - 0.5f  * cosf(2.0f * kPi * t)
                     + 0.08f * cosf(4.0f * kPi * t);
    case FftWindow::Rectangular:
    default:
        return 1.0f;
    }
}

void LocalFft::Radix2FFT(std::vector<std::complex<float>>& x)
{
    const size_t N = x.size();
    if (N <= 1) return;

    // Bit-reversal permutation.
    for (size_t i = 1, j = 0; i < N; ++i) {
        size_t bit = N >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) std::swap(x[i], x[j]);
    }

    // Cooley-Tukey iterative FFT.
    for (size_t len = 2; len <= N; len <<= 1) {
        float ang = -2.0f * kPi / static_cast<float>(len);
        std::complex<float> wlen(cosf(ang), sinf(ang));
        for (size_t i = 0; i < N; i += len) {
            std::complex<float> w(1.0f, 0.0f);
            for (size_t j = 0; j < len / 2; ++j) {
                std::complex<float> u = x[i + j];
                std::complex<float> v = x[i + j + len / 2] * w;
                x[i + j]           = u + v;
                x[i + j + len / 2] = u - v;
                w *= wlen;
            }
        }
    }
}

std::vector<float> LocalFft::Compute(const uint16_t* samples, size_t nSamples,
                                     uint32_t sampleRateHz,
                                     const FftSettings& cfg,
                                     float& outFreqResHz)
{
    int fftSize = cfg.size;

    // Clamp fftSize to available samples.
    while (fftSize > 1 && static_cast<size_t>(fftSize) > nSamples)
        fftSize >>= 1;
    if (fftSize < 2) fftSize = 2;

    // Build complex input: apply window + normalize 12-bit ADC (0..4095) to [-1,1].
    std::vector<std::complex<float>> buf(static_cast<size_t>(fftSize));
    const uint16_t adcMask = 0x0FFF;
    const float    scale   = 1.0f / 2048.0f;   // 12-bit mid = 2048, range [-1..1]
    for (int i = 0; i < fftSize; ++i) {
        float raw = static_cast<float>(samples[i] & adcMask) - 2048.0f;
        float win = WindowVal(cfg.window, i, fftSize);
        buf[static_cast<size_t>(i)] = { raw * scale * win, 0.0f };
    }

    Radix2FFT(buf);

    outFreqResHz = static_cast<float>(sampleRateHz) / static_cast<float>(fftSize);

    // Compute magnitude for first half (real signal is symmetric).
    size_t half = static_cast<size_t>(fftSize) / 2;
    std::vector<float> mag(half);
    for (size_t i = 0; i < half; ++i) {
        float re = buf[i].real();
        float im = buf[i].imag();
        float m  = sqrtf(re * re + im * im);
        mag[i] = cfg.logScale ? (m > 1e-9f ? 20.0f * log10f(m) + 100.0f : 0.0f) : m;
    }
    return mag;
}

float LocalFft::PeakFrequencyHz(const std::vector<float>& mag, float freqResHz)
{
    if (mag.empty() || freqResHz <= 0.0f) return 0.0f;
    // Skip DC bin (i=0).
    size_t peakIdx = 1;
    for (size_t i = 2; i < mag.size(); ++i)
        if (mag[i] > mag[peakIdx]) peakIdx = i;

    // Parabolic interpolation for sub-bin precision.
    // If peak is not at edges, refine using neighbors.
    float peakFrac = 0.0f;
    if (peakIdx > 0 && peakIdx + 1 < mag.size()) {
        float alpha = mag[peakIdx - 1];
        float beta  = mag[peakIdx];
        float gamma = mag[peakIdx + 1];
        // Parabolic vertex offset: delta = (alpha - gamma) / (2*(alpha - 2*beta + gamma))
        float denom = alpha - 2.0f * beta + gamma;
        if (fabsf(denom) > 1e-9f)
            peakFrac = 0.5f * (alpha - gamma) / denom;
    }

    return freqResHz * (static_cast<float>(peakIdx) + peakFrac);
}
