#include "pch.h"
#include "Lang.h"
#include "RangeProfileView.h"
#include "AppMessages.h"
#include "resource.h"

// ---------------------------------------------------------------------------
void RangeProfileView::SetResult(std::shared_ptr<const dsp::FrameResult> r)
{
    m_res = std::move(r);
    if (::IsWindow(m_hWnd)) Invalidate(FALSE);
}

void RangeProfileView::SetDisplay(const DisplaySettings& d)
{
    m_disp = d;
    m_autoDbActive = false;
    if (::IsWindow(m_hWnd)) Invalidate(FALSE);
}

double RangeProfileView::FullSpanHz() const
{
    if (!m_res || !m_res->up.valid) return kEmptySpanHz;
    const double span = m_res->up.fsEffHz / 2.0;
    return span > 0.0 ? span : kEmptySpanHz;
}

double RangeProfileView::MaxSpanHz() const
{
    const double full = FullSpanHz();
    if (!m_res || !m_res->valid || m_res->fsHz <= 0.0) return full;
    return (std::max)(full, m_res->fsHz / 2.0);
}

void RangeProfileView::EffectiveX(double& f0, double& f1) const
{
    const double full = FullSpanHz();
    if (m_haveZoom && m_x1 > m_x0) { f0 = m_x0; f1 = m_x1; }
    else {
        f0 = 0.0;
        f1 = full;
        if (m_res && m_res->rangePerHz > 0.0 && m_maxRangeM > 0.0) {
            double fMax = m_maxRangeM / m_res->rangePerHz;
            if (fMax > 0.0 && fMax < full) f1 = fMax;
        }
    }
    if (f1 <= f0) f1 = f0 + 1.0;
}

void RangeProfileView::EffectiveDb(double& top, double& bottom) const
{
    if (m_autoDbActive || m_disp.autoDb) { top = m_autoTop; bottom = m_autoBottom; }
    else { top = m_disp.dbTop; bottom = m_disp.dbBottom; }
    if (top <= bottom) { top = 0.0; bottom = -120.0; }
}

void RangeProfileView::SetXRange(double f0, double f1)
{
    if (f1 <= f0) { m_haveZoom = false; m_x0 = m_x1 = 0.0; }
    else { m_haveZoom = true; m_x0 = f0; m_x1 = f1; }
    if (::IsWindow(m_hWnd)) Invalidate(FALSE);
}

void RangeProfileView::ResetZoom()
{
    m_haveZoom = false;
    m_x0 = m_x1 = 0.0;
    NotifyXRange();
    Invalidate(FALSE);
}

void RangeProfileView::ClearMarkers()
{
    m_mA = Marker{};
    m_mB = Marker{};
    Invalidate(FALSE);
}

void RangeProfileView::AutoscaleDb()
{
    if (!m_res || !m_res->up.valid) return;
    double f0, f1; EffectiveX(f0, f1);
    const dsp::RampSpectrum& s = m_res->up;
    size_t k0 = static_cast<size_t>((std::max)(0.0, f0 / s.freqResHz));
    size_t k1 = static_cast<size_t>(f1 / s.freqResHz) + 1;
    if (k1 > s.db.size()) k1 = s.db.size();
    if (k0 >= k1) return;
    float mx = -300.0f;
    for (size_t k = (std::max)(k0, s.firstBin); k < k1; ++k) mx = (std::max)(mx, s.db[k]);
    if (m_res->down.valid)
        for (size_t k = (std::max)(k0, m_res->down.firstBin); k < k1 && k < m_res->down.db.size(); ++k)
            mx = (std::max)(mx, m_res->down.db[k]);
    m_autoTop    = std::ceil((mx + 6.0) / 10.0) * 10.0;
    m_autoBottom = std::floor((s.noiseFloorDb - 15.0) / 10.0) * 10.0;
    if (m_autoTop - m_autoBottom < 30.0) m_autoBottom = m_autoTop - 30.0;
    m_autoDbActive = true;
    Invalidate(FALSE);
}

