#include "Decimator.h"
#include "Window.h"

#include <algorithm>
#include <cmath>

namespace dsp {

static constexpr double kPi = 3.14159265358979323846;

FirDecimator::FirDecimator(int factor, double attenuationDb)
    : m_factor(factor < 1 ? 1 : factor)
{
    if (m_factor == 1) {
        m_h.assign(1, 1.0f);
        m_cutoff = 0.5;
        return;
    }
    const double D        = static_cast<double>(m_factor);
    const double fNyqNew  = 0.5 / D;               // new Nyquist, cycles/sample (input rate)
    const double fPass    = 0.8 * fNyqNew;
    const double fStop    = 1.0 * fNyqNew;
    const double dF       = fStop - fPass;          // transition width
    m_cutoff              = 0.5 * (fPass + fStop);

    // Kaiser design (Oppenheim & Schafer).
    const double A    = attenuationDb;
    double beta;
    if (A > 50.0)      beta = 0.1102 * (A - 8.7);
    else if (A >= 21.) beta = 0.5842 * std::pow(A - 21.0, 0.4) + 0.07886 * (A - 21.0);
    else               beta = 0.0;
    size_t taps = static_cast<size_t>(std::ceil((A - 8.0) / (2.285 * 2.0 * kPi * dF))) + 1;
    if (taps < 15) taps = 15;
    if (taps > 32769) taps = 32769;
    if ((taps & 1) == 0) ++taps;                     // odd length -> symmetric, integer delay

    std::vector<float> w = MakeWindow(WindowType::Kaiser, taps, static_cast<float>(beta));
    m_h.resize(taps);
    const double c = static_cast<double>(taps - 1) / 2.0;
    double sum = 0.0;
    for (size_t k = 0; k < taps; ++k) {
        const double t = static_cast<double>(k) - c;
        double sinc = (std::fabs(t) < 1e-12) ? 1.0
                    : std::sin(2.0 * kPi * m_cutoff * t) / (2.0 * kPi * m_cutoff * t);
        double v = 2.0 * m_cutoff * sinc * w[k];
        m_h[k] = static_cast<float>(v);
        sum += v;
    }
    if (sum != 0.0)
        for (auto& v : m_h) v = static_cast<float>(v / sum);  // unity DC gain
}

void FirDecimator::Process(const float* in, size_t n, std::vector<float>& out) const
{
    const size_t nOut = n / static_cast<size_t>(m_factor);
    out.resize(nOut);
    if (nOut == 0) return;

    if (m_factor == 1) {
        std::copy(in, in + n, out.begin());
        return;
    }

    // h is symmetric, so y[m] = sum_i h[i] * x[center - c + i]: a forward
    // dot product that the compiler can vectorize.
    const long   taps = static_cast<long>(m_h.size());
    const long   c    = (taps - 1) / 2;
    const float* h    = m_h.data();
    const long   N    = static_cast<long>(n);

    for (size_t m = 0; m < nOut; ++m) {
        const long x0 = static_cast<long>(m) * m_factor - c;   // input index of h[0]
        long iStart = (x0 < 0) ? -x0 : 0;
        long iEnd   = taps;                                      // exclusive
        if (x0 + iEnd > N) iEnd = N - x0;
        float acc = 0.0f;
        if (iEnd > iStart) {
            const float* xp = in + (x0 + iStart);
            const float* hp = h + iStart;
            const long   cnt = iEnd - iStart;
            float a0 = 0.0f, a1 = 0.0f, a2 = 0.0f, a3 = 0.0f;
            long i = 0;
            for (; i + 4 <= cnt; i += 4) {
                a0 += hp[i]     * xp[i];
                a1 += hp[i + 1] * xp[i + 1];
                a2 += hp[i + 2] * xp[i + 2];
                a3 += hp[i + 3] * xp[i + 3];
            }
            for (; i < cnt; ++i) a0 += hp[i] * xp[i];
            acc = (a0 + a1) + (a2 + a3);
        }
        out[m] = acc;
    }
}

} // namespace dsp
