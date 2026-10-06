#include "VcoCurve.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace core {

VcoCurve VcoCurve::Hmc431Typical()
{
    // Read from the datasheet plots "Frequency vs. Tuning Voltage, T = 25 C"
    // and "Sensitivity vs. Tuning Voltage, Vcc = +3 V" (HMC431LP4): the
    // oscillator starts at about 5.09 GHz with Vtune = 0 V and reaches about
    // 6.29 GHz at 10 V; the sensitivity falls from ~450 MHz/V near 0 V to
    // ~60 MHz/V at 10 V, so the curve is steep below 2 V and gentle above.
    // The guaranteed band in the specification table (5.5..6.1 GHz) is the
    // part of this curve from about 1.6 V to about 7.4 V.
    static const double kPts[][2] = {
        { 0.0, 5.085e9 }, { 1.0, 5.410e9 }, { 2.0, 5.585e9 }, { 3.0, 5.715e9 }, { 4.0, 5.815e9 },
        { 5.0, 5.905e9 }, { 6.0, 5.990e9 }, { 7.0, 6.070e9 }, { 8.0, 6.150e9 }, { 9.0, 6.220e9 },
        { 10.0, 6.285e9 },
    };
    VcoCurve c;
    for (const auto& p : kPts) c.m_pts.push_back({ p[0], p[1] });
    return c;
}

const char* VcoCurve::LegacyTypicalText()
{
    return "0:5.5000,1:5.5900,2:5.6700,3:5.7400,4:5.8000,5:5.8600,6:5.9100,7:5.9600,8:6.0100,9:6.0600,10:6.1000";
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
