#pragma once
//
// Decimator.h -- FIR anti-alias low-pass + integer decimation (Kaiser design).
//

#include <cstddef>
#include <vector>

namespace dsp {

class FirDecimator {
public:
    // factor >= 1 (1 = pass-through). attenuationDb sets the Kaiser design target.
    explicit FirDecimator(int factor, double attenuationDb = 80.0);

    int    Factor() const { return m_factor; }
    size_t Taps()   const { return m_h.size(); }
    const std::vector<float>& Coefficients() const { return m_h; }

    // Produces floor(n / factor) output samples. The filter is centered; input
    // outside [0, n) is treated as zero (edge effects are confined to taps/2
    // samples at each end, which the ramp guard interval removes).
    void Process(const float* in, size_t n, std::vector<float>& out) const;

    // Normalized cut-off (cycles/sample of the INPUT rate).
    double Cutoff() const { return m_cutoff; }

private:
    int                m_factor;
    double             m_cutoff{0.5};
    std::vector<float> m_h;
};

} // namespace dsp