void RangeProfileView::SnapMarkersToPeaks()
{
    if (!m_res || !m_res->up.valid) return;
    const auto& pk = m_res->up.peaks;
    if (pk.size() >= 1) { m_mA.valid = true; m_mA.fHz = pk[0].freqHz; }
    if (pk.size() >= 2) { m_mB.valid = true; m_mB.fHz = pk[1].freqHz; }
    Invalidate(FALSE);
}

void RangeProfileView::NotifyXRange()
{
    if (CWnd* p = GetParent())
        if (::IsWindow(p->GetSafeHwnd()))
            p->PostMessage(WM_APP_XRANGE_CHANGED, static_cast<WPARAM>(GetDlgCtrlID()), 0);
}

// ---------------------------------------------------------------------------
RangeProfileView::Layout RangeProfileView::ComputeLayout(const CRect& rc) const
{
    Layout L;
    const bool haveRange = m_res && m_res->rangePerHz > 0.0;
    const int titleH  = S(20);
    const int topAxH  = haveRange ? S(18) : S(4);
    const int botAxH  = S(18);
    const int leftAxW = S(56);
    L.title      = CRect(rc.left, rc.top, rc.right, rc.top + titleH);
    L.axisTop    = CRect(rc.left + leftAxW, rc.top + titleH, rc.right - S(6), rc.top + titleH + topAxH);
    L.plot       = CRect(rc.left + leftAxW, L.axisTop.bottom, rc.right - S(6), rc.bottom - botAxH);
    L.axisBottom = CRect(L.plot.left, L.plot.bottom, L.plot.right, rc.bottom);
    L.axisLeft   = CRect(rc.left, L.plot.top, L.plot.left, L.plot.bottom);
    return L;
}

LinearAxis RangeProfileView::XAxis(const CRect& plot) const
{
    LinearAxis a;
    EffectiveX(a.v0, a.v1);
    a.px0 = plot.left;
    a.px1 = plot.right;
    return a;
}

LinearAxis RangeProfileView::YAxis(const CRect& plot) const
{
    LinearAxis a;
    double top, bottom; EffectiveDb(top, bottom);
    a.v0  = bottom; a.px0 = plot.bottom;
    a.v1  = top;    a.px1 = plot.top;
    return a;
}

double RangeProfileView::ValueAt(const dsp::RampSpectrum& s, double fHz, const std::vector<float>* vec) const
{
    const std::vector<float>& v = vec ? *vec : s.db;
    if (v.empty() || s.freqResHz <= 0.0) return -300.0;
    double kf = fHz / s.freqResHz;
    if (kf < 0.0) kf = 0.0;
    size_t k = static_cast<size_t>(kf);
    if (k + 1 >= v.size()) return v.back();
    double frac = kf - static_cast<double>(k);
    float a = v[k], b = v[k + 1];
    if (!std::isfinite(a) || !std::isfinite(b)) return a;
    return a + (b - a) * frac;
}

double RangeProfileView::SnapToPeak(double fHz, const LinearAxis& xa) const
{
    if (!m_res) return fHz;
    const int px = xa.ToPx(fHz);
    double best = fHz; int bestD = S(8) + 1;
    auto consider = [&](const std::vector<dsp::Peak>& pk) {
        for (const auto& p : pk) {
            int d = std::abs(xa.ToPx(p.freqHz) - px);
            if (d < bestD) { bestD = d; best = p.freqHz; }
        }
    };
    if (m_res->up.valid)   consider(m_res->up.peaks);
    if (m_res->down.valid) consider(m_res->down.peaks);
    return best;
}

