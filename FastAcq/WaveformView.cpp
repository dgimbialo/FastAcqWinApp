#include "pch.h"
#include "Lang.h"
#include "WaveformView.h"
#include "resource.h"

BEGIN_MESSAGE_MAP(WaveformView, PlotWnd)
    ON_WM_CREATE()
    ON_WM_HSCROLL()
    ON_WM_VSCROLL()
    ON_BN_CLICKED(IDC_WV_XM,  &WaveformView::OnBtnXMinus)
    ON_BN_CLICKED(IDC_WV_XP,  &WaveformView::OnBtnXPlus)
    ON_BN_CLICKED(IDC_WV_YM,  &WaveformView::OnBtnYMinus)
    ON_BN_CLICKED(IDC_WV_YP,  &WaveformView::OnBtnYPlus)
    ON_BN_CLICKED(IDC_WV_RST, &WaveformView::OnBtnReset)
END_MESSAGE_MAP()

BOOL WaveformView::CreateView(CWnd* parent, UINT id)
{
    return CreatePlot(parent, id, WS_HSCROLL | WS_VSCROLL);
}

int WaveformView::OnCreate(LPCREATESTRUCT lpcs)
{
    if (PlotWnd::OnCreate(lpcs) == -1) return -1;
    Dpi::MakeFont(m_btnFont, m_hWnd, 9, true);
    auto mkBtn = [&](CButton& b, UINT id, LPCTSTR text) {
        b.Create(text, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, CRect(0, 0, S(kBtnW96), S(kBtnH96)), this, id);
        b.SetFont(&m_btnFont);
    };
    mkBtn(m_btnXm,  IDC_WV_XM,  _T("H\u2212"));
    mkBtn(m_btnXp,  IDC_WV_XP,  _T("H+"));
    mkBtn(m_btnYm,  IDC_WV_YM,  _T("V\u2212"));
    mkBtn(m_btnYp,  IDC_WV_YP,  _T("V+"));
    mkBtn(m_btnRst, IDC_WV_RST, _T("\u21BA"));
    return 0;
}

// ---------------------------------------------------------------------------
CRect WaveformView::PlotRect() const
{
    CRect rc; GetClientRect(&rc);
    rc.top    += S(kToolH96);
    rc.left   += S(kAxisW96);
    rc.bottom -= S(kAxisH96);
    if (rc.bottom < rc.top + 2) rc.bottom = rc.top + 2;
    if (rc.right < rc.left + 2) rc.right = rc.left + 2;
    return rc;
}

size_t WaveformView::VisibleCount() const
{
    const size_t n = m_samples.size();
    if (n == 0) return 0;
    size_t vis = static_cast<size_t>(static_cast<double>(n) / m_zoomX);
    if (vis < 2) vis = 2;
    if (vis > n) vis = n;
    return vis;
}

void WaveformView::ClampOffsets()
{
    const size_t n = m_samples.size();
    const size_t vis = VisibleCount();
    const size_t maxOff = (n > vis) ? n - vis : 0;
    if (m_offsetX > maxOff) m_offsetX = maxOff;
    const double full = static_cast<double>(1u << m_adcBits);
    const double visY = full / m_zoomY;
    if (m_offsetY < 0.0) m_offsetY = 0.0;
    if (m_offsetY > full - visY) m_offsetY = full - visY;
    if (m_offsetY < 0.0) m_offsetY = 0.0;
}

