#include "Cfar.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace dsp {

const char* DetectorName(DetectorType t)
{
    switch (t) {
    case DetectorType::FixedAboveNoise: return "Noise floor + X dB";
    case DetectorType::CaCfar:          return "CA-CFAR";
    case DetectorType::OsCfar:          return "OS-CFAR";
    case DetectorType::GoCfar:          return "GO-CFAR (greatest of)";
    case DetectorType::SoCfar:          return "SO-CFAR (smallest of)";
    default:                            return "?";
    }
}

float MedianOf(const std::vector<float>& v, size_t from, size_t to)
{
    if (to > v.size()) to = v.size();
    if (from >= to) return 0.0f;
    std::vector<float> tmp(v.begin() + static_cast<std::ptrdiff_t>(from),
                           v.begin() + static_cast<std::ptrdiff_t>(to));
    size_t mid = tmp.size() / 2;
    std::nth_element(tmp.begin(), tmp.begin() + static_cast<std::ptrdiff_t>(mid), tmp.end());
    return tmp[mid];
}

double CaCfarAlpha(int nTrainTotal, double pfa)
{
    if (nTrainTotal < 1) nTrainTotal = 1;
    if (pfa <= 0.0) pfa = 1e-12;
    if (pfa >= 1.0) pfa = 0.999;
    const double N = static_cast<double>(nTrainTotal);
    return N * (std::pow(pfa, -1.0 / N) - 1.0);
}

double OsCfarAlpha(int nTrainTotal, int rank, double pfa)
{
    // Pfa(alpha) = prod_{i=0}^{k-1} (N - i) / (N - i + alpha); monotonic decreasing in alpha.
    if (nTrainTotal < 1) nTrainTotal = 1;
    if (rank < 1) rank = 1;
    if (rank > nTrainTotal) rank = nTrainTotal;
    if (pfa <= 0.0) pfa = 1e-12;
    if (pfa >= 1.0) pfa = 0.999;
    const double N = static_cast<double>(nTrainTotal);
    auto pfaOf = [&](double a) {
        double p = 1.0;
        for (int i = 0; i < rank; ++i) p *= (N - i) / (N - i + a);
        return p;
    };
    double lo = 0.0, hi = 1.0;
    while (pfaOf(hi) > pfa && hi < 1e9) hi *= 2.0;
    for (int it = 0; it < 200; ++it) {
        double mid = 0.5 * (lo + hi);
        if (pfaOf(mid) > pfa) lo = mid; else hi = mid;
        if (hi - lo < 1e-9 * hi) break;
    }
    return 0.5 * (lo + hi);
}

namespace {

// Pfa of GO / SO cell averaging with n cells per side and threshold
// T = a * (sum of the chosen side), unit-mean exponential noise (Hansen 1980):
//   Pfa_SO = 2 * sum_{k=0}^{n-1} C(n-1+k, k) (2 + a)^-(n+k)
//   Pfa_GO = 2 (1 + a)^-n - Pfa_SO
double LogBinom(int n, int k)
{
    return std::lgamma(n + 1.0) - std::lgamma(k + 1.0) - std::lgamma(n - k + 1.0);
}
double PfaSo(int n, double a)
{
    double s = 0.0;
    for (int k = 0; k < n; ++k)
        s += std::exp(LogBinom(n - 1 + k, k) - (n + k) * std::log(2.0 + a));
    return 2.0 * s;
}
double PfaGo(int n, double a) { return 2.0 * std::pow(1.0 + a, -n) - PfaSo(n, a); }

double SolveAlphaSum(int n, double pfa, double (*pfaOf)(int, double))
{
    if (n < 1) n = 1;
    if (pfa <= 0.0) pfa = 1e-12;
    if (pfa >= 1.0) pfa = 0.999;
    double lo = 0.0, hi = 1.0;
    while (pfaOf(n, hi) > pfa && hi < 1e9) hi *= 2.0;
    for (int it = 0; it < 200; ++it) {
        const double mid = 0.5 * (lo + hi);
        if (pfaOf(n, mid) > pfa) lo = mid; else hi = mid;
        if (hi - lo < 1e-9 * hi) break;
    }
    return 0.5 * (lo + hi);
}

} // namespace

// Mean convention: alpha_mean = n * alpha_sum.
double GoCfarAlpha(int nPerSide, double pfa) { return nPerSide * SolveAlphaSum(nPerSide, pfa, &PfaGo); }
double SoCfarAlpha(int nPerSide, double pfa) { return nPerSide * SolveAlphaSum(nPerSide, pfa, &PfaSo); }