// ---------------------------------------------------------------------------
void RangeProfileView::DrawTrace(CDC& dc, const CRect& plot, const LinearAxis& xa, const LinearAxis& ya,
                                 const std::vector<float>& db, double freqRes, COLORREF color, int penStyle, int width)
{
    if (db.size() < 2 || freqRes <= 0.0 || plot.Width() < 2) return;
    std::vector<POINT> pts;
    pts.reserve(static_cast<size_t>(plot.Width()) * 2);
    const int yMin = plot.top, yMax = plot.bottom;
    for (int x = plot.left; x < plot.right; ++x) {
        const double fA = xa.FromPx(x), fB = xa.FromPx(x + 1);
        double kA = fA / freqRes, kB = fB / freqRes;
        if (kB < 0.0 || kA >= static_cast<double>(db.size() - 1)) continue;
        if (kA < 0.0) kA = 0.0;
        size_t k0 = static_cast<size_t>(kA);
        size_t k1 = static_cast<size_t>(kB);
        if (k1 >= db.size()) k1 = db.size() - 1;
        float vMax, vMin;
        if (k1 > k0) {
            vMax = -1e30f; vMin = 1e30f;
            for (size_t k = k0; k <= k1; ++k) {
                float v = db[k];
                if (!std::isfinite(v)) continue;
                vMax = (std::max)(vMax, v); vMin = (std::min)(vMin, v);
            }
            if (vMax < -1e29f) continue;
        } else {
            double frac = kA - static_cast<double>(k0);
            float a = db[k0], b = db[(std::min)(k0 + 1, db.size() - 1)];
            if (!std::isfinite(a) || !std::isfinite(b)) continue;
            vMax = vMin = static_cast<float>(a + (b - a) * frac);
        }
        int y1 = ya.ToPx(vMax), y2 = ya.ToPx(vMin);
        y1 = (std::max)(yMin, (std::min)(yMax, y1));
        y2 = (std::max)(yMin, (std::min)(yMax, y2));
        pts.push_back({ x, y1 });
        if (y2 != y1) pts.push_back({ x, y2 });
    }
    if (pts.size() < 2) return;
    CPen pen(penStyle, width, color);
    CPen* pOld = dc.SelectObject(&pen);
    dc.Polyline(pts.data(), static_cast<int>(pts.size()));
    dc.SelectObject(pOld);
}

void RangeProfileView::DrawPeaks(CDC& dc, const CRect& plot, const LinearAxis& xa, const LinearAxis& ya)
{
    if (!m_res) return;
    const Theme::Palette& th = Theme::Get();
    CFont* pOld = dc.SelectObject(&m_fontLabel);
    dc.SetBkMode(TRANSPARENT);
    const int tri = S(5);

    auto drawSet = [&](const dsp::RampSpectrum& s, bool isUp) {
        int rank = 1;
        for (const auto& p : s.peaks) {
            const int x = xa.ToPx(p.freqHz);
            if (x < plot.left || x > plot.right) { ++rank; continue; }
            int y = ya.ToPx(p.ampDb) - S(4);
            if (y < plot.top + S(14)) y = plot.top + S(14);
            POINT t[3] = { { x, y }, { x - tri, y - 2 * tri }, { x + tri, y - 2 * tri } };
            CBrush br(isUp ? th.peak : th.traceDn);
            CPen pen(PS_SOLID, 1, isUp ? th.peak : th.traceDn);
            CBrush* pb = dc.SelectObject(&br);
            CPen* pp = dc.SelectObject(&pen);
            if (isUp) dc.Polygon(t, 3);
            else { CBrush* nb = static_cast<CBrush*>(dc.SelectStockObject(NULL_BRUSH)); dc.Polygon(t, 3); if (nb) dc.SelectObject(nb); }
            dc.SelectObject(pb);
            dc.SelectObject(pp);
            if (isUp) {
                CString lbl; lbl.Format(_T("%d"), rank);
                // Range / velocity from the matching target (if any).
                for (const auto& tg : m_res->targets) {
                    if (std::fabs(tg.fUpHz - p.freqHz) < 1e-6) {
                        if (m_res->rangePerHz > 0.0) {
                            CString r = FormatRange(tg.rangeM);
                            if (tg.paired) { CString v; v.Format(_T("  %+.2f m/s"), tg.velocityMps); r += v; }
                            lbl += _T("  ") + r;
                        }
                        break;
                    }
                }
                dc.SetTextColor(th.peak);
                CRect r(x - S(60), y - 2 * tri - S(16), x + S(140), y - 2 * tri - S(1));
                dc.DrawText(lbl, r, DT_LEFT | DT_BOTTOM | DT_SINGLELINE | DT_NOCLIP);
            }
            ++rank;
        }
    };
    if (m_disp.showUp && m_disp.showPeaks && m_res->up.valid)     drawSet(m_res->up, true);
    if (m_disp.showDown && m_disp.showPeaks && m_res->down.valid) drawSet(m_res->down, false);
    dc.SelectObject(pOld);
}

