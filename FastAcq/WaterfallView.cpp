#include "pch.h"
#include "WaterfallView.h"
#include "AppMessages.h"
#include "ColorMap.h"
#include "resource.h"

WaterfallView::~WaterfallView()
{
    FreeDib();
}

void WaterfallView::FreeDib()
{
    if (m_hDib) { ::DeleteObject(m_hDib); m_hDib = nullptr; }
    m_px = nullptr;
    m_dibW = m_dibH = 0;
}

void WaterfallView::EnsureDib(int w, int h)
{
    if (w <= 0 || h <= 0) { FreeDib(); return; }
    if (m_hDib && w == m_dibW && h == m_dibH) return;
    FreeDib();
    BITMAPINFO bi{};
    bi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth       = w;
    bi.bmiHeader.biHeight      = -h;    // top-down
    bi.bmiHeader.biPlanes      = 1;
    bi.bmiHeader.biBitCount    = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    HDC hdc = ::GetDC(nullptr);
    m_hDib = ::CreateDIBSection(hdc, &bi, DIB_RGB_COLORS, reinterpret_cast<void**>(&m_px), nullptr, 0);
    ::ReleaseDC(nullptr, hdc);
    if (!m_hDib) { m_px = nullptr; return; }
    m_dibW = w; m_dibH = h;
    std::memset(m_px, 0, static_cast<size_t>(w) * h * 4);
    m_dirty = true;
}

// ---------------------------------------------------------------------------
void WaterfallView::PushResult(const dsp::FrameResult& r)
{
    if (!r.valid || !r.up.valid || r.up.db.size() < 2) return;
    const dsp::RampSpectrum& s = r.up;
    const size_t bins = s.db.size();
    const int cols = static_cast<int>((std::min<size_t>)(bins, kMaxCols));
    const double hzPerCol = s.freqResHz * static_cast<double>(bins) / cols;

    const bool gridChanged = (m_hzPerCol > 0.0 && std::fabs(hzPerCol - m_hzPerCol) > 1e-9 * hzPerCol) ||
                             std::fabs(m_fsEffHz - s.fsEffHz) > 1e-6;
    if (gridChanged) { m_rows.clear(); m_dirty = true; }
    m_hzPerCol    = hzPerCol;
    m_fsEffHz     = s.fsEffHz;
    m_rangePerHz  = r.rangePerHz;
    m_rangeOffsetM= r.rangeOffsetM;

    Row row;
    row.db.resize(cols);
    for (int c = 0; c < cols; ++c) {
        size_t k0 = static_cast<size_t>(c) * bins / cols;
        size_t k1 = static_cast<size_t>(c + 1) * bins / cols;
        if (k1 <= k0) k1 = k0 + 1;
        if (k1 > bins) k1 = bins;
        float v = -300.0f;
        for (size_t k = k0; k < k1; ++k) v = (std::max)(v, s.db[k]);
        row.db[c] = v;
    }
    row.hzPerCol = hzPerCol;
    row.tsMs     = r.timestampMs;
    row.frameId  = r.frameId;
    m_rows.push_front(std::move(row));
    while (m_rows.size() > m_maxRows) m_rows.pop_back();

    // Fast path: scroll the cached DIB and paint only the new row.
    if (!m_dirty && m_px && m_dibW > 0 && m_dibH > 1) {
        const size_t rowBytes = static_cast<size_t>(m_dibW) * 4;
        std::memmove(reinterpret_cast<uint8_t*>(m_px) + rowBytes, m_px, rowBytes * (m_dibH - 1));
        RenderRow(m_rows.front(), m_px, m_dibW, m_dibF0, m_dibF1, m_dibTop, m_dibBottom);
    } else {
        m_dirty = true;
    }
    if (::IsWindow(m_hWnd)) Invalidate(FALSE);
}

void WaterfallView::SetDisplay(const DisplaySettings& d)
{
    m_disp = d;
    m_maxRows = static_cast<size_t>((std::max)(64, d.waterfallRows));
    while (m_rows.size() > m_maxRows) m_rows.pop_back();
    m_autoDbActive = false;
    m_dirty = true;
    if (::IsWindow(m_hWnd)) Invalidate(FALSE);
}

double WaterfallView::FullSpanHz() const
{
    return m_fsEffHz > 0.0 ? m_fsEffHz / 2.0 : kEmptySpanHz;
}

