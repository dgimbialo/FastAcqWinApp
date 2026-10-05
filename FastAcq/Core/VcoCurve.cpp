#include "VcoCurve.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace core {

VcoCurve VcoCurve::Hmc431Typical()
{
    // Approximate typical curve of the HMC431 (sensitivity falls from about
    // 90 MHz/V at low Vtune to about 40 MHz/V at 10 V).
    static const double kPts[][2] = {
        { 0.0, 5.50e9 }, { 1.0, 5.59e9 }, { 2.0, 5.67e9 }, { 3.0, 5.74e9 }, { 4.0, 5.80e9 },
        { 5.0, 5.86e9 }, { 6.0, 5.91e9 }, { 7.0, 5.96e9 }, { 8.0, 6.01e9 }, { 9.0, 6.06e9 },
        { 10.0, 6.10e9 },
    };
    VcoCurve c;
    for (const auto& p : kPts) c.m_pts.push_back({ p[0], p[1] });
    return c;
}

void VcoCurve::SetPoints(std::vector<VcoPoint> pts)
{
    std::sort(pts.begin(), pts.end(), [](const VcoPoint& a, const VcoPoint& b) { return a.vtuneV < b.vtuneV; });
    // Drop duplicate voltages (keep the first).
    std::vector<VcoPoint> out;
    for (const auto& p : pts) {
        if (!out.empty() && !(p.vtuneV > out.back().vtuneV)) continue;
        if (!(p.freqHz > 0.0)) continue;
        out.push_back(p);
    }
    m_pts = std::move(out);
}

bool VcoCurve::Parse(const std::string& text)
{
    std::vector<VcoPoint> pts;
    std::string tok;
    auto flush = [&]() {
        if (tok.empty()) return true;
        size_t sep = tok.find_first_of(":=");
        if (sep == std::string::npos) return false;
        char* e1 = nullptr; char* e2 = nullptr;
        const double v = std::strtod(tok.c_str(), &e1);
        const double g = std::strtod(tok.c_str() + sep + 1, &e2);
        if (e1 != tok.c_str() + sep || *e2 != '\0') return false;
        if (!(g > 0.0)) return false;
        pts.push_back({ v, g * 1e9 });
        tok.clear();
        return true;
    };
    for (char ch : text) {
        if (ch == ',' || ch == ';' || ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') {
            if (!flush()) return false;
        } else {
            tok.push_back(ch);
        }
    }
    if (!flush()) return false;
    if (pts.size() < 2) return false;
    SetPoints(std::move(pts));
    return Valid();
}

std::string VcoCurve::Format() const
{
    std::string s;
    char buf[64];
    for (size_t i = 0; i < m_pts.size(); ++i) {
        std::snprintf(buf, sizeof(buf), "%s%.4g:%.4f", i ? "," : "", m_pts[i].vtuneV, m_pts[i].freqHz / 1e9);
        s += buf;
    }
    return s;
}

double VcoCurve::FreqHz(double v) const
{
    if (m_pts.empty()) return 0.0;
    if (m_pts.size() == 1) return m_pts[0].freqHz;
    size_t i = 1;
    while (i + 1 < m_pts.size() && v > m_pts[i].vtuneV) ++i;
    const VcoPoint& a = m_pts[i - 1];
    const VcoPoint& b = m_pts[i];
    const double t = (v - a.vtuneV) / (b.vtuneV - a.vtuneV);
    return a.freqHz + t * (b.freqHz - a.freqHz);
}

double VcoCurve::SensitivityHzPerV(double v) const
{
    if (m_pts.size() < 2) return 0.0;
    size_t i = 1;
    while (i + 1 < m_pts.size() && v > m_pts[i].vtuneV) ++i;
    const VcoPoint& a = m_pts[i - 1];
    const VcoPoint& b = m_pts[i];
    return (b.freqHz - a.freqHz) / (b.vtuneV - a.vtuneV);
}

VcoSweep ComputeVcoSweep(const VcoCurve& curve, double vLowV, double vHighV)
{
    VcoSweep s;
    s.vLowV = vLowV; s.vHighV = vHighV;
    if (!curve.Valid() || !(vHighV > vLowV)) return s;
    s.valid      = true;
    s.outOfTable = (vLowV < curve.VminV() - 1e-9) || (vHighV > curve.VmaxV() + 1e-9);
    s.fStartHz   = curve.FreqHz(vLowV);
    s.fStopHz    = curve.FreqHz(vHighV);
    s.f0Hz       = 0.5 * (s.fStartHz + s.fStopHz);
    s.bandwidthHz = std::fabs(s.fStopHz - s.fStartHz);

    // Sensitivity range and deviation from a linear sweep, sampled along the
    // swept segment including every table knot inside it.
    std::vector<double> vs = { vLowV, vHighV };
    for (const auto& p : curve.Points())
        if (p.vtuneV > vLowV && p.vtuneV < vHighV) vs.push_back(p.vtuneV);
    std::sort(vs.begin(), vs.end());
    double smin = 1e300, smax = -1e300, dev = 0.0;
    for (size_t i = 0; i < vs.size(); ++i) {
        const double v = vs[i];
        if (i + 1 < vs.size()) {
            const double k = (curve.FreqHz(vs[i + 1]) - curve.FreqHz(v)) / (vs[i + 1] - v);
            smin = (std::min)(smin, k); smax = (std::max)(smax, k);
        }
        const double chord = s.fStartHz + (s.fStopHz - s.fStartHz) * (v - vLowV) / (vHighV - vLowV);
        dev = (std::max)(dev, std::fabs(curve.FreqHz(v) - chord));
    }
    s.sensMinHzPerV = (smin < 1e300) ? smin : 0.0;
    s.sensMaxHzPerV = (smax > -1e300) ? smax : 0.0;
    s.nonlinearityPct = (s.bandwidthHz > 0.0) ? 100.0 * dev / s.bandwidthHz : 0.0;
    return s;
}

} // namespace core