void RangeProfileView::DrawMarker(CDC& dc, const Layout& L, const LinearAxis& xa, double fHz,
                                  COLORREF color, LPCTSTR name, int& readoutX)
{
    const int x = xa.ToPx(fHz);
    if (x < L.plot.left || x > L.plot.right) return;
    CPen pen(PS_DASH, 1, color);
    CPen* pOld = dc.SelectObject(&pen);
    dc.MoveTo(x, L.plot.top); dc.LineTo(x, L.plot.bottom);
    dc.SelectObject(pOld);

    std::vector<CString> lines;
    CString s;
    s.Format(_T("%s  %s"), name, FormatFreq(fHz).GetString()); lines.push_back(s);
    if (m_res && m_res->rangePerHz > 0.0) {
        s.Format(_T("R  %s"), FormatRange(m_res->rangePerHz * fHz - m_res->rangeOffsetM).GetString());
        lines.push_back(s);
    }
    if (m_res && m_res->up.valid)   { s.Format(_T("UP  %s"), FormatDb(ValueAt(m_res->up, fHz)).GetString()); lines.push_back(s); }
    if (m_res && m_res->down.valid) { s.Format(_T("DN  %s"), FormatDb(ValueAt(m_res->down, fHz)).GetString()); lines.push_back(s); }
    DrawReadout(dc, L.plot, CPoint(readoutX, L.plot.top - S(10)), lines);
    readoutX += S(150);
}

void RangeProfileView::DrawCursor(CDC& dc, const Layout& L, const LinearAxis& xa, const LinearAxis& ya)
{
    if (!m_mouseIn || !L.plot.PtInRect(m_mouse) || !m_res) return;
    const Theme::Palette& th = Theme::Get();
    CPen pen(PS_DOT, 1, th.cursor);
    CPen* pOld = dc.SelectObject(&pen);
    dc.MoveTo(m_mouse.x, L.plot.top); dc.LineTo(m_mouse.x, L.plot.bottom);
    dc.MoveTo(L.plot.left, m_mouse.y); dc.LineTo(L.plot.right, m_mouse.y);
    dc.SelectObject(pOld);

    const double f = xa.FromPx(m_mouse.x);
    std::vector<CString> lines;
    CString s;
    s.Format(_T("f   %s"), FormatFreq(f).GetString()); lines.push_back(s);
    if (m_res->rangePerHz > 0.0) {
        s.Format(_T("R   %s"), FormatRange(m_res->rangePerHz * f - m_res->rangeOffsetM).GetString()); lines.push_back(s);
    }
    s.Format(_T("y   %s"), FormatDb(ya.FromPx(m_mouse.y)).GetString()); lines.push_back(s);
    if (m_res->up.valid) {
        s.Format(_T("UP  %s"), FormatDb(ValueAt(m_res->up, f)).GetString()); lines.push_back(s);
        double thr = ValueAt(m_res->up, f, &m_res->up.thrDb);
        if (std::isfinite(thr)) { s.Format(_T("thr %s"), FormatDb(thr).GetString()); lines.push_back(s); }
        double nz = ValueAt(m_res->up, f, &m_res->up.noiseDb);
        if (nz > -250.0) { s.Format(_T("nz  %s"), FormatDb(nz).GetString()); lines.push_back(s); }
    }
    if (m_res->down.valid) { s.Format(_T("DN  %s"), FormatDb(ValueAt(m_res->down, f)).GetString()); lines.push_back(s); }
    DrawReadout(dc, L.plot, m_mouse, lines);
}

