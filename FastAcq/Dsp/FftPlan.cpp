#include "FftPlan.h"

#include <cmath>
#include <stdexcept>

namespace dsp {

static constexpr double kPi = 3.14159265358979323846;

size_t FftPlan::NextPow2(size_t n)
{
    size_t p = 1;
    while (p < n) p <<= 1;
    return p;
}

FftPlan::FftPlan(size_t n)
    : m_n(n)
{
    if (!IsPow2(n)) throw std::invalid_argument("FftPlan: size must be a power of two");

    m_tw.resize(n / 2 > 0 ? n / 2 : 1);
    for (size_t k = 0; k < n / 2; ++k) {
        double a = -2.0 * kPi * static_cast<double>(k) / static_cast<double>(n);
        m_tw[k] = std::complex<float>(static_cast<float>(std::cos(a)),
                                      static_cast<float>(std::sin(a)));
    }

    m_rev.resize(n);
    unsigned bits = 0;
    while ((size_t{1} << bits) < n) ++bits;
    for (size_t i = 0; i < n; ++i) {
        uint32_t r = 0;
        for (unsigned b = 0; b < bits; ++b)
            if (i & (size_t{1} << b)) r |= 1u << (bits - 1 - b);
        m_rev[i] = r;
    }
}

void FftPlan::Transform(std::complex<float>* x, bool inverse) const
{
    const size_t n = m_n;
    if (n <= 1) return;

    for (size_t i = 0; i < n; ++i) {
        size_t j = m_rev[i];
        if (i < j) std::swap(x[i], x[j]);
    }

    for (size_t len = 2; len <= n; len <<= 1) {
        const size_t half   = len >> 1;
        const size_t stride = n / len;
        for (size_t i = 0; i < n; i += len) {
            for (size_t j = 0; j < half; ++j) {
                std::complex<float> w = m_tw[j * stride];
                if (inverse) w = std::conj(w);
                std::complex<float> u = x[i + j];
                std::complex<float> v = x[i + j + half] * w;
                x[i + j]        = u + v;
                x[i + j + half] = u - v;
            }
        }
    }

    if (inverse) {
        const float s = 1.0f / static_cast<float>(n);
        for (size_t i = 0; i < n; ++i) x[i] *= s;
    }
}

void FftPlan::Forward(std::complex<float>* x) const { Transform(x, false); }
void FftPlan::Inverse(std::complex<float>* x) const { Transform(x, true); }

// ---------------------------------------------------------------------------
RealFft::RealFft(size_t n)
    : m_n(n), m_half(n >= 4 ? n / 2 : 2)
{
    if (!FftPlan::IsPow2(n) || n < 4)
        throw std::invalid_argument("RealFft: size must be a power of two >= 4");
    m_twN.resize(n / 2 + 1);
    for (size_t k = 0; k <= n / 2; ++k) {
        double a = -2.0 * kPi * static_cast<double>(k) / static_cast<double>(n);
        m_twN[k] = std::complex<float>(static_cast<float>(std::cos(a)),
                                       static_cast<float>(std::sin(a)));
    }
    m_tmp.resize(n / 2);
}

void RealFft::Forward(const float* in, std::complex<float>* out) const
{
    const size_t n    = m_n;
    const size_t half = n / 2;

    // Pack even samples into real, odd samples into imaginary part.
    for (size_t k = 0; k < half; ++k)
        m_tmp[k] = std::complex<float>(in[2 * k], in[2 * k + 1]);

    m_half.Forward(m_tmp.data());

    // Split: X[k] = Fe[k] + W^k Fo[k], with Z[half] == Z[0].
    for (size_t k = 0; k <= half; ++k) {
        const std::complex<float> zk  = (k == half) ? m_tmp[0] : m_tmp[k];
        const std::complex<float> znk = (k == 0 || k == half) ? m_tmp[0] : m_tmp[half - k];
        const std::complex<float> zc  = std::conj(znk);
        const std::complex<float> fe  = 0.5f * (zk + zc);
        const std::complex<float> fo  = std::complex<float>(0.0f, -0.5f) * (zk - zc);
        out[k] = fe + m_twN[k] * fo;
    }
}

} // namespace dsp
