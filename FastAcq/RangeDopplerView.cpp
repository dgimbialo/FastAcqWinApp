#include "pch.h"
#include "RangeDopplerView.h"
#include "ColorMap.h"
#include "resource.h"

RangeDopplerView::~RangeDopplerView() { FreeDib(); }

void RangeDopplerView::FreeDib()
{
    if (m_hDib) { ::DeleteObject(m_hDib); m_hDib = nullptr; }
    m_px = nullptr; m_dibW = m_dibH = 0;
}

void RangeDopplerView::SetResult(std::shared_ptr<const dsp::FrameResult> r)
{
    m_res = std::move(r);
    m_dirty = true;
    if (::IsWindow(m_hWnd)) Invalidate(FALSE);
}

void RangeDopplerView::SetDisplay(const DisplaySettings& d)
{
    m_disp = d;
    m_dirty = true;
    if (::IsWindow(m_hWnd)) Invalidate(FALSE);
}

RangeDopplerView::Layout RangeDopplerView::ComputeLayout(const CRect& rc) const
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

void RangeDopplerView::BuildDib()
{
    FreeDib();
    if (!HasData()) return;
    const dsp::RangeDopplerMap& rd = m_res->rd;
    const int w = rd.nRange, h = rd.nDoppler;
    BITMAPINFO bi{};
    bi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth       = w;
    bi.bmiHeader.biHeight      = -h;
    bi.bmiHeader.biPlanes      = 1;
    bi.bmiHeader.biBitCount    = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    HDC hdc = ::GetDC(nullptr);
    m_hDib = ::CreateDIBSection(hdc, &bi, DIB_RGB_COLORS, reinterpret_cast<void**>(&m_px), nullptr, 0);
    ::ReleaseDC(nullptr, hdc);
    if (!m_hDib) { m_px = nullptr; return; }
    m_dibW = w; m_dibH = h;

    m_top    = rd.maxDb;
    m_bottom = (std::max)(static_cast<double>(rd.minDb), rd.maxDb - 70.0);
    if (m_top <= m_bottom) m_bottom = m_top - 1.0;
    const COLORREF* pal = ColorMap::Table(static_cast<Palette>(m_disp.palette));
    const double scale = 255.0 / (m_top - m_bottom);
    // Row 0 of the map = most negative velocity; draw with positive velocity at the top.
    for (int y = 0; y < h; ++y) {
        const int srcRow = h - 1 - y;
        DWORD* dst = m_px + static_cast<size_t>(y) * w;
        const float* src = rd.db.data() + static_cast<size_t>(srcRow) * w;
        for (int x = 0; x < w; ++x) {
            double n = (src[x] - m_bottom) * scale;
            int idx = (n <= 0.0) ? 0 : (n >= 255.0 ? 255 : static_cast<int>(n));
            const COLORREF c = pal[idx];
            dst[x] = (static_cast<DWORD>(GetRValue(c)) << 16) | (static_cast<DWORD>(GetGValue(c)) << 8) | GetBValue(c);
        }
    }
    m_dirty = false;
}