// ---------------------------------------------------------------------------
void RangeProfileView::Render(CDC& dc, const CRect& rc)
{
    const Theme::Palette& th = Theme::Get();
    const Layout L = ComputeLayout(rc);
    dc.FillSolidRect(rc, th.plot);
    dc.FillSolidRect(L.title, th.panel);
    dc.FillSolidRect(L.axisBottom, th.axisStrip);
    dc.FillSolidRect(L.axisLeft, th.axisStrip);
    if (L.axisTop.Height() > S(6)) dc.FillSolidRect(L.axisTop, th.axisStrip);
    dc.SetBkMode(TRANSPARENT);

    // Title line.
    {
        CFont* pOld = dc.SelectObject(&m_fontTitle);
        dc.SetTextColor(th.textHdr);
        CString t = m_title.IsEmpty() ? CString(TR("Range profile")) : m_title;
        dc.TextOut(L.title.left + S(6), L.title.top + S(3), t);
        dc.SelectObject(&m_fontAxis);
        dc.SetTextColor(th.textDim);
        CString info;
        if (m_res && m_res->valid) {
            const dsp::RampSpectrum& s = m_res->up;
            info.Format(TR("frame %u   t = %u ms   bin %.1f Hz"), m_res->frameId, m_res->timestampMs, s.freqResHz);
            if (m_res->rangePerHz > 0.0) {
                CString r; r.Format(_T(" (%s)"), FormatRange(s.freqResHz * m_res->rangePerHz).GetString()); info += r;
            }
            CString more;
            more.Format(TR("   fs/%d = %s   N = %zu   DSP %.1f ms"), m_res->decimation,
                        FormatFreq(s.fsEffHz).GetString(), s.nSamples, m_res->processingMs);
            info += more;
            if (m_res->fromMcuFft) info += TR("   [MCU FFT]");
            const int rej = m_res->up.rejected.Total() + m_res->down.rejected.Total();
            if (rej > 0 || m_res->unconfirmed > 0) {
                CString rj; rj.Format(TR("   rejected: %d harmonic, %d spur, %d low SNR, %d unconfirmed"),
                                      m_res->up.rejected.harmonics + m_res->down.rejected.harmonics,
                                      m_res->up.rejected.spurs + m_res->down.rejected.spurs,
                                      m_res->up.rejected.lowSnr + m_res->down.rejected.lowSnr, m_res->unconfirmed);
                info += rj;
            }
        } else {
            info = TR("no data");
        }
        CRect ir(L.title.left + S(130), L.title.top, L.title.right - S(4), L.title.bottom);
        dc.DrawText(info, ir, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        dc.SelectObject(pOld);
    }

    if (L.plot.Width() < 10 || L.plot.Height() < 10) return;

    const LinearAxis xa = XAxis(L.plot);
    const LinearAxis ya = YAxis(L.plot);

    // Axes.
    DrawYAxis(dc, L.plot, L.axisLeft, ya, [](double v) { CString s; s.Format(_T("%.0f"), v); return s; }, true);
    DrawXAxis(dc, L.plot, L.axisBottom, xa, [](double v) {
        CString s;
        if (std::fabs(v) >= 1e6) s.Format(_T("%.3g MHz"), v / 1e6);
        else if (std::fabs(v) >= 1e3) s.Format(_T("%.4g kHz"), v / 1e3);
        else s.Format(_T("%.0f Hz"), v);
        return s; }, false, true);
    if (m_res && m_res->rangePerHz > 0.0) {
        LinearAxis ra = xa;
        const double off = m_res->rangeOffsetM;
        ra.v0 = xa.v0 * m_res->rangePerHz - off;
        ra.v1 = xa.v1 * m_res->rangePerHz - off;
        DrawXAxis(dc, L.plot, L.axisTop, ra, [](double v) { return FormatRangeTick(v); }, true, false);
    }
    {
        CFont* pOld = dc.SelectObject(&m_fontAxis);
        dc.SetTextColor(th.textDim);
        CString yl = (m_res && (m_res->fromMcuFft)) ? _T("dB") : _T("dBFS");
        dc.TextOut(rc.left + S(4), L.plot.top + S(2), yl);
        dc.SelectObject(pOld);
    }

    // Plot frame.
    {
        CPen pen(PS_SOLID, 1, th.axis);
        CPen* p = dc.SelectObject(&pen);
        CBrush* b = static_cast<CBrush*>(dc.SelectStockObject(NULL_BRUSH));
        dc.Rectangle(L.plot.left, L.plot.top, L.plot.right + 1, L.plot.bottom + 1);
        if (b) dc.SelectObject(b);
        dc.SelectObject(p);
    }

    if (!m_res || !m_res->valid) return;

    CRgn clip;
    clip.CreateRectRgnIndirect(&L.plot);
    dc.SelectClipRgn(&clip);

    if (m_disp.showNoise && m_res->up.valid)
        DrawTrace(dc, L.plot, xa, ya, m_res->up.noiseDb, m_res->up.freqResHz, th.noise, PS_DOT, 1);
    if (m_disp.showThreshold && m_res->up.valid)
        DrawTrace(dc, L.plot, xa, ya, m_res->up.thrDb, m_res->up.freqResHz, th.threshold, PS_DOT, 1);
    if (m_disp.showDown && m_res->down.valid)
        DrawTrace(dc, L.plot, xa, ya, m_res->down.db, m_res->down.freqResHz, th.traceDn, PS_SOLID, 1);
    if (m_disp.showUp && m_res->up.valid)
        DrawTrace(dc, L.plot, xa, ya, m_res->up.db, m_res->up.freqResHz, th.traceUp, PS_SOLID, 1);

    DrawPeaks(dc, L.plot, xa, ya);

    int rx = L.plot.left + S(4);
    if (m_mA.valid) DrawMarker(dc, L, xa, m_mA.fHz, th.markerA, _T("A"), rx);
    if (m_mB.valid) DrawMarker(dc, L, xa, m_mB.fHz, th.markerB, _T("B"), rx);
    if (m_mA.valid && m_mB.valid) {
        std::vector<CString> lines;
        CString s;
        const double df = m_mB.fHz - m_mA.fHz;
        s.Format(_T("B-A  %s"), FormatFreq(df).GetString()); lines.push_back(s);
        if (m_res->rangePerHz > 0.0) { s.Format(_T("dR   %s"), FormatRange(df * m_res->rangePerHz).GetString()); lines.push_back(s); }
        if (m_res->up.valid) { s.Format(_T("dUP  %s"), FormatDb(ValueAt(m_res->up, m_mB.fHz) - ValueAt(m_res->up, m_mA.fHz)).GetString()); lines.push_back(s); }
        DrawReadout(dc, L.plot, CPoint(rx, L.plot.top - S(10)), lines);
    }

    dc.SelectClipRgn(nullptr);

    // Legend.
    {
        int lx = L.plot.right - S(260), ly = L.plot.top + S(10);
        if (m_disp.showUp)        DrawLegendItem(dc, lx, ly, th.traceUp, TR("UP"));
        if (m_disp.showDown && m_res->down.valid) DrawLegendItem(dc, lx, ly, th.traceDn, TR("DOWN"));
        if (m_disp.showThreshold) DrawLegendItem(dc, lx, ly, th.threshold, TR("threshold"), true);
        if (m_disp.showNoise)     DrawLegendItem(dc, lx, ly, th.noise, TR("noise"), true);
    }

    DrawCursor(dc, L, xa, ya);
}

// ---------------------------------------------------------------------------
void RangeProfileView::OnPlotMouseMove(CPoint pt, UINT flags)
{
    if (m_dragging && (flags & MK_LBUTTON)) {
        CRect rc; GetClientRect(&rc);
        const Layout L = ComputeLayout(rc);
        const int dx = pt.x - m_dragStart.x;
        if (std::abs(dx) > S(3)) m_dragMoved = true;
        if (m_dragMoved && L.plot.Width() > 0) {
            const double span = m_dragX1 - m_dragX0;
            double shift = -static_cast<double>(dx) / L.plot.Width() * span;
            double nx0 = m_dragX0 + shift, nx1 = m_dragX1 + shift;
            const double full = FullSpanHz();
            if (nx0 < 0.0) { nx1 -= nx0; nx0 = 0.0; }
            if (nx1 > full) { nx0 -= (nx1 - full); nx1 = full; if (nx0 < 0.0) nx0 = 0.0; }
            m_x0 = nx0; m_x1 = nx1; m_haveZoom = true;
            NotifyXRange();
        }
    }
    Invalidate(FALSE);
}

void RangeProfileView::OnPlotLButtonDown(CPoint pt, UINT)
{
    CRect rc; GetClientRect(&rc);
    const Layout L = ComputeLayout(rc);
    if (!L.plot.PtInRect(pt)) return;
    m_dragging  = true;
    m_dragMoved = false;
    m_dragStart = pt;
    EffectiveX(m_dragX0, m_dragX1);
    SetCapture();
}

void RangeProfileView::OnPlotLButtonUp(CPoint pt, UINT flags)
{
    if (!m_dragging) return;
    m_dragging = false;
    if (GetCapture() == this) ReleaseCapture();
    if (m_dragMoved) return;
    CRect rc; GetClientRect(&rc);
    const Layout L = ComputeLayout(rc);
    if (!L.plot.PtInRect(pt)) return;
    const LinearAxis xa = XAxis(L.plot);
    double f = SnapToPeak(xa.FromPx(pt.x), xa);
    if (flags & MK_SHIFT) { m_mB.valid = true; m_mB.fHz = f; }
    else                  { m_mA.valid = true; m_mA.fHz = f; }
    Invalidate(FALSE);
}

void RangeProfileView::OnPlotLButtonDblClk(CPoint, UINT)
{
    m_dragging = false;
    ResetZoom();
}

BOOL RangeProfileView::OnPlotMouseWheel(UINT flags, short zDelta, CPoint pt)
{
    CRect rc; GetClientRect(&rc);
    const Layout L = ComputeLayout(rc);
    if (!L.plot.PtInRect(pt) || L.plot.Width() < 2) return FALSE;
    double f0, f1; EffectiveX(f0, f1);
    const double full = FullSpanHz();
    const double span = f1 - f0;
    if (flags & MK_SHIFT) {
        const double shift = (zDelta > 0 ? -0.1 : 0.1) * span;
        f0 += shift; f1 += shift;
    } else {
        const LinearAxis xa = XAxis(L.plot);
        const double fc = xa.FromPx(pt.x);
        const double k = (zDelta > 0) ? 1.0 / 1.25 : 1.25;
        f0 = fc - (fc - f0) * k;
        f1 = fc + (f1 - fc) * k;
        if (f1 - f0 < 10.0 * ((m_res && m_res->up.valid) ? m_res->up.freqResHz : 1.0)) return TRUE;
    }
    const double maxSpan = MaxSpanHz();
    if (f0 < 0.0) { f1 -= f0; f0 = 0.0; }
    if (f1 > maxSpan) { f0 -= (f1 - maxSpan); f1 = maxSpan; if (f0 < 0.0) f0 = 0.0; }
    m_x0 = f0; m_x1 = f1; m_haveZoom = (f0 > 0.0 || f1 < full || f1 > full * 1.0001);
    NotifyXRange();
    Invalidate(FALSE);
    return TRUE;
}

void RangeProfileView::OnPlotKeyDown(UINT nChar, UINT)
{
    switch (nChar) {
    case VK_ESCAPE: ClearMarkers(); break;
    case VK_HOME:   ResetZoom(); break;
    case 'A':       AutoscaleDb(); break;
    default: break;
    }
}

void RangeProfileView::OnPlotRButtonUp(CPoint pt, UINT)
{
    CMenu menu;
    menu.CreatePopupMenu();
    menu.AppendMenu(MF_STRING, ID_PLOT_RESET_ZOOM,    TR("Reset zoom\tHome"));
    menu.AppendMenu(MF_STRING, ID_PLOT_AUTOSCALE,     TR("Autoscale dB\tA"));
    menu.AppendMenu(MF_STRING, ID_PLOT_SNAP_PEAKS,    TR("Markers on two strongest peaks"));
    menu.AppendMenu(MF_STRING, ID_PLOT_CLEAR_MARKERS, TR("Clear markers\tEsc"));
    menu.AppendMenu(MF_SEPARATOR);
    menu.AppendMenu(MF_STRING, ID_PLOT_COPY_IMAGE,    TR("Copy image"));
    CPoint sp = pt; ClientToScreen(&sp);
    int cmd = menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD, sp.x, sp.y, this);
    switch (cmd) {
    case ID_PLOT_RESET_ZOOM:    ResetZoom(); break;
    case ID_PLOT_AUTOSCALE:     AutoscaleDb(); break;
    case ID_PLOT_SNAP_PEAKS:    SnapMarkersToPeaks(); break;
    case ID_PLOT_CLEAR_MARKERS: ClearMarkers(); break;
    case ID_PLOT_COPY_IMAGE:    CopyImageToClipboard(); break;
    default: break;
    }
}