void WaveformView::UpdateScrollBars()
{
    if (!m_hWnd) return;
    const size_t n = m_samples.size();
    if (n == 0 || m_zoomX <= 1.0) {
        EnableScrollBarCtrl(SB_HORZ, FALSE);
    } else {
        EnableScrollBarCtrl(SB_HORZ, TRUE);
        SCROLLINFO si{}; si.cbSize = sizeof(si);
        si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
        si.nMin  = 0;
        si.nMax  = static_cast<int>(n - 1);
        si.nPage = static_cast<UINT>(VisibleCount());
        si.nPos  = static_cast<int>(m_offsetX);
        SetScrollInfo(SB_HORZ, &si, TRUE);
    }
    if (m_zoomY <= 1.0) {
        EnableScrollBarCtrl(SB_VERT, FALSE);
    } else {
        EnableScrollBarCtrl(SB_VERT, TRUE);
        const int full = 1 << m_adcBits;
        const int page = static_cast<int>(full / m_zoomY);
        int posInv = full - page - static_cast<int>(m_offsetY);
        if (posInv < 0) posInv = 0;
        SCROLLINFO si{}; si.cbSize = sizeof(si);
        si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
        si.nMin  = 0;
        si.nMax  = full;
        si.nPage = static_cast<UINT>(page);
        si.nPos  = posInv;
        SetScrollInfo(SB_VERT, &si, TRUE);
    }
}

void WaveformView::SetSamples(const uint16_t* data, size_t n, size_t offset)
{
    m_samples.assign(data, data + n);
    m_offset = offset;
    ClampOffsets();
    UpdateScrollBars();
    if (::IsWindow(m_hWnd)) Invalidate(FALSE);
}

void WaveformView::SetSegments(const std::vector<dsp::Segment>& segs)
{
    m_segments = segs;
    if (m_segLabels.size() != m_segments.size()) m_segLabels.clear();
    if (::IsWindow(m_hWnd)) Invalidate(FALSE);
}

void WaveformView::SetSegmentLabels(const std::vector<CString>& labels)
{
    m_segLabels = labels;
    if (::IsWindow(m_hWnd)) Invalidate(FALSE);
}

void WaveformView::SetAdcConfig(int bits, float vRef, bool showVolts)
{
    if (bits < 8) bits = 8;
    if (bits > 16) bits = 16;
    m_adcBits = bits; m_vRef = vRef; m_showVolts = showVolts;
    ClampOffsets();
    UpdateScrollBars();
    if (::IsWindow(m_hWnd)) Invalidate(FALSE);
}

void WaveformView::ResetZoom()
{
    m_zoomX = 1.0; m_zoomY = 1.0; m_offsetX = 0; m_offsetY = 0.0;
    UpdateScrollBars();
    Invalidate(FALSE);
}

void WaveformView::ClearCursors()
{
    m_cA = Cursor{}; m_cB = Cursor{};
    Invalidate(FALSE);
}

void WaveformView::OnPlotSize(int, int)
{
    if (!m_btnXm.GetSafeHwnd()) return;
    const int y0 = (S(kToolH96) - S(kBtnH96)) / 2;
    int x = S(kAxisW96) + S(4);
    auto place = [&](CButton& b, int w) { b.MoveWindow(x, y0, w, S(kBtnH96)); x += w + S(3); };
    place(m_btnXm,  S(kBtnW96));
    place(m_btnXp,  S(kBtnW96));
    x += S(6);
    place(m_btnYm,  S(kBtnW96));
    place(m_btnYp,  S(kBtnW96));
    x += S(6);
    place(m_btnRst, S(kBtnW96) + S(4));
    UpdateScrollBars();
}

// ---------------------------------------------------------------------------
double WaveformView::SampleAtX(int x, const CRect& plot) const
{
    const size_t vis = VisibleCount();
    if (vis == 0 || plot.Width() <= 1) return 0.0;
    return static_cast<double>(m_offsetX) + static_cast<double>(x - plot.left) / plot.Width() * vis;
}

int WaveformView::XOfSample(double s, const CRect& plot) const
{
    const size_t vis = VisibleCount();
    if (vis == 0) return plot.left;
    return plot.left + static_cast<int>(std::lround((s - static_cast<double>(m_offsetX)) / vis * plot.Width()));
}

