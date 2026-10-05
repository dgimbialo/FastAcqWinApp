#include "PeakFinder.h"

#include <algorithm>
#include <cmath>

namespace dsp {

const char* PeakInterpName(PeakInterp t)
{
    switch (t) {
    case PeakInterp::None:        return "None";
    case PeakInterp::ParabolicDb: return "Parabolic (dB)";
    case PeakInterp::Jacobsen:    return "Jacobsen";
    case PeakInterp::Candan:      return "Candan";
    case PeakInterp::Quinn2:      return "Quinn (2nd)";
    default:                      return "?";
    }
}

static double Clamp05(double d)
{
    if (!(d == d)) return 0.0;   // NaN
    if (d > 0.5)  return 0.5;
    if (d < -0.5) return -0.5;
    return d;
}

static double QuinnTau(double x)
{
    const double s6 = std::sqrt(6.0);
    const double r23 = std::sqrt(2.0 / 3.0);
    return 0.25 * std::log(3.0 * x * x + 6.0 * x + 1.0)
         - (s6 / 24.0) * std::log((x + 1.0 - r23) / (x + 1.0 + r23));
}

double InterpolateOffset(PeakInterp method,
                         const std::vector<float>& db,
                         const std::vector<std::complex<float>>* cplx,
                         size_t k)
{
    const size_t n = db.size();
    if (k == 0 || k + 1 >= n) return 0.0;

    switch (method) {
    case PeakInterp::None:
        return 0.0;

    case PeakInterp::ParabolicDb: {
        const double a = db[k - 1], b = db[k], c = db[k + 1];
        const double d = a - 2.0 * b + c;
        if (std::fabs(d) < 1e-12) return 0.0;
        return Clamp05(0.5 * (a - c) / d);
    }

    case PeakInterp::Jacobsen:
    case PeakInterp::Candan: {
        if (!cplx || cplx->size() != n) return InterpolateOffset(PeakInterp::ParabolicDb, db, cplx, k);
        const std::complex<double> xm((*cplx)[k - 1]);
        const std::complex<double> x0((*cplx)[k]);
        const std::complex<double> xp((*cplx)[k + 1]);
        const std::complex<double> den = 2.0 * x0 - xm - xp;
        if (std::abs(den) < 1e-20) return 0.0;
        double d = -std::real((xp - xm) / den);
        if (method == PeakInterp::Candan) {
            // Bias correction: delta *= tan(pi/N) / (pi/N), N = full FFT length ~ 2*(n-1).
            const double N = 2.0 * static_cast<double>(n - 1);
            const double a = 3.14159265358979323846 / N;
            d *= std::tan(a) / a;
        }
        return Clamp05(d);
    }

    case PeakInterp::Quinn2: {
        if (!cplx || cplx->size() != n) return InterpolateOffset(PeakInterp::ParabolicDb, db, cplx, k);
        const std::complex<double> xm((*cplx)[k - 1]);
        const std::complex<double> x0((*cplx)[k]);
        const std::complex<double> xp((*cplx)[k + 1]);
        if (std::abs(x0) < 1e-20) return 0.0;
        const double ap = std::real(xp / x0);
        const double am = std::real(xm / x0);
        if (std::fabs(1.0 - ap) < 1e-12 || std::fabs(1.0 - am) < 1e-12) return 0.0;
        const double dp = -ap / (1.0 - ap);
        const double dm =  am / (1.0 - am);
        double d = 0.5 * (dp + dm) + QuinnTau(dp * dp) - QuinnTau(dm * dm);
        return Clamp05(d);
    }
    default:
        return 0.0;
    }
}

std::vector<Peak> FindPeaks(const std::vector<float>& db,
                            const std::vector<float>& thrDb,
                            const std::vector<float>& noiseDb,
                            const std::vector<std::complex<float>>* cplx,
                            double freqResHz,
                            const PeakParams& p)
{
    std::vector<Peak> out;
    const size_t n = db.size();
    if (n < 3) return out;

    size_t lo = p.minBin < 1 ? 1 : p.minBin;
    size_t hi = (p.maxBin == 0 || p.maxBin > n - 1) ? n - 1 : p.maxBin;
    if (lo >= hi) return out;

    struct Cand { size_t bin; float v; };
    std::vector<Cand> cands;
    for (size_t k = lo; k < hi; ++k) {
        const float v = db[k];
        if (!(v > db[k - 1] && v >= db[k + 1])) continue;
        const float thr = (k < thrDb.size()) ? thrDb[k] : 0.0f;
        if (!(v > thr)) continue;
        cands.push_back({ k, v });
    }
    std::sort(cands.begin(), cands.end(),
              [](const Cand& a, const Cand& b) { return a.v > b.v; });

    const double minDist = p.minDistBins < 0.0 ? 0.0 : p.minDistBins;
    std::vector<size_t> chosen;
    for (const auto& c : cands) {
        if (p.maxPeaks > 0 && static_cast<int>(out.size()) >= p.maxPeaks) break;
        bool tooClose = false;
        for (size_t b : chosen) {
            const double d = (b > c.bin) ? static_cast<double>(b - c.bin)
                                         : static_cast<double>(c.bin - b);
            if (d < minDist) { tooClose = true; break; }
        }
        if (tooClose) continue;
        chosen.push_back(c.bin);

        Peak pk;
        pk.bin     = c.bin;
        const double delta = InterpolateOffset(p.interp, db, cplx, c.bin);
        pk.fracBin = static_cast<double>(c.bin) + delta;
        pk.freqHz  = pk.fracBin * freqResHz;
        // Parabola vertex value (dB).
        const double a = db[c.bin - 1], b = db[c.bin], cc = db[c.bin + 1];
        pk.ampDb = static_cast<float>(b - 0.25 * (a - cc) * delta);
        const float noise = (c.bin < noiseDb.size()) ? noiseDb[c.bin] : 0.0f;
        pk.snrDb = pk.ampDb - noise;
        out.push_back(pk);
    }
    return out;
}

} // namespace dsp