void RangeDopplerView::Render(CDC& dc, const CRect& rc)
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
        dc.TextOut(L.title.left + S(6), L.title.top + S(3), _T("Range - Doppler"));
        dc.SelectObject(&m_fontAxis);
        dc.SetTextColor(th.textDim);
        CString info;
        if (HasData()) {
            const dsp::RangeDopplerMap& rd = m_res->rd;
            info.Format(_T("%d chirps   %d x %d   v bin %.3f m/s   v max %.2f m/s"),
                        m_res->chirps, rd.nRange, rd.nDoppler, rd.velBinMps, rd.velMaxMps);
        } else {
            info = _T("needs chirps per frame > 1 (burst)");
        }
        CRect ir(L.title.left + S(130), L.title.top, L.title.right - S(4), L.title.bottom);
        dc.DrawText(info, ir, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        dc.SelectObject(pOld);
    }
    if (L.plot.Width() < 10 || L.plot.Height() < 10) return;
    if (m_dirty) BuildDib();
    if (!m_px || !HasData()) return;

    const dsp::RangeDopplerMap& rd = m_res->rd;
    BITMAPINFO bi{};
    bi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth       = m_dibW;
    bi.bmiHeader.biHeight      = -m_dibH;
    bi.bmiHeader.biPlanes      = 1;
    bi.bmiHeader.biBitCount    = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    ::SetStretchBltMode(dc.GetSafeHdc(), COLORONCOLOR);
    ::StretchDIBits(dc.GetSafeHdc(), L.plot.left, L.plot.top, L.plot.Width(), L.plot.Height(),
                    0, 0, m_dibW, m_dibH, m_px, &bi, DIB_RGB_COLORS, SRCCOPY);

    // Axes: x = range (or frequency), y = velocity.
    LinearAxis xa; xa.px0 = L.plot.left; xa.px1 = L.plot.right;
    const bool haveRange = rd.rangeBinM > 0.0;
    xa.v0 = haveRange ? -m_res->rangeOffsetM : 0.0;
    xa.v1 = haveRange ? rd.nRange * rd.rangeBinM - m_res->rangeOffsetM : rd.nRange * rd.freqResHz;
    DrawXAxis(dc, L.plot, L.axisBottom, xa, [haveRange](double v) {
        CString s;
        if (haveRange) s.Format(_T("%.4g m"), v);
        else if (std::fabs(v) >= 1e3) s.Format(_T("%.4g kHz"), v / 1e3);
        else s.Format(_T("%.0f Hz"), v);
        return s; }, false, false);
    LinearAxis ya; ya.px0 = L.plot.bottom; ya.px1 = L.plot.top;
    ya.v0 = -rd.velMaxMps; ya.v1 = rd.velMaxMps;
    if (rd.velMaxMps <= 0.0) { ya.v0 = -static_cast<double>(rd.nDoppler) / 2; ya.v1 = static_cast<double>(rd.nDoppler) / 2; }
    DrawYAxis(dc, L.plot, L.axisLeft, ya, [&](double v) { CString s; s.Format(rd.velMaxMps > 0.0 ? _T("%+.2f") : _T("%+.0f"), v); return s; }, false);
    {
        CFont* pOld = dc.SelectObject(&m_fontAxis);
        dc.SetTextColor(th.textDim);
        dc.TextOut(rc.left + S(4), L.plot.top + S(2), rd.velMaxMps > 0.0 ? _T("m/s") : _T("bin"));
        dc.SelectObject(pOld);
    }
    // Zero-velocity line.
    {
        CPen pen(PS_DOT, 1, th.cursor);
        CPen* p = dc.SelectObject(&pen);
        int y0 = ya.ToPx(0.0);
        dc.MoveTo(L.plot.left, y0); dc.LineTo(L.plot.right, y0);
        dc.SelectObject(p);
    }
    // Peak marker.
    if (rd.peakRange >= 0 && rd.peakDoppler >= 0) {
        const int px = L.plot.left + static_cast<int>((rd.peakRange + 0.5) * L.plot.Width() / rd.nRange);
        const int py = L.plot.top + static_cast<int>((rd.nDoppler - 1 - rd.peakDoppler + 0.5) * L.plot.Height() / rd.nDoppler);
        CPen pen(PS_SOLID, 1, th.peak);
        CPen* p = dc.SelectObject(&pen);
        CBrush* b = static_cast<CBrush*>(dc.SelectStockObject(NULL_BRUSH));
        dc.Ellipse(px - S(6), py - S(6), px + S(6), py + S(6));
        if (b) dc.SelectObject(b);
        dc.SelectObject(p);
        CFont* pOld = dc.SelectObject(&m_fontLabel);
        dc.SetTextColor(th.peak);
        CString lbl;
        const double v = (rd.peakDoppler - rd.nDoppler / 2) * rd.velBinMps;
        if (haveRange) lbl.Format(_T("%s  %+.2f m/s  %.1f dB"), FormatRange(rd.peakRange * rd.rangeBinM - m_res->rangeOffsetM).GetString(), v, rd.peakDb);
        else lbl.Format(_T("bin %d  row %d  %.1f dB"), rd.peakRange, rd.peakDoppler, rd.peakDb);
        dc.TextOut(px + S(8), py - S(8), lbl);
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
        CString t; t.Format(_T("%.0f"), m_top);
        dc.TextOut(L.cbLabels.left, L.cbLabels.top, t);
        t.Format(_T("%.0f"), m_bottom);
        dc.TextOut(L.cbLabels.left, L.cbLabels.bottom - S(12), t);
        dc.SelectObject(pOld);
    }
    // Cursor readout.
    if (m_mouseIn && L.plot.PtInRect(m_mouse)) {
        const int col = (m_mouse.x - L.plot.left) * rd.nRange / (std::max)(1, L.plot.Width());
        const int rowFromTop = (m_mouse.y - L.plot.top) * rd.nDoppler / (std::max)(1, L.plot.Height());
        const int row = rd.nDoppler - 1 - rowFromTop;
        std::vector<CString> lines;
        CString s;
        if (col >= 0 && col < rd.nRange && row >= 0 && row < rd.nDoppler) {
            if (haveRange) s.Format(_T("R   %s"), FormatRange(col * rd.rangeBinM - m_res->rangeOffsetM).GetString());
            else s.Format(_T("f   %s"), FormatFreq(col * rd.freqResHz).GetString());
            lines.push_back(s);
            s.Format(_T("v   %+.2f m/s"), (row - rd.nDoppler / 2) * rd.velBinMps); lines.push_back(s);
            s.Format(_T("P   %s"), FormatDb(rd.db[static_cast<size_t>(row) * rd.nRange + col]).GetString()); lines.push_back(s);
            DrawReadout(dc, L.plot, m_mouse, lines);
        }
    }
}

void RangeDopplerView::OnPlotRButtonUp(CPoint pt, UINT)
{
    CMenu menu;
    menu.CreatePopupMenu();
    menu.AppendMenu(MF_STRING, ID_PLOT_COPY_IMAGE, _T("Copy image"));
    CPoint sp = pt; ClientToScreen(&sp);
    int cmd = menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD, sp.x, sp.y, this);
    if (cmd == ID_PLOT_COPY_IMAGE) CopyImageToClipboard();
}