double WaveformView::CodesAtY(int y, const CRect& plot) const
{
    const double full = static_cast<double>(1u << m_adcBits);
    const double visY = full / m_zoomY;
    return m_offsetY + visY * (static_cast<double>(plot.bottom - y) / (std::max)(1, plot.Height()));
}

double WaveformView::CodeToUnit(double code) const
{
    if (!m_showVolts) return code;
    const double full = static_cast<double>(1u << m_adcBits);
    return code * m_vRef / full;
}

CString WaveformView::FormatUnit(double code) const
{
    CString s;
    if (m_showVolts) {
        const double v = CodeToUnit(code);
        if (std::fabs(v) < 0.1) s.Format(_T("%.2f mV"), v * 1000.0);
        else s.Format(_T("%.4f V"), v);
    } else {
        s.Format(_T("%.0f"), code);
    }
    return s;
}

// ---------------------------------------------------------------------------
void WaveformView::ZoomX(double factor, double anchorSample)
{
    const size_t n = m_samples.size();
    if (n < 2) return;
    double newZoom = m_zoomX * factor;
    if (newZoom < 1.0) newZoom = 1.0;
    if (newZoom > kMaxZoomX) newZoom = kMaxZoomX;
    const double visOld = static_cast<double>(n) / m_zoomX;
    const double visNew = static_cast<double>(n) / newZoom;
    const double rel = (visOld > 0.0) ? (anchorSample - static_cast<double>(m_offsetX)) / visOld : 0.5;
    double newOff = anchorSample - rel * visNew;
    if (newOff < 0.0) newOff = 0.0;
    m_zoomX = newZoom;
    m_offsetX = static_cast<size_t>(newOff);
    ClampOffsets();
    UpdateScrollBars();
    Invalidate(FALSE);
}

void WaveformView::ZoomY(double factor)
{
    const double full = static_cast<double>(1u << m_adcBits);
    const double visOld = full / m_zoomY;
    const double center = m_offsetY + visOld * 0.5;
    double z = m_zoomY * factor;
    if (z < 1.0) z = 1.0;
    if (z > kMaxZoomY) z = kMaxZoomY;
    m_zoomY = z;
    const double visNew = full / m_zoomY;
    m_offsetY = center - visNew * 0.5;
    ClampOffsets();
    UpdateScrollBars();
    Invalidate(FALSE);
}

void WaveformView::OnBtnXMinus() { ZoomX(0.5, static_cast<double>(m_offsetX) + VisibleCount() * 0.5); }
void WaveformView::OnBtnXPlus()  { ZoomX(2.0, static_cast<double>(m_offsetX) + VisibleCount() * 0.5); }
void WaveformView::OnBtnYMinus() { ZoomY(0.5); }
void WaveformView::OnBtnYPlus()  { ZoomY(2.0); }
void WaveformView::OnBtnReset()  { ResetZoom(); }

void WaveformView::OnHScroll(UINT nSBCode, UINT, CScrollBar* pBar)
{
    if (pBar) { PlotWnd::OnHScroll(nSBCode, 0, pBar); return; }
    const size_t n = m_samples.size(); if (n == 0) return;
    const size_t vis = VisibleCount();
    const size_t maxOff = (n > vis) ? n - vis : 0;
    size_t newOff = m_offsetX;
    switch (nSBCode) {
    case SB_LEFT:      newOff = 0; break;
    case SB_RIGHT:     newOff = maxOff; break;
    case SB_LINELEFT:  newOff = (newOff > vis / 20) ? newOff - vis / 20 : 0; break;
    case SB_LINERIGHT: newOff = (newOff + vis / 20 < maxOff) ? newOff + vis / 20 : maxOff; break;
    case SB_PAGELEFT:  newOff = (newOff > vis) ? newOff - vis : 0; break;
    case SB_PAGERIGHT: newOff = (newOff + vis < maxOff) ? newOff + vis : maxOff; break;
    case SB_THUMBTRACK:
    case SB_THUMBPOSITION: {
        SCROLLINFO si{}; si.cbSize = sizeof(si); si.fMask = SIF_TRACKPOS;
        GetScrollInfo(SB_HORZ, &si);
        newOff = static_cast<size_t>(si.nTrackPos);
        break;
    }
    default: return;
    }
    if (newOff > maxOff) newOff = maxOff;
    m_offsetX = newOff;
    SetScrollPos(SB_HORZ, static_cast<int>(newOff), TRUE);
    Invalidate(FALSE);
}