void WaterfallView::EffectiveX(double& f0, double& f1) const
{
    const double full = FullSpanHz();
    if (m_haveZoom && m_x1 > m_x0) { f0 = m_x0; f1 = m_x1; }
    else {
        f0 = 0.0; f1 = full;
        if (m_rangePerHz > 0.0 && m_maxRangeM > 0.0) {
            double fMax = m_maxRangeM / m_rangePerHz;
            if (fMax > 0.0 && fMax < full) f1 = fMax;
        }
    }
    if (f1 <= f0) f1 = f0 + 1.0;
}

void WaterfallView::EffectiveDb(double& top, double& bottom) const
{
    if (m_autoDbActive || m_disp.autoDb) { top = m_autoTop; bottom = m_autoBottom; }
    else { top = m_disp.dbTop; bottom = m_disp.dbBottom; }
    if (top <= bottom) { top = 0.0; bottom = -120.0; }
}

void WaterfallView::SetXRange(double f0, double f1)
{
    if (f1 <= f0) { m_haveZoom = false; m_x0 = m_x1 = 0.0; }
    else { m_haveZoom = true; m_x0 = f0; m_x1 = f1; }
    m_dirty = true;
    if (::IsWindow(m_hWnd)) Invalidate(FALSE);
}

void WaterfallView::ResetZoom()
{
    m_haveZoom = false; m_x0 = m_x1 = 0.0; m_dirty = true;
    NotifyXRange();
    Invalidate(FALSE);
}

void WaterfallView::Clear()
{
    m_rows.clear();
    m_dirty = true;
    if (::IsWindow(m_hWnd)) Invalidate(FALSE);
}

void WaterfallView::AutoscaleDb()
{
    if (m_rows.empty()) return;
    double f0, f1; EffectiveX(f0, f1);
    float mx = -300.0f;
    std::vector<float> all;
    size_t nRows = (std::min<size_t>)(m_rows.size(), 32);
    for (size_t i = 0; i < nRows; ++i) {
        const Row& r = m_rows[i];
        int c0 = static_cast<int>(f0 / r.hzPerCol), c1 = static_cast<int>(f1 / r.hzPerCol) + 1;
        if (c0 < 1) c0 = 1;
        if (c1 > static_cast<int>(r.db.size())) c1 = static_cast<int>(r.db.size());
        for (int c = c0; c < c1; ++c) { mx = (std::max)(mx, r.db[c]); all.push_back(r.db[c]); }
    }
    if (all.empty()) return;
    std::nth_element(all.begin(), all.begin() + static_cast<std::ptrdiff_t>(all.size() / 2), all.end());
    const float median = all[all.size() / 2];
    m_autoTop    = std::ceil((mx + 3.0) / 5.0) * 5.0;
    m_autoBottom = std::floor((median - 5.0) / 5.0) * 5.0;
    if (m_autoTop - m_autoBottom < 20.0) m_autoBottom = m_autoTop - 20.0;
    m_autoDbActive = true;
    m_dirty = true;
    Invalidate(FALSE);
}

void WaterfallView::NotifyXRange()
{
    if (CWnd* p = GetParent())
        if (::IsWindow(p->GetSafeHwnd()))
            p->PostMessage(WM_APP_XRANGE_CHANGED, static_cast<WPARAM>(GetDlgCtrlID()), 0);
}

// ---------------------------------------------------------------------------
WaterfallView::Layout WaterfallView::ComputeLayout(const CRect& rc) const
{
    Layout L;
    const int titleH = S(20), botAxH = S(18), leftAxW = S(56), cbW = S(12), cbLblW = S(40);
    L.title      = CRect(rc.left, rc.top, rc.right, rc.top + titleH);
    L.plot       = CRect(rc.left + leftAxW, rc.top + titleH, rc.right - cbW - cbLblW - S(6), rc.bottom - botAxH);
    L.axisBottom = CRect(L.plot.left, L.plot.bottom, L.plot.right, rc.bottom);
    L.axisLeft   = CRect(rc.left, L.plot.top, L.plot.left, L.plot.bottom);
    L.colorbar   = CRect(L.plot.right + S(4), L.plot.top, L.plot.right + S(4) + cbW, L.plot.bottom);
    L.cbLabels   = CRect(L.colorbar.right + S(2), L.plot.top, rc.right, L.plot.bottom);
    return L;
}

