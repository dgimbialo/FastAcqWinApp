#include "ToneEstimator.h"

#include <cmath>
#include <complex>
#include <vector>

namespace dsp {

namespace {

constexpr double kPiD = 3.14159265358979323846;

// Iterative radix-2 complex FFT in double precision (twiddles recomputed per
// stage; the estimator runs once per frame, so speed is not critical).
void Fft(std::vector<std::complex<double>>& a)
{
    const size_t n = a.size();
    for (size_t i = 1, j = 0; i < n; ++i) {
        size_t bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) std::swap(a[i], a[j]);
    }
    for (size_t len = 2; len <= n; len <<= 1) {
        const double ang = -2.0 * kPiD / static_cast<double>(len);
        const std::complex<double> wl(std::cos(ang), std::sin(ang));
        for (size_t i = 0; i < n; i += len) {
            std::complex<double> w(1.0, 0.0);
            for (size_t k = 0; k < len / 2; ++k) {
                const std::complex<double> u = a[i + k];
                const std::complex<double> v = a[i + k + len / 2] * w;
                a[i + k]           = u + v;
                a[i + k + len / 2] = u - v;
                w *= wl;
            }
        }
    }
}

template <class Get>
ToneEstimate Estimate(size_t nSamples, double fs, Get get)
{
    ToneEstimate e;
    if (fs <= 0.0 || nSamples < 64) return e;
    size_t N = 1;
    while (N * 2 <= nSamples && N * 2 <= 65536) N *= 2;

    double mean = 0.0;
    for (size_t i = 0; i < N; ++i) mean += get(i);
    mean /= static_cast<double>(N);

    std::vector<std::complex<double>> buf(N);
    double wsum = 0.0;
    for (size_t i = 0; i < N; ++i) {
        const double w = 0.5 * (1.0 - std::cos(2.0 * kPiD * static_cast<double>(i) / static_cast<double>(N)));
        wsum += w;
        buf[i] = { (get(i) - mean) * w, 0.0 };
    }
    Fft(buf);

    const size_t half = N / 2;
    std::vector<double> mag(half);
    for (size_t i = 0; i < half; ++i) mag[i] = std::abs(buf[i]);

    // Skip the DC main lobe (Hann leaks residual offset into bins 1..3).
    const size_t first = 4;
    if (first + 1 >= half) return e;
    size_t k = first;
    for (size_t i = first + 1; i + 1 < half; ++i)
        if (mag[i] > mag[k]) k = i;
    e.nUsed = N;
    e.valid = true;
    // Amplitude: coherent gain of the window = wsum / N; |X| = A * wsum / 2.
    e.ampFs = (wsum > 0.0) ? mag[k] * 2.0 / wsum : 0.0;
    if (k + 1 >= half || mag[k] <= 0.0) {
        e.freqHz = static_cast<double>(k) * fs / static_cast<double>(N);
        return e;
    }
    const double l = mag[k - 1], r = mag[k + 1];
    const double side = (r > l) ? 1.0 : -1.0;
    const double a = ((r > l) ? r : l) / mag[k];
    double d = (2.0 * a - 1.0) / (a + 1.0);
    if (d < 0.0) d = 0.0;
    if (d > 0.5) d = 0.5;
    e.freqHz = (static_cast<double>(k) + side * d) * fs / static_cast<double>(N);
    return e;
}

} // namespace

ToneEstimate EstimateTone(const uint16_t* samples, size_t nSamples, double sampleRateHz)
{
    if (!samples) return {};
    ToneEstimate e = Estimate(nSamples, sampleRateHz,
                              [&](size_t i) { return static_cast<double>(samples[i] & 0x0FFFu); });
    e.ampFs /= 2048.0;   // 12-bit codes -> fraction of full scale
    return e;
}

ToneEstimate EstimateTone(const float* samples, size_t nSamples, double sampleRateHz)
{
    if (!samples) return {};
    return Estimate(nSamples, sampleRateHz, [&](size_t i) { return static_cast<double>(samples[i]); });
}

} // namespace dsp
