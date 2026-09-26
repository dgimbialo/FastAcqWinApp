#include "pch.h"
#include "LocalFft.h"

#define _USE_MATH_DEFINES
#include <cmath>
#include <algorithm>

// ---------------------------------------------------------------------------
static constexpr float  kPi  = 3.14159265358979323846f;
static constexpr double kPiD = 3.14159265358979323846;

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

// Iterative radix-2 Cooley-Tukey FFT. Twiddles are computed directly in
// double for every stage: the former recurrence w *= wlen in float drifted by
// ~1e-3 over thousands of steps and raised the spectral noise floor.
template <typename T>
static void Radix2FftT(std::vector<std::complex<T>>& x)
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

    std::vector<std::complex<T>> tw;
    for (size_t len = 2; len <= N; len <<= 1) {
        const size_t halfLen = len / 2;
        tw.resize(halfLen);
        for (size_t j = 0; j < halfLen; ++j) {
            const double ang = -2.0 * kPiD * static_cast<double>(j) / static_cast<double>(len);
            tw[j] = std::complex<T>(static_cast<T>(cos(ang)), static_cast<T>(sin(ang)));
        }
        for (size_t i = 0; i < N; i += len) {
            for (size_t j = 0; j < halfLen; ++j) {
                std::complex<T> u = x[i + j];
                std::complex<T> v = x[i + j + halfLen] * tw[j];
                x[i + j]           = u + v;
                x[i + j + halfLen] = u - v;
            }
        }
    }
}

void LocalFft::Radix2FFT(std::vector<std::complex<float>>& x)
{
    Radix2FftT(x);
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

double LocalFft::EstimateToneHz(const uint16_t* samples, size_t nSamples, uint32_t sampleRateHz)
{
    // Dedicated estimator, independent of the display FFT settings (size,
    // window, dB scale): largest power-of-two block of the segment (<= 65536),
    // periodic Hann window, double precision, LINEAR magnitudes, then the
    // Hann ratio estimator delta = (2a - 1) / (a + 1), a = larger neighbour /
    // peak. Verified on captured 1 MHz data: +-0.4 Hz vs a sine fit, where the
    // former log-parabolic interpolation was biased by 20-50 Hz.
    if (samples == nullptr || sampleRateHz == 0 || nSamples < 64) return 0.0;
    size_t N = 1;
    while (N * 2 <= nSamples && N * 2 <= 65536) N *= 2;

    double mean = 0.0;
    for (size_t i = 0; i < N; ++i) mean += static_cast<double>(samples[i] & 0x0FFF);
    mean /= static_cast<double>(N);

    std::vector<std::complex<double>> buf(N);
    for (size_t i = 0; i < N; ++i) {
        const double w = 0.5 * (1.0 - cos(2.0 * kPiD * static_cast<double>(i) / static_cast<double>(N)));
        buf[i] = { (static_cast<double>(samples[i] & 0x0FFF) - mean) * w, 0.0 };
    }
    Radix2FftT(buf);

    const size_t half = N / 2;
    std::vector<double> mag(half);
    for (size_t i = 0; i < half; ++i) mag[i] = std::abs(buf[i]);

    // Skip the DC main lobe (Hann leaks residual offset into bins 1..3).
    const size_t first = 4;
    size_t k = first;
    for (size_t i = first + 1; i + 1 < half; ++i)
        if (mag[i] > mag[k]) k = i;
    if (k + 1 >= half || mag[k] <= 0.0)
        return static_cast<double>(k) * sampleRateHz / static_cast<double>(N);

    const double l = mag[k - 1], r = mag[k + 1];
    const double side = (r > l) ? 1.0 : -1.0;
    const double a = ((r > l) ? r : l) / mag[k];
    double d = (2.0 * a - 1.0) / (a + 1.0);
    if (d < 0.0) d = 0.0;
    if (d > 0.5) d = 0.5;
    return (static_cast<double>(k) + side * d) * sampleRateHz / static_cast<double>(N);
}