void WaterfallView::RenderRow(const Row& r, DWORD* dst, int w, double f0, double f1,
                              double top, double bottom) const
{
    const COLORREF* pal = ColorMap::Table(static_cast<Palette>(m_disp.palette));
    const double scale = 255.0 / (top - bottom);
    const int ncol = static_cast<int>(r.db.size());
    for (int x = 0; x < w; ++x) {
        const double fa = f0 + (f1 - f0) * (static_cast<double>(x) / w);
        const double fb = f0 + (f1 - f0) * (static_cast<double>(x + 1) / w);
        int c0 = static_cast<int>(fa / r.hzPerCol);
        int c1 = static_cast<int>(fb / r.hzPerCol);
        if (c1 < c0) c1 = c0;
        if (c0 < 0) c0 = 0;
        if (c0 >= ncol) { dst[x] = 0; continue; }
        if (c1 >= ncol) c1 = ncol - 1;
        float v = -300.0f;
        for (int c = c0; c <= c1; ++c) v = (std::max)(v, r.db[c]);
        double n = (v - bottom) * scale;
        int idx = (n <= 0.0) ? 0 : (n >= 255.0 ? 255 : static_cast<int>(n));
        const COLORREF cc = pal[idx];
        dst[x] = (static_cast<DWORD>(GetRValue(cc)) << 16) | (static_cast<DWORD>(GetGValue(cc)) << 8) | GetBValue(cc);
    }
}

void WaterfallView::RebuildDib(const CRect& plot)
{
    const int w = plot.Width();
    const int h = (std::max)(1, plot.Height());
    EnsureDib(w, h);
    if (!m_px) return;
    double f0, f1; EffectiveX(f0, f1);
    double top, bottom; EffectiveDb(top, bottom);
    const size_t rowBytes = static_cast<size_t>(w) * 4;
    const COLORREF bg = Theme::Get().plot;
    const DWORD bgPx = (static_cast<DWORD>(GetRValue(bg)) << 16) | (static_cast<DWORD>(GetGValue(bg)) << 8) | GetBValue(bg);
    for (int y = 0; y < h; ++y) {
        DWORD* dst = m_px + static_cast<size_t>(y) * w;
        if (static_cast<size_t>(y) < m_rows.size()) RenderRow(m_rows[static_cast<size_t>(y)], dst, w, f0, f1, top, bottom);
        else for (int x = 0; x < w; ++x) dst[x] = bgPx;
    }
    (void)rowBytes;
    m_dibF0 = f0; m_dibF1 = f1; m_dibTop = top; m_dibBottom = bottom; m_dibPalette = m_disp.palette;
    m_dirty = false;
}

float WaterfallView::ValueAt(const Row& r, double fHz) const
{
    if (r.hzPerCol <= 0.0 || r.db.empty()) return -300.0f;
    int c = static_cast<int>(fHz / r.hzPerCol);
    if (c < 0) c = 0;
    if (c >= static_cast<int>(r.db.size())) c = static_cast<int>(r.db.size()) - 1;
    return r.db[static_cast<size_t>(c)];
}

void WaterfallView::OnPlotSize(int, int)
{
    m_dirty = true;
}