void WaveformView::OnVScroll(UINT nSBCode, UINT, CScrollBar* pBar)
{
    if (pBar) { PlotWnd::OnVScroll(nSBCode, 0, pBar); return; }
    if (m_zoomY <= 1.0) return;
    const int full = 1 << m_adcBits;
    const int page = static_cast<int>(full / m_zoomY);
    int posInv = GetScrollPos(SB_VERT);
    switch (nSBCode) {
    case SB_TOP:      posInv = 0; break;
    case SB_BOTTOM:   posInv = full - page; break;
    case SB_LINEUP:   posInv = (std::max)(0, posInv - page / 10); break;
    case SB_LINEDOWN: posInv = (std::min)(full - page, posInv + page / 10); break;
    case SB_PAGEUP:   posInv = (std::max)(0, posInv - page); break;
    case SB_PAGEDOWN: posInv = (std::min)(full - page, posInv + page); break;
    case SB_THUMBTRACK:
    case SB_THUMBPOSITION: {
        SCROLLINFO si{}; si.cbSize = sizeof(si); si.fMask = SIF_TRACKPOS;
        GetScrollInfo(SB_VERT, &si);
        posInv = si.nTrackPos;
        break;
    }
    default: return;
    }
    if (posInv < 0) posInv = 0;
    if (posInv > full - page) posInv = full - page;
    m_offsetY = static_cast<double>(full - page - posInv);
    ClampOffsets();
    SetScrollPos(SB_VERT, posInv, TRUE);
    Invalidate(FALSE);
}

BOOL WaveformView::OnPlotMouseWheel(UINT flags, short zDelta, CPoint pt)
{
    const CRect plot = PlotRect();
    if (flags & MK_CONTROL) {
        if (m_zoomY > 1.0) {
            const double full = static_cast<double>(1u << m_adcBits);
            const double step = (full / m_zoomY) * 0.1;
            m_offsetY += (zDelta > 0) ? step : -step;
            ClampOffsets(); UpdateScrollBars(); Invalidate(FALSE);
        }
    } else if (flags & MK_SHIFT) {
        ZoomY(zDelta > 0 ? 2.0 : 0.5);
    } else {
        const double anchor = plot.PtInRect(pt) ? SampleAtX(pt.x, plot)
                                                : static_cast<double>(m_offsetX) + VisibleCount() * 0.5;
        ZoomX(zDelta > 0 ? 2.0 : 0.5, anchor);
    }
    return TRUE;
}

void WaveformView::OnPlotLButtonDown(CPoint pt, UINT)
{
    const CRect plot = PlotRect();
    if (!plot.PtInRect(pt)) return;
    m_dragging = true; m_dragMoved = false; m_dragStart = pt; m_dragOffset = m_offsetX;
    SetCapture();
}

void WaveformView::OnPlotMouseMove(CPoint pt, UINT flags)
{
    if (m_dragging && (flags & MK_LBUTTON)) {
        const CRect plot = PlotRect();
        const int dx = pt.x - m_dragStart.x;
        if (std::abs(dx) > S(3)) m_dragMoved = true;
        if (m_dragMoved && plot.Width() > 0) {
            const double vis = static_cast<double>(VisibleCount());
            const double shift = -static_cast<double>(dx) / plot.Width() * vis;
            double off = static_cast<double>(m_dragOffset) + shift;
            if (off < 0.0) off = 0.0;
            m_offsetX = static_cast<size_t>(off);
            ClampOffsets();
            UpdateScrollBars();
        }
    }
    Invalidate(FALSE);
}

