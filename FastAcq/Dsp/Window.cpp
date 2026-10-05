#include "Window.h"

#include <cmath>

namespace dsp {

static constexpr double kPi = 3.14159265358979323846;

const char* WindowName(WindowType t)
{
    switch (t) {
    case WindowType::Rectangular:     return "Rectangular";
    case WindowType::Hann:            return "Hann";
    case WindowType::Hamming:         return "Hamming";
    case WindowType::Blackman:        return "Blackman";
    case WindowType::BlackmanHarris4: return "Blackman-Harris 4";
    case WindowType::Kaiser:          return "Kaiser";
    case WindowType::FlatTop:         return "Flat-top";
    default:                          return "?";
    }
}

double BesselI0(double x)
{
    // Power series; converges quickly for the beta range used here (< 30).
    double sum = 1.0, term = 1.0;
    const double y = x * x / 4.0;
    for (int k = 1; k < 200; ++k) {
        term *= y / (static_cast<double>(k) * static_cast<double>(k));
        sum += term;
        if (term < sum * 1e-12) break;
    }
    return sum;
}

std::vector<float> MakeWindow(WindowType t, size_t n, float kaiserBeta)
{
    std::vector<float> w(n, 1.0f);
    if (n <= 1) return w;
    const double N1 = static_cast<double>(n - 1);

    auto cosineSum = [&](const double* a, int terms) {
        for (size_t i = 0; i < n; ++i) {
            const double x = 2.0 * kPi * static_cast<double>(i) / N1;
            double v = 0.0;
            for (int k = 0; k < terms; ++k)
                v += ((k & 1) ? -1.0 : 1.0) * a[k] * std::cos(k * x);
            w[i] = static_cast<float>(v);
        }
    };

    switch (t) {
    case WindowType::Rectangular:
        break;
    case WindowType::Hann: {
        const double a[] = { 0.5, 0.5 };
        cosineSum(a, 2);
        break;
    }
    case WindowType::Hamming: {
        const double a[] = { 0.54, 0.46 };
        cosineSum(a, 2);
        break;
    }
    case WindowType::Blackman: {
        const double a[] = { 0.42, 0.5, 0.08 };
        cosineSum(a, 3);
        break;
    }
    case WindowType::BlackmanHarris4: {
        const double a[] = { 0.35875, 0.48829, 0.14128, 0.01168 };
        cosineSum(a, 4);
        break;
    }
    case WindowType::FlatTop: {
        const double a[] = { 0.21557895, 0.41663158, 0.277263158, 0.083578947, 0.006947368 };
        cosineSum(a, 5);
        break;
    }
    case WindowType::Kaiser: {
        const double beta = kaiserBeta > 0.0f ? kaiserBeta : 0.0;
        const double denom = BesselI0(beta);
        for (size_t i = 0; i < n; ++i) {
            const double r = 2.0 * static_cast<double>(i) / N1 - 1.0;
            const double arg = 1.0 - r * r;
            w[i] = static_cast<float>(BesselI0(beta * std::sqrt(arg > 0.0 ? arg : 0.0)) / denom);
        }
        break;
    }
    default:
        break;
    }
    return w;
}

double WindowMainLobeBins(WindowType t, float kaiserBeta)
{
    switch (t) {
    case WindowType::Rectangular:     return 1.0;
    case WindowType::Hann:            return 2.0;
    case WindowType::Hamming:         return 2.0;
    case WindowType::Blackman:        return 3.0;
    case WindowType::BlackmanHarris4: return 4.0;
    case WindowType::FlatTop:         return 5.0;
    case WindowType::Kaiser: {
        const double b = kaiserBeta / kPi;
        return std::sqrt(1.0 + b * b);
    }
    default:                          return 2.0;
    }
}

WindowInfo AnalyzeWindow(const std::vector<float>& w, WindowType t, float kaiserBeta)
{
    WindowInfo info;
    if (w.empty()) return info;
    double s1 = 0.0, s2 = 0.0;
    for (float v : w) { s1 += v; s2 += static_cast<double>(v) * v; }
    const double n = static_cast<double>(w.size());
    info.coherentGain = s1 / n;
    info.enbwBins     = (s1 > 0.0) ? n * s2 / (s1 * s1) : 1.0;
    info.mainLobeBins = WindowMainLobeBins(t, kaiserBeta);
    return info;
}

} // namespace dsp