// ---------------------------------------------------------------------------
void WaterfallView::Render(CDC& dc, const CRect& rc)
{
    const Theme::Palette& th = Theme::Get();
    const Layout L = ComputeLayout(rc);
    dc.FillSolidRect(rc, th.plot);
    dc.FillSolidRect(L.title, th.panel);
    dc.FillSolidRect(L.axisBottom, th.axisStrip);
    dc.FillSolidRect(L.axisLeft, th.axisStrip);
    dc.SetBkMode(TRANSPARENT);

    {
        CFont* pOld = dc.SelectObject(&m_fontTitle);
        dc.SetTextColor(th.textHdr);
        dc.TextOut(L.title.left + S(6), L.title.top + S(3), m_title.IsEmpty() ? CString(_T("Waterfall (range - time)")) : m_title);
        dc.SelectObject(&m_fontAxis);
        dc.SetTextColor(th.textDim);
        CString info;
        info.Format(_T("%zu rows   palette %s"), m_rows.size(), ColorMap::Name(static_cast<Palette>(m_disp.palette)));
        CRect ir(L.title.left + S(180), L.title.top, L.title.right - S(4), L.title.bottom);
        dc.DrawText(info, ir, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        dc.SelectObject(pOld);
    }
    if (L.plot.Width() < 10 || L.plot.Height() < 10) return;

    double f0, f1; EffectiveX(f0, f1);
    double top, bottom; EffectiveDb(top, bottom);
    if (m_dirty || !m_px || m_dibW != L.plot.Width() || m_dibH != L.plot.Height() ||
        m_dibF0 != f0 || m_dibF1 != f1 || m_dibTop != top || m_dibBottom != bottom || m_dibPalette != m_disp.palette)
        RebuildDib(L.plot);

    if (m_px) {
        BITMAPINFO bi{};
        bi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
        bi.bmiHeader.biWidth       = m_dibW;
        bi.bmiHeader.biHeight      = -m_dibH;
        bi.bmiHeader.biPlanes      = 1;
        bi.bmiHeader.biBitCount    = 32;
        bi.bmiHeader.biCompression = BI_RGB;
        ::SetDIBitsToDevice(dc.GetSafeHdc(), L.plot.left, L.plot.top, m_dibW, m_dibH,
                            0, 0, 0, m_dibH, m_px, &bi, DIB_RGB_COLORS);
    }

    // X axis (frequency, with range if known).
    LinearAxis xa; xa.v0 = f0; xa.v1 = f1; xa.px0 = L.plot.left; xa.px1 = L.plot.right;
    if (m_rangePerHz > 0.0) {
        LinearAxis ra = xa;
        ra.v0 = f0 * m_rangePerHz - m_rangeOffsetM;
        ra.v1 = f1 * m_rangePerHz - m_rangeOffsetM;
        DrawXAxis(dc, L.plot, L.axisBottom, ra, [](double v) { CString s; s.Format(_T("%.4g m"), v); return s; }, false, false);
    } else {
        DrawXAxis(dc, L.plot, L.axisBottom, xa, [](double v) {
            CString s;
            if (std::fabs(v) >= 1e6) s.Format(_T("%.3g MHz"), v / 1e6);
            else if (std::fabs(v) >= 1e3) s.Format(_T("%.4g kHz"), v / 1e3);
            else s.Format(_T("%.0f Hz"), v);
            return s; }, false, false);
    }

    // Y axis: time (seconds before the newest row), one row per pixel.
    {
        CFont* pOld = dc.SelectObject(&m_fontAxis);
        dc.SetTextColor(th.axisText);
        CPen tickPen(PS_SOLID, 1, th.axis);
        CPen* pp = dc.SelectObject(&tickPen);
        const int step = (std::max)(S(40), 1);
        const uint32_t ts0 = m_rows.empty() ? 0 : m_rows.front().tsMs;
        for (int y = L.plot.top; y < L.plot.bottom; y += step) {
            const size_t row = static_cast<size_t>(y - L.plot.top);
            CString lbl;
            if (row < m_rows.size()) {
                const uint32_t ts = m_rows[row].tsMs;
                if (ts0 >= ts) lbl = _T("-") + FormatTime((ts0 - ts) / 1000.0);
                else lbl.Format(_T("-%zu"), row);
            } else if (m_rows.empty() && y == L.plot.top) {
                lbl = _T("0");
            } else {
                continue;
            }
            dc.MoveTo(L.plot.left - S(3), y); dc.LineTo(L.plot.left, y);
            CRect r(L.axisLeft.left, y - S(8), L.axisLeft.right - S(4), y + S(8));
            dc.DrawText(lbl, r, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP);
        }
        dc.SelectObject(pp);
        dc.SelectObject(pOld);
    }

    // Colour bar.
    {
        const COLORREF* pal = ColorMap::Table(static_cast<Palette>(m_disp.palette));
        const int h = L.colorbar.Height();
        for (int y = 0; y < h; ++y) {
            int idx = 255 - (y * 255) / (std::max)(1, h - 1);
            dc.FillSolidRect(L.colorbar.left, L.colorbar.top + y, L.colorbar.Width(), 1, pal[idx]);
        }
        CFont* pOld = dc.SelectObject(&m_fontAxis);
        dc.SetTextColor(th.axisText);
        CString t; t.Format(_T("%.0f"), top);
        dc.TextOut(L.cbLabels.left, L.cbLabels.top, t);
        t.Format(_T("%.0f"), bottom);
        dc.TextOut(L.cbLabels.left, L.cbLabels.bottom - S(12), t);
        t.Format(_T("%.0f"), 0.5 * (top + bottom));
        dc.TextOut(L.cbLabels.left, (L.cbLabels.top + L.cbLabels.bottom) / 2 - S(6), t);
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

    // Cursor readout.
    if (m_mouseIn && L.plot.PtInRect(m_mouse) && !m_rows.empty()) {
        const size_t row = static_cast<size_t>(m_mouse.y - L.plot.top);
        const double f = xa.FromPx(m_mouse.x);
        std::vector<CString> lines;
        CString s;
        s.Format(_T("f   %s"), FormatFreq(f).GetString()); lines.push_back(s);
        if (m_rangePerHz > 0.0) { s.Format(_T("R   %s"), FormatRange(f * m_rangePerHz - m_rangeOffsetM).GetString()); lines.push_back(s); }
        if (row < m_rows.size()) {
            const Row& r = m_rows[row];
            const uint32_t ts0 = m_rows.front().tsMs;
            s.Format(_T("t   -%s  (frame %u)"), FormatTime(ts0 >= r.tsMs ? (ts0 - r.tsMs) / 1000.0 : 0.0).GetString(), r.frameId);
            lines.push_back(s);
            s.Format(_T("P   %s"), FormatDb(ValueAt(r, f)).GetString()); lines.push_back(s);
        }
        CPen pen(PS_DOT, 1, th.cursor);
        CPen* pp = dc.SelectObject(&pen);
        dc.MoveTo(m_mouse.x, L.plot.top); dc.LineTo(m_mouse.x, L.plot.bottom);
        dc.SelectObject(pp);
        DrawReadout(dc, L.plot, m_mouse, lines);
    }
}

// ---------------------------------------------------------------------------
void WaterfallView::OnPlotMouseMove(CPoint, UINT)
{
    Invalidate(FALSE);
}

void WaterfallView::OnPlotLButtonDblClk(CPoint, UINT)
{
    ResetZoom();
}

BOOL WaterfallView::OnPlotMouseWheel(UINT flags, short zDelta, CPoint pt)
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
        LinearAxis xa; xa.v0 = f0; xa.v1 = f1; xa.px0 = L.plot.left; xa.px1 = L.plot.right;
        const double fc = xa.FromPx(pt.x);
        const double k = (zDelta > 0) ? 1.0 / 1.25 : 1.25;
        f0 = fc - (fc - f0) * k;
        f1 = fc + (f1 - fc) * k;
        if (f1 - f0 < 10.0 * (m_hzPerCol > 0.0 ? m_hzPerCol : 1.0)) return TRUE;
    }
    if (f0 < 0.0) { f1 -= f0; f0 = 0.0; }
    if (f1 > full) { f0 -= (f1 - full); f1 = full; if (f0 < 0.0) f0 = 0.0; }
    m_x0 = f0; m_x1 = f1; m_haveZoom = (f0 > 0.0 || f1 < full);
    m_dirty = true;
    NotifyXRange();
    Invalidate(FALSE);
    return TRUE;
}

void WaterfallView::OnPlotRButtonUp(CPoint pt, UINT)
{
    CMenu menu;
    menu.CreatePopupMenu();
    menu.AppendMenu(MF_STRING, ID_PLOT_RESET_ZOOM,    _T("Reset zoom"));
    menu.AppendMenu(MF_STRING, ID_PLOT_AUTOSCALE,     _T("Autoscale colours"));
    menu.AppendMenu(MF_STRING, ID_PLOT_CLEAR_HISTORY, _T("Clear history"));
    menu.AppendMenu(MF_SEPARATOR);
    menu.AppendMenu(MF_STRING, ID_PLOT_COPY_IMAGE,    _T("Copy image"));
    CPoint sp = pt; ClientToScreen(&sp);
    int cmd = menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD, sp.x, sp.y, this);
    switch (cmd) {
    case ID_PLOT_RESET_ZOOM:    ResetZoom(); break;
    case ID_PLOT_AUTOSCALE:     AutoscaleDb(); break;
    case ID_PLOT_CLEAR_HISTORY: Clear(); break;
    case ID_PLOT_COPY_IMAGE:    CopyImageToClipboard(); break;
    default: break;
    }
}