void WaveformView::OnPlotLButtonUp(CPoint pt, UINT flags)
{
    if (!m_dragging) return;
    m_dragging = false;
    if (GetCapture() == this) ReleaseCapture();
    if (m_dragMoved) return;
    const CRect plot = PlotRect();
    if (!plot.PtInRect(pt)) return;
    const double s = SampleAtX(pt.x, plot);
    if (flags & MK_SHIFT) { m_cB.valid = true; m_cB.sample = s; }
    else                  { m_cA.valid = true; m_cA.sample = s; }
    Invalidate(FALSE);
}

void WaveformView::OnPlotLButtonDblClk(CPoint, UINT)
{
    m_dragging = false;
    ResetZoom();
}

void WaveformView::OnPlotKeyDown(UINT nChar, UINT)
{
    switch (nChar) {
    case VK_ESCAPE: ClearCursors(); break;
    case VK_HOME:   ResetZoom(); break;
    case VK_LEFT:   OnHScroll(SB_LINELEFT, 0, nullptr); break;
    case VK_RIGHT:  OnHScroll(SB_LINERIGHT, 0, nullptr); break;
    case VK_ADD:    OnBtnXPlus(); break;
    case VK_SUBTRACT: OnBtnXMinus(); break;
    default: break;
    }
}

void WaveformView::OnPlotRButtonUp(CPoint pt, UINT)
{
    CMenu menu;
    menu.CreatePopupMenu();
    menu.AppendMenu(MF_STRING, ID_PLOT_RESET_ZOOM,    TR("Reset zoom\tHome"));
    menu.AppendMenu(MF_STRING, ID_PLOT_CLEAR_MARKERS, TR("Clear cursors\tEsc"));
    menu.AppendMenu(MF_SEPARATOR);
    menu.AppendMenu(MF_STRING, ID_PLOT_COPY_IMAGE,    TR("Copy image"));
    CPoint sp = pt; ClientToScreen(&sp);
    int cmd = menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD, sp.x, sp.y, this);
    switch (cmd) {
    case ID_PLOT_RESET_ZOOM:    ResetZoom(); break;
    case ID_PLOT_CLEAR_MARKERS: ClearCursors(); break;
    case ID_PLOT_COPY_IMAGE:    CopyImageToClipboard(); break;
    default: break;
    }
}