void RunDetector(const std::vector<float>& powerLin, size_t fromBin, size_t toBin,
                 const DetectorParams& p, DetectorOutput& out)
{
    const size_t n = powerLin.size();
    if (toBin > n) toBin = n;
    if (fromBin > toBin) fromBin = toBin;

    const float inf = std::numeric_limits<float>::infinity();
    out.thresholdDb.assign(n, inf);
    out.noiseDb.assign(n, -300.0f);
    out.globalNoiseDb = PowerToDb(MedianOf(powerLin, fromBin, toBin));
    if (fromBin >= toBin) return;

    switch (p.type) {
    case DetectorType::FixedAboveNoise: {
        const float thr = out.globalNoiseDb + p.thresholdDb;
        for (size_t k = fromBin; k < toBin; ++k) {
            out.noiseDb[k]     = out.globalNoiseDb;
            out.thresholdDb[k] = thr;
        }
        break;
    }
    case DetectorType::CaCfar: {
        const int G = p.guardCells < 0 ? 0 : p.guardCells;
        const int T = p.trainCells < 1 ? 1 : p.trainCells;
        // Prefix sums over the evaluated band for O(1) window means.
        std::vector<double> pre(n + 1, 0.0);
        for (size_t k = 0; k < n; ++k) pre[k + 1] = pre[k] + powerLin[k];
        auto sumRange = [&](long a, long b) -> double {   // [a, b) clipped to band
            if (a < static_cast<long>(fromBin)) a = static_cast<long>(fromBin);
            if (b > static_cast<long>(toBin))   b = static_cast<long>(toBin);
            if (a >= b) return 0.0;
            return pre[static_cast<size_t>(b)] - pre[static_cast<size_t>(a)];
        };
        auto countRange = [&](long a, long b) -> int {
            if (a < static_cast<long>(fromBin)) a = static_cast<long>(fromBin);
            if (b > static_cast<long>(toBin))   b = static_cast<long>(toBin);
            return (a >= b) ? 0 : static_cast<int>(b - a);
        };
        for (size_t k = fromBin; k < toBin; ++k) {
            const long kk = static_cast<long>(k);
            double s = sumRange(kk - G - T, kk - G) + sumRange(kk + G + 1, kk + G + 1 + T);
            int    c = countRange(kk - G - T, kk - G) + countRange(kk + G + 1, kk + G + 1 + T);
            if (c < 1) { c = 1; s = powerLin[k]; }
            const double noise = s / c;
            const double alpha = CaCfarAlpha(c, p.pfa);
            out.noiseDb[k]     = PowerToDb(static_cast<float>(noise));
            out.thresholdDb[k] = PowerToDb(static_cast<float>(alpha * noise));
        }
        break;
    }
    case DetectorType::GoCfar:
    case DetectorType::SoCfar: {
        const bool greatest = (p.type == DetectorType::GoCfar);
        const int G = p.guardCells < 0 ? 0 : p.guardCells;
        const int T = p.trainCells < 1 ? 1 : p.trainCells;
        std::vector<double> pre(n + 1, 0.0);
        for (size_t k = 0; k < n; ++k) pre[k + 1] = pre[k] + powerLin[k];
        auto meanRange = [&](long a, long b, int& cnt) -> double {
            if (a < static_cast<long>(fromBin)) a = static_cast<long>(fromBin);
            if (b > static_cast<long>(toBin))   b = static_cast<long>(toBin);
            cnt = (a >= b) ? 0 : static_cast<int>(b - a);
            return cnt ? (pre[static_cast<size_t>(b)] - pre[static_cast<size_t>(a)]) / cnt : 0.0;
        };
        for (size_t k = fromBin; k < toBin; ++k) {
            const long kk = static_cast<long>(k);
            int c1 = 0, c2 = 0;
            const double m1 = meanRange(kk - G - T, kk - G, c1);
            const double m2 = meanRange(kk + G + 1, kk + G + 1 + T, c2);
            double noise; int cells;
            if (c1 && c2) { noise = greatest ? (std::max)(m1, m2) : (std::min)(m1, m2); cells = (std::min)(c1, c2); }
            else if (c1)  { noise = m1; cells = c1; }
            else if (c2)  { noise = m2; cells = c2; }
            else          { noise = powerLin[k]; cells = 1; }
            const double alpha = (c1 && c2) ? (greatest ? GoCfarAlpha(cells, p.pfa) : SoCfarAlpha(cells, p.pfa))
                                            : CaCfarAlpha(cells, p.pfa);
            out.noiseDb[k]     = PowerToDb(static_cast<float>(noise));
            out.thresholdDb[k] = PowerToDb(static_cast<float>(alpha * noise));
        }
        break;
    }
    case DetectorType::OsCfar: {
        const int G = p.guardCells < 0 ? 0 : p.guardCells;
        const int T = p.trainCells < 1 ? 1 : p.trainCells;
        std::vector<float> win;
        win.reserve(static_cast<size_t>(2 * T));
        for (size_t k = fromBin; k < toBin; ++k) {
            win.clear();
            const long kk = static_cast<long>(k);
            for (long j = kk - G - T; j < kk - G; ++j)
                if (j >= static_cast<long>(fromBin) && j < static_cast<long>(toBin))
                    win.push_back(powerLin[static_cast<size_t>(j)]);
            for (long j = kk + G + 1; j < kk + G + 1 + T; ++j)
                if (j >= static_cast<long>(fromBin) && j < static_cast<long>(toBin))
                    win.push_back(powerLin[static_cast<size_t>(j)]);
            if (win.empty()) win.push_back(powerLin[k]);
            int rank = static_cast<int>(std::lround(p.osRankFrac * static_cast<float>(win.size())));
            if (rank < 1) rank = 1;
            if (rank > static_cast<int>(win.size())) rank = static_cast<int>(win.size());
            std::nth_element(win.begin(), win.begin() + (rank - 1), win.end());
            const double noise = win[static_cast<size_t>(rank - 1)];
            const double alpha = OsCfarAlpha(static_cast<int>(win.size()), rank, p.pfa);
            out.noiseDb[k]     = PowerToDb(static_cast<float>(noise));
            out.thresholdDb[k] = PowerToDb(static_cast<float>(alpha * noise));
        }
        break;
    }
    default:
        break;
    }
}

} // namespace dsp
