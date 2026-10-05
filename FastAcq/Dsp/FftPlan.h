#pragma once
//
// FftPlan.h -- radix-2 complex FFT with cached twiddles, plus a real-input
// FFT built on an N/2-point complex transform. Portable, no dependencies.
//

#include <complex>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace dsp {

class FftPlan {
public:
    // n must be a power of two (>= 1).
    explicit FftPlan(size_t n);

    size_t Size() const { return m_n; }

    // In-place forward transform, unnormalized: X[k] = sum x[j] e^{-2*pi*i*jk/n}.
    void Forward(std::complex<float>* x) const;
    // In-place inverse transform, normalized by 1/n.
    void Inverse(std::complex<float>* x) const;

    static bool   IsPow2(size_t n) { return n != 0 && (n & (n - 1)) == 0; }
    static size_t NextPow2(size_t n);

private:
    void Transform(std::complex<float>* x, bool inverse) const;

    size_t                           m_n;
    std::vector<std::complex<float>> m_tw;   // e^{-2*pi*i*k/n}, k < n/2
    std::vector<uint32_t>            m_rev;  // bit-reversal permutation
};

// Real-input FFT of n samples (n power of two, >= 4) -> n/2+1 complex bins.
class RealFft {
public:
    explicit RealFft(size_t n);

    size_t Size() const { return m_n; }
    size_t Bins() const { return m_n / 2 + 1; }

    // out must hold n/2+1 entries. Unnormalized (same scaling as FftPlan::Forward).
    void Forward(const float* in, std::complex<float>* out) const;

private:
    size_t                                   m_n;
    FftPlan                                  m_half;
    std::vector<std::complex<float>>         m_twN;   // e^{-2*pi*i*k/n}, k <= n/2
    mutable std::vector<std::complex<float>> m_tmp;   // scratch (single-thread use)
};

} // namespace dsp