// ---------------------------------------------------------------------------
void WaveformView::DrawCursorReadout(CDC& dc, const CRect& plot)
{
    const Theme::Palette& th = Theme::Get();
    auto valueAt = [&](double s) -> double {
        if (m_samples.empty()) return 0.0;
        size_t i = static_cast<size_t>(s < 0.0 ? 0.0 : s);
        if (i >= m_samples.size()) i = m_samples.size() - 1;
        return static_cast<double>(m_samples[i] & ((1u << m_adcBits) - 1u));
    };
    int rx = plot.left + S(4);
    auto drawOne = [&](const Cursor& c, COLORREF col, LPCTSTR name) {
        if (!c.valid) return;
        const int x = XOfSample(c.sample, plot);
        if (x >= plot.left && x <= plot.right) {
            CPen pen(PS_DASH, 1, col);
            CPen* p = dc.SelectObject(&pen);
            dc.MoveTo(x, plot.top); dc.LineTo(x, plot.bottom);
            dc.SelectObject(p);
        }
        std::vector<CString> lines;
        CString s;
        const double t = (m_fs > 0.0) ? (c.sample + static_cast<double>(m_offset)) / m_fs : 0.0;
        s.Format(_T("%s  %s"), name, FormatTime(t).GetString()); lines.push_back(s);
        s.Format(_T("#%.0f  %s"), c.sample + static_cast<double>(m_offset), FormatUnit(valueAt(c.sample)).GetString()); lines.push_back(s);
        DrawReadout(dc, plot, CPoint(rx, plot.top - S(10)), lines);
        rx += S(130);
    };
    drawOne(m_cA, th.markerA, _T("A"));
    drawOne(m_cB, th.markerB, _T("B"));
    if (m_cA.valid && m_cB.valid && m_fs > 0.0) {
        std::vector<CString> lines;
        CString s;
        const double dt = (m_cB.sample - m_cA.sample) / m_fs;
        s.Format(_T("B-A  %s"), FormatTime(dt).GetString()); lines.push_back(s);
        if (std::fabs(dt) > 1e-12) { s.Format(_T("1/dt %s"), FormatFreq(1.0 / std::fabs(dt)).GetString()); lines.push_back(s); }
        s.Format(_T("dV   %s"), FormatUnit(valueAt(m_cB.sample) - valueAt(m_cA.sample)).GetString()); lines.push_back(s);
        DrawReadout(dc, plot, CPoint(rx, plot.top - S(10)), lines);
    }
    // Live cursor.
    if (m_mouseIn && plot.PtInRect(m_mouse)) {
        CPen pen(PS_DOT, 1, th.cursor);
        CPen* p = dc.SelectObject(&pen);
        dc.MoveTo(m_mouse.x, plot.top); dc.LineTo(m_mouse.x, plot.bottom);
        dc.SelectObject(p);
        const double s = SampleAtX(m_mouse.x, plot);
        std::vector<CString> lines;
        CString l;
        const double t = (m_fs > 0.0) ? (s + static_cast<double>(m_offset)) / m_fs : 0.0;
        l.Format(_T("t  %s"), FormatTime(t).GetString()); lines.push_back(l);
        l.Format(_T("#  %.0f"), s + static_cast<double>(m_offset)); lines.push_back(l);
        l.Format(_T("y  %s"), FormatUnit(CodesAtY(m_mouse.y, plot)).GetString()); lines.push_back(l);
        l.Format(_T("s  %s"), FormatUnit(valueAt(s)).GetString()); lines.push_back(l);
        DrawReadout(dc, plot, m_mouse, lines);
    }
}

void WaveformView::Render(CDC& dc, const CRect& full)
{
    const Theme::Palette& th = Theme::Get();
    const CRect plot = PlotRect();
    CRect tbRc(full.left, full.top, full.right, full.top + S(kToolH96));
    dc.FillSolidRect(full, th.plot);
    dc.FillSolidRect(tbRc, th.panel);
    CRect axRc(full.left, plot.top, plot.left, plot.bottom);
    dc.FillSolidRect(axRc, th.axisStrip);
    CRect txRc(plot.left, plot.bottom, plot.right, full.bottom);
    dc.FillSolidRect(txRc, th.axisStrip);
    dc.SetBkMode(TRANSPARENT);

    // Toolbar text: title, info, zoom.
    {
        CFont* pOld = dc.SelectObject(&m_fontTitle);
        const int btnAreaEnd = S(kAxisW96) + 5 * S(kBtnW96) + 4 * S(3) + 2 * S(6) + S(4) + S(10);
        dc.SetTextColor(th.textHdr);
        const int textRight = tbRc.right - S(140);
        const int titleW = (std::min)(static_cast<int>(dc.GetTextExtent(m_title).cx), (std::max)(0, textRight - btnAreaEnd));
        CRect tRc(btnAreaEnd, tbRc.top, btnAreaEnd + titleW + S(2), tbRc.bottom);
        dc.DrawText(m_title, tRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        if (!m_info.IsEmpty() && tRc.right + S(24) < textRight) {
            dc.SelectObject(&m_fontLabel);
            dc.SetTextColor(th.peak);
            CRect iRc(tRc.right + S(16), tbRc.top, textRight, tbRc.bottom);
            dc.DrawText(m_info, iRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        }
        dc.SelectObject(&m_fontAxis);
        dc.SetTextColor(th.textDim);
        CString z; z.Format(_T("H x%.0f   V x%.0f"), m_zoomX, m_zoomY);
        CRect zRc(full.right - S(135), tbRc.top, full.right - S(4), tbRc.bottom);
        dc.DrawText(z, zRc, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
        dc.SelectObject(pOld);
    }

    if (plot.Width() <= 2 || plot.Height() <= 2) return;

    const size_t n   = m_samples.size();
    const size_t vis = VisibleCount();
    const size_t i0  = m_offsetX;
    const size_t i1  = (std::min)(n, i0 + vis);

    // Segment shading: UP / DOWN / guard bands, a boundary line at every
    // segment start and a caption inside the band (what the DSP took as
    // this ramp: samples, duration, beat cycles). Samples after the last
    // segment belong to no chirp and stay on the plain background.
    if (n > 0) {
        CPen boundary(PS_SOLID, 1, th.axis);
        CFont* pOldF = dc.SelectObject(&m_fontAxis);
        dc.SetBkMode(TRANSPARENT);
        for (size_t k = 0; k < m_segments.size(); ++k) {
            const auto& sg = m_segments[k];
            const double a = static_cast<double>(sg.start) - static_cast<double>(m_offset);
            const double b = a + static_cast<double>(sg.length);
            if (b <= static_cast<double>(i0) || a >= static_cast<double>(i1)) continue;
            const bool startVisible = a >= static_cast<double>(i0);
            int xa = XOfSample((std::max)(a, static_cast<double>(i0)), plot);
            int xb = XOfSample((std::min)(b, static_cast<double>(i1)), plot);
            if (xb <= xa) xb = xa + 1;
            COLORREF c = (sg.kind == dsp::Segment::Up) ? th.segUp : (sg.kind == dsp::Segment::Down) ? th.segDn : th.segGuard;
            dc.FillSolidRect(xa, plot.top, xb - xa, plot.Height(), c);
            if (startVisible) {
                CPen* op = dc.SelectObject(&boundary);
                dc.MoveTo(xa, plot.top); dc.LineTo(xa, plot.bottom);
                dc.SelectObject(op);
            }
            if (k < m_segLabels.size() && !m_segLabels[k].IsEmpty()) {
                const CString& lbl = m_segLabels[k];
                const int tw = dc.GetTextExtent(lbl).cx;
                if (xb - xa >= tw + S(8)) {
                    dc.SetTextColor(sg.kind == dsp::Segment::Up ? th.traceUp : sg.kind == dsp::Segment::Down ? th.traceDn : th.textDim);
                    dc.TextOut(xa + S(4), plot.top + S(2), lbl);
                } else if (xb - xa >= S(14) && sg.kind != dsp::Segment::Guard) {
                    dc.SetTextColor(sg.kind == dsp::Segment::Up ? th.traceUp : th.traceDn);
                    dc.TextOut(xa + S(3), plot.top + S(2), sg.kind == dsp::Segment::Up ? TR("UP") : CString(_T("DN")));
                }
            }
        }
        dc.SelectObject(pOldF);
    }

    // Y axis + grid.
    const double fullCodes = static_cast<double>(1u << m_adcBits);
    LinearAxis ya;
    ya.px0 = plot.bottom; ya.px1 = plot.top;
    ya.v0  = CodeToUnit(m_offsetY);
    ya.v1  = CodeToUnit(m_offsetY + fullCodes / m_zoomY);
    DrawYAxis(dc, plot, axRc, ya, [this](double v) {
        CString s;
        if (m_showVolts) s.Format(std::fabs(v) < 0.1 && v != 0.0 ? _T("%.3f") : _T("%.3f"), v);
        else s.Format(_T("%.0f"), v);
        return s; }, true);
    {
        CFont* pOld = dc.SelectObject(&m_fontAxis);
        dc.SetTextColor(th.textDim);
        dc.TextOut(full.left + S(4), plot.top + S(2), m_showVolts ? _T("V") : _T("code"));
        dc.SelectObject(pOld);
    }
    // Time axis.
    if (m_fs > 0.0 && n > 0) {
        LinearAxis xa;
        xa.px0 = plot.left; xa.px1 = plot.right;
        xa.v0 = (static_cast<double>(i0) + m_offset) / m_fs;
        xa.v1 = (static_cast<double>(i0) + vis + m_offset) / m_fs;
        const double span = xa.v1 - xa.v0;
        DrawXAxis(dc, plot, txRc, xa, [span](double v) {
            CString s;
            if (span >= 1e-2) s.Format(_T("%.3f ms"), v * 1e3);
            else if (span >= 1e-5) s.Format(_T("%.1f \u00B5s"), v * 1e6);
            else s.Format(_T("%.0f ns"), v * 1e9);
            return s; }, false, true);
    }

    // Plot frame.
    {
        CPen pen(PS_SOLID, 1, th.axis);
        CPen* p = dc.SelectObject(&pen);
        CBrush* b = static_cast<CBrush*>(dc.SelectStockObject(NULL_BRUSH));
        dc.Rectangle(plot.left, plot.top, plot.right + 1, plot.bottom + 1);
        if (b) dc.SelectObject(b);
        dc.SelectObject(p);
    }

    if (n == 0) return;

    const int    w = plot.Width() - 2;
    const int    h = plot.Height() - 2;
    const uint16_t mask = static_cast<uint16_t>((1u << m_adcBits) - 1u);
    const double visY = fullCodes / m_zoomY;
    auto codeToY = [&](double code) -> int {
        double norm = (code - m_offsetY) / visY;
        if (norm < 0.0) norm = 0.0;
        if (norm > 1.0) norm = 1.0;
        return plot.bottom - 1 - static_cast<int>(norm * h);
    };

    std::vector<POINT> pts;
    const size_t cnt = i1 - i0;
    if (cnt <= static_cast<size_t>(w)) {
        pts.resize(cnt);
        for (size_t i = 0; i < cnt; ++i) {
            int x = plot.left + 1 + (cnt > 1 ? static_cast<int>((i * w) / (cnt - 1)) : 0);
            pts[i] = { x, codeToY(m_samples[i0 + i] & mask) };
        }
    } else {
        pts.reserve(static_cast<size_t>(w) * 2);
        for (int col = 0; col < w; ++col) {
            size_t c0 = i0 + (static_cast<size_t>(col) * cnt) / w;
            size_t c1 = i0 + (static_cast<size_t>(col + 1) * cnt) / w;
            if (c1 <= c0) c1 = c0 + 1;
            if (c1 > i1) c1 = i1;
            uint16_t mn = m_samples[c0] & mask, mx = mn;
            for (size_t i = c0 + 1; i < c1; ++i) {
                uint16_t v = m_samples[i] & mask;
                if (v < mn) mn = v;
                if (v > mx) mx = v;
            }
            int x = plot.left + 1 + col;
            pts.push_back({ x, codeToY(mx) });
            pts.push_back({ x, codeToY(mn) });
        }
    }

    CRgn clip; clip.CreateRectRgnIndirect(&plot);
    dc.SelectClipRgn(&clip);
    if (m_dots) {
        for (const auto& pt : pts) dc.FillSolidRect(pt.x - 1, pt.y - 1, 2, 2, th.wave);
    } else if (pts.size() >= 2) {
        CPen pen(PS_SOLID, 1, th.wave);
        CPen* p = dc.SelectObject(&pen);
        dc.Polyline(pts.data(), static_cast<int>(pts.size()));
        dc.SelectObject(p);
    }
    DrawCursorReadout(dc, plot);
    dc.SelectClipRgn(nullptr);
}
