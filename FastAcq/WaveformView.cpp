#include "pch.h"
#include "WaveformView.h"

#include <algorithm>

BEGIN_MESSAGE_MAP(WaveformView, CWnd)
    ON_WM_CREATE()
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
    ON_WM_SIZE()
    ON_WM_HSCROLL()
    ON_WM_VSCROLL()
    ON_WM_MOUSEWHEEL()
    ON_BN_CLICKED(ID_XM,  &WaveformView::OnBtnXMinus)
    ON_BN_CLICKED(ID_XP,  &WaveformView::OnBtnXPlus)
    ON_BN_CLICKED(ID_YM,  &WaveformView::OnBtnYMinus)
    ON_BN_CLICKED(ID_YP,  &WaveformView::OnBtnYPlus)
    ON_BN_CLICKED(ID_RST, &WaveformView::OnBtnReset)
END_MESSAGE_MAP()

// ---------------------------------------------------------------------------
BOOL WaveformView::CreateView(CWnd* parent, UINT id)
{
    LPCTSTR cls = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW,
                                      ::LoadCursor(nullptr, IDC_ARROW),
                                      reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1),
                                      nullptr);
    return Create(cls, nullptr,
                  WS_CHILD | WS_VISIBLE | WS_BORDER | WS_HSCROLL | WS_VSCROLL,
                  CRect(0, 0, 10, 10), parent, id);
}

int WaveformView::OnCreate(LPCREATESTRUCT lpcs)
{
    if (CWnd::OnCreate(lpcs) == -1) return -1;

    m_btnFont.CreateFont(-11, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                         DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                         CLEARTYPE_QUALITY, VARIABLE_PITCH | FF_SWISS, _T("Segoe UI"));

    m_axisFont.CreateFont(-10, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                          DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                          CLEARTYPE_QUALITY, VARIABLE_PITCH | FF_SWISS, _T("Segoe UI"));

    m_freqFont.CreateFont(-13, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                          DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                          CLEARTYPE_QUALITY, VARIABLE_PITCH | FF_SWISS, _T("Segoe UI"));

    auto mkBtn = [&](CButton& b, UINT id, LPCTSTR text) {
        b.Create(text, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                 CRect(0, 0, kBtnW, kBtnH), this, id);
        b.SetFont(&m_btnFont);
    };
    mkBtn(m_btnXm,  ID_XM,  _T("H\u2212"));
    mkBtn(m_btnXp,  ID_XP,  _T("H+"));
    mkBtn(m_btnYm,  ID_YM,  _T("V\u2212"));
    mkBtn(m_btnYp,  ID_YP,  _T("V+"));
    mkBtn(m_btnRst, ID_RST, _T("\u21BA"));
    return 0;
}

// ---------------------------------------------------------------------------
CRect WaveformView::PlotRect() const
{
    CRect rc; const_cast<WaveformView*>(this)->GetClientRect(&rc);
    rc.top  += kToolH;
    rc.left += kAxisW;
    return rc;
}

void WaveformView::UpdateScrollBar()
{
    if (!m_hWnd) return;
    const size_t n = m_samples.size();
    if (n == 0 || m_zoomX <= 1.0f) { EnableScrollBarCtrl(SB_HORZ, FALSE); return; }
    EnableScrollBarCtrl(SB_HORZ, TRUE);

    size_t visible = static_cast<size_t>(n / m_zoomX);
    if (visible < 1) visible = 1;

    SCROLLINFO si{}; si.cbSize = sizeof(si);
    si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    si.nMin  = 0;
    si.nMax  = static_cast<int>(n - 1);
    si.nPage = static_cast<UINT>(visible);
    si.nPos  = static_cast<int>(m_offsetX);
    SetScrollInfo(SB_HORZ, &si, TRUE);
}

void WaveformView::ClampOffsetY()
{
    float visRange = m_vRef / m_zoomY;
    float maxOff   = m_vRef - visRange;
    if (maxOff < 0.0f) maxOff = 0.0f;
    if (m_offsetY < 0.0f)    m_offsetY = 0.0f;
    if (m_offsetY > maxOff)  m_offsetY = maxOff;
}

void WaveformView::UpdateVScrollBar()
{
    if (!m_hWnd) return;
    if (m_zoomY <= 1.0f) { EnableScrollBarCtrl(SB_VERT, FALSE); return; }
    EnableScrollBarCtrl(SB_VERT, TRUE);

    // Use integer units = 0.1 mV (multiply volts by 10000) for scroll precision.
    const int scale    = 10000;
    int       total    = static_cast<int>(m_vRef   * scale);
    int       page     = static_cast<int>((m_vRef / m_zoomY) * scale);
    int       pos      = static_cast<int>(m_offsetY * scale);
    // Scrollbar pos=0 → view bottom at 0V; pos=max → view top at vRef.
    // Invert so scrolling UP shows higher voltages.
    int       posInv   = total - page - pos;
    if (posInv < 0) posInv = 0;

    SCROLLINFO si{}; si.cbSize = sizeof(si);
    si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    si.nMin  = 0;
    si.nMax  = total;
    si.nPage = static_cast<UINT>(page);
    si.nPos  = posInv;
    SetScrollInfo(SB_VERT, &si, TRUE);
}

size_t WaveformView::DefaultVisibleSamples() const
{
    if (!m_hWnd) return 1024;

    CRect rc = PlotRect();
    int plotWidth = rc.Width() - 2;
    if (plotWidth < 1) plotWidth = 1;

    size_t targetVisible = static_cast<size_t>(plotWidth);
    if (targetVisible < 256U) targetVisible = 256U;
    if (targetVisible > 1024U) targetVisible = 1024U;
    return targetVisible;
}

void WaveformView::ApplyDefaultHorizontalZoom()
{
    const size_t n = m_samples.size();
    const size_t targetVisible = DefaultVisibleSamples();

    if (n > targetVisible) {
        m_zoomX = static_cast<float>(n) / static_cast<float>(targetVisible);
    } else {
        m_zoomX = 1.0f;
    }
    m_offsetX = 0;
}

void WaveformView::SetSamples(const uint16_t* data, size_t n)
{
    m_samples.assign(data, data + n);

    // Preserve zoom — only clamp offsetX so it stays in valid range.
    size_t visible = (m_zoomX > 1.0f) ? static_cast<size_t>(n / m_zoomX) : n;
    if (visible < 1) visible = 1;
    if (n > 0 && m_offsetX + visible > n)
        m_offsetX = (n > visible) ? n - visible : 0;

    UpdateScrollBar();
    if (::IsWindow(m_hWnd)) Invalidate(FALSE);
}

// ---------------------------------------------------------------------------
void WaveformView::OnSize(UINT t, int cx, int cy)
{
    CWnd::OnSize(t, cx, cy);
    if (!m_btnXm.GetSafeHwnd()) return;

    const int y0 = (kToolH - kBtnH) / 2;
    int x = kAxisW + 4;
    auto place = [&](CButton& b, int w) { b.MoveWindow(x, y0, w, kBtnH); x += w + 3; };
    place(m_btnXm,  kBtnW);
    place(m_btnXp,  kBtnW);
    x += 6;
    place(m_btnYm,  kBtnW);
    place(m_btnYp,  kBtnW);
    x += 6;
    place(m_btnRst, kBtnW + 4);

    UpdateScrollBar();
    Invalidate(FALSE);
}

// ---------------------------------------------------------------------------
void WaveformView::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pBar)
{
    if (pBar) { CWnd::OnHScroll(nSBCode, nPos, pBar); return; }
    const size_t n = m_samples.size(); if (n == 0) return;
    size_t visible = static_cast<size_t>(n / m_zoomX); if (visible < 1) visible = 1;
    size_t maxOff  = (n > visible) ? n - visible : 0;
    size_t newOff  = m_offsetX;
    switch (nSBCode) {
    case SB_LEFT:          newOff = 0;       break;
    case SB_RIGHT:         newOff = maxOff;  break;
    case SB_LINELEFT:      newOff = (newOff > 1)       ? newOff - 1        : 0;      break;
    case SB_LINERIGHT:     newOff = (newOff < maxOff)  ? newOff + 1        : maxOff; break;
    case SB_PAGELEFT:      newOff = (newOff > visible) ? newOff - visible  : 0;      break;
    case SB_PAGERIGHT:     newOff = (newOff + visible < maxOff) ? newOff + visible : maxOff; break;
    case SB_THUMBTRACK:
    case SB_THUMBPOSITION: { SCROLLINFO si{}; si.cbSize = sizeof(si); si.fMask = SIF_TRACKPOS;
                             GetScrollInfo(SB_HORZ, &si); newOff = static_cast<size_t>(si.nTrackPos); break; }
    default: return;
    }
    if (newOff > maxOff) newOff = maxOff;
    m_offsetX = newOff;
    SetScrollPos(SB_HORZ, static_cast<int>(newOff), TRUE);
    Invalidate(FALSE);
}

void WaveformView::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pBar)
{
    if (pBar) { CWnd::OnVScroll(nSBCode, nPos, pBar); return; }
    if (m_zoomY <= 1.0f) return;

    const int scale  = 10000;
    int total        = static_cast<int>(m_vRef * scale);
    int page         = static_cast<int>((m_vRef / m_zoomY) * scale);
    int posInv       = GetScrollPos(SB_VERT);  // current inverted pos

    switch (nSBCode) {
    case SB_TOP:          posInv = 0;                break;
    case SB_BOTTOM:       posInv = total - page;     break;
    case SB_LINEUP:       posInv = (std::max)(0, posInv - page / 10); break;
    case SB_LINEDOWN:     posInv = (std::min)(total - page, posInv + page / 10); break;
    case SB_PAGEUP:       posInv = (std::max)(0, posInv - page);      break;
    case SB_PAGEDOWN:     posInv = (std::min)(total - page, posInv + page);      break;
    case SB_THUMBTRACK:
    case SB_THUMBPOSITION: { SCROLLINFO si{}; si.cbSize = sizeof(si); si.fMask = SIF_TRACKPOS;
                             GetScrollInfo(SB_VERT, &si); posInv = si.nTrackPos; break; }
    default: return;
    }
    if (posInv < 0) posInv = 0;
    if (posInv > total - page) posInv = total - page;

    // Convert inverted scrollbar position back to m_offsetY.
    m_offsetY = static_cast<float>(total - page - posInv) / static_cast<float>(scale);
    ClampOffsetY();
    SetScrollPos(SB_VERT, posInv, TRUE);
    Invalidate(FALSE);
}

BOOL WaveformView::OnMouseWheel(UINT fFlags, short zDelta, CPoint)
{
    if (fFlags & MK_CONTROL) {
        // Ctrl+Wheel → vertical pan
        if (m_zoomY > 1.0f) {
            float step = (m_vRef / m_zoomY) * 0.1f;
            if (zDelta > 0) m_offsetY += step; else m_offsetY -= step;
            ClampOffsetY();
            UpdateVScrollBar();
            Invalidate(FALSE);
        }
    } else if (fFlags & MK_SHIFT) {
        // Shift+Wheel → vertical zoom
        if (zDelta > 0) OnBtnYPlus(); else OnBtnYMinus();
    } else {
        // plain Wheel → horizontal zoom
        if (zDelta > 0) OnBtnXPlus(); else OnBtnXMinus();
    }
    return TRUE;
}

// ---------------------------------------------------------------------------
void WaveformView::OnBtnXMinus()
{
    if (m_zoomX <= 1.0f) return;
    m_zoomX = (m_zoomX <= 2.0f) ? 1.0f : m_zoomX / 2.0f;
    if (m_zoomX <= 1.0f) m_offsetX = 0;
    UpdateScrollBar(); Invalidate(FALSE);
}
void WaveformView::OnBtnXPlus()
{
    if (m_zoomX >= kMaxZoomX) return;
    m_zoomX = (m_zoomX < 1.0f) ? 2.0f : m_zoomX * 2.0f;
    const size_t n = m_samples.size();
    if (n) { size_t vis = static_cast<size_t>(n / m_zoomX); if (vis < 1) vis = 1;
             size_t mx  = (n > vis) ? n - vis : 0; if (m_offsetX > mx) m_offsetX = mx; }
    UpdateScrollBar(); Invalidate(FALSE);
}
void WaveformView::OnBtnYMinus()
{
    if (m_zoomY <= 1.0f) return;
    // Zoom out around current view center.
    float visRange = m_vRef / m_zoomY;
    float center   = m_offsetY + visRange * 0.5f;
    m_zoomY = (m_zoomY <= 2.0f) ? 1.0f : m_zoomY / 2.0f;
    float newRange = m_vRef / m_zoomY;
    m_offsetY = center - newRange * 0.5f;
    ClampOffsetY();
    UpdateVScrollBar(); Invalidate(FALSE);
}
void WaveformView::OnBtnYPlus()
{
    if (m_zoomY >= kMaxZoomY) return;
    // Zoom in around current view center.
    float visRange = m_vRef / m_zoomY;
    float center   = m_offsetY + visRange * 0.5f;
    m_zoomY = (m_zoomY < 1.0f) ? 2.0f : m_zoomY * 2.0f;
    float newRange = m_vRef / m_zoomY;
    m_offsetY = center - newRange * 0.5f;
    ClampOffsetY();
    UpdateVScrollBar(); Invalidate(FALSE);
}
void WaveformView::OnBtnReset()
{
    ApplyDefaultHorizontalZoom();
    m_zoomY = 1.0f;
    m_offsetY = 0.0f;
    UpdateScrollBar(); UpdateVScrollBar(); Invalidate(FALSE);
}

// ---------------------------------------------------------------------------
void WaveformView::OnPaint()
{
    CPaintDC dc(this);
    CRect rc; GetClientRect(&rc);
    if (rc.Width() <= 2 || rc.Height() <= 2) return;

    CDC mem; mem.CreateCompatibleDC(&dc);
    CBitmap bmp; bmp.CreateCompatibleBitmap(&dc, rc.Width(), rc.Height());
    CBitmap* pOld = mem.SelectObject(&bmp);
    Render(mem, rc);
    dc.BitBlt(0, 0, rc.Width(), rc.Height(), &mem, 0, 0, SRCCOPY);
    mem.SelectObject(pOld);
}

// ---------------------------------------------------------------------------
void WaveformView::DrawYAxis(CDC& dc, const CRect& plotRc, float vTop, float vBot)
{
    // Y-axis strip is plotRc.left - kAxisW .. plotRc.left
    CRect axRc(plotRc.left - kAxisW, plotRc.top, plotRc.left, plotRc.bottom);
    dc.FillSolidRect(axRc, ::GetSysColor(COLOR_WINDOW));

    CFont* pOldFont = dc.SelectObject(&m_axisFont);
    dc.SetBkMode(TRANSPARENT);
    dc.SetTextColor(RGB(80,80,80));

    const int divisions = 10;
    const float vStep   = (vTop - vBot) / divisions;   // V per division

    for (int i = 0; i <= divisions; ++i) {
        float v = vTop - i * vStep;
        int y   = plotRc.top + (plotRc.Height() * i / divisions);

        // Tick mark.
        CPen tickPen(PS_SOLID, 1, RGB(90,90,90));
        CPen* pOp = dc.SelectObject(&tickPen);
        dc.MoveTo(plotRc.left - 5, y);
        dc.LineTo(plotRc.left,     y);
        dc.SelectObject(pOp);

        // Label � choose mV or V.
        CString lbl;
        if (fabsf(vTop - vBot) < 0.1f) {
            lbl.Format(_T("%+.1f"), v * 1000.0f);    // mV
            if (i == 0) lbl += _T("mV");
        } else {
            lbl.Format(_T("%.3f"), v);
            if (i == 0) lbl += _T("V");
        }

        CRect lrc(axRc.left, y - 8, axRc.right - 3, y + 8);
        dc.DrawText(lbl, lrc, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
    }
    dc.SelectObject(pOldFont);

    // Vertical axis line.
    CPen axisPen(PS_SOLID, 1, RGB(90,90,90));
    CPen* pOp = dc.SelectObject(&axisPen);
    dc.MoveTo(plotRc.left - 1, plotRc.top);
    dc.LineTo(plotRc.left - 1, plotRc.bottom);
    dc.SelectObject(pOp);
}

void WaveformView::Render(CDC& dc, const CRect& full)
{
    CRect tbRc(full.left, full.top, full.right, full.top + kToolH);
    dc.FillSolidRect(tbRc, ::GetSysColor(COLOR_BTNFACE));

    dc.SetBkMode(TRANSPARENT);
    CFont* pOldFont = dc.SelectObject(&m_btnFont);
    CString zInfo;
    zInfo.Format(_T("Hx%.0f   Vx%.0f"), m_zoomX, m_zoomY);
    CRect zoomRc(full.right - 130, full.top, full.right - 4, full.top + kToolH);
    dc.SetTextColor(::GetSysColor(COLOR_WINDOWTEXT));
    dc.DrawText(zInfo, zoomRc, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);

    int btnAreaEnd = kAxisW + 5 * kBtnW + 4 * 3 + 2 * 6 + (kBtnW + 4) + 10;

    if (!m_title.IsEmpty()) {
        CRect tRc(btnAreaEnd, full.top, full.right - 140, full.top + kToolH);
        dc.SetTextColor(::GetSysColor(COLOR_WINDOWTEXT));
        dc.DrawText(m_title, tRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }

    if (m_freqHz > 1.0f) {
        long long hz = static_cast<long long>(m_freqHz + 0.5f);
        CString freqStr;
        if (hz >= 1000)
            freqStr.Format(_T("%lld.%03lld kHz"), hz / 1000, hz % 1000);
        else
            freqStr.Format(_T("%lld Hz"), hz);

        if (m_sampleRateHz > 0) {
            uint32_t spp = static_cast<uint32_t>(static_cast<float>(m_sampleRateHz) / m_freqHz + 0.5f);
            CString ext;
            ext.Format(_T("%s   |   %u samp/period"), freqStr.GetString(), spp);
            freqStr = ext;
        }

        CFont* pFreqOld = dc.SelectObject(&m_freqFont);
        dc.SetTextColor(RGB(180,90,0));
        CRect freqRc(btnAreaEnd, full.top, full.right - 140, full.top + kToolH);
        dc.DrawText(freqStr, freqRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        dc.SelectObject(pFreqOld);
    }
    dc.SelectObject(pOldFont);

    const CRect rc = PlotRect();
    dc.FillSolidRect(rc, ::GetSysColor(COLOR_WINDOW));

    const uint16_t adcMask = static_cast<uint16_t>((1u << m_adcBits) - 1u);
    const float    adcMax  = static_cast<float>(adcMask);
    const float visRange = m_vRef / m_zoomY;
    const float vBot = m_offsetY;
    const float vTop = m_offsetY + visRange;

    DrawYAxis(dc, rc, vTop, vBot);

    if (rc.Width() <= 2 || rc.Height() <= 2) return;

    CPen gridPen(PS_SOLID, 1, RGB(210,210,210));
    CPen* pOldPen = dc.SelectObject(&gridPen);
    for (int i = 1; i < 10; ++i) {
        int y = rc.top + rc.Height() * i / 10;
        dc.MoveTo(rc.left, y); dc.LineTo(rc.right, y);
    }
    for (int i = 1; i < 10; ++i) {
        int x = rc.left + rc.Width() * i / 10;
        dc.MoveTo(x, rc.top); dc.LineTo(x, rc.bottom);
    }
    dc.SelectObject(pOldPen);

    if (m_samples.empty()) return;

    const int    w = rc.Width() - 2;
    const int    h = rc.Height() - 2;
    const size_t n = m_samples.size();

    size_t visCount = (m_zoomX > 1.0f) ? static_cast<size_t>(n / m_zoomX) : n;
    if (visCount < 1) visCount = 1;
    if (visCount > n) visCount = n;
    size_t i0 = m_offsetX;
    size_t i1 = i0 + visCount;
    if (i1 > n) { i1 = n; i0 = (n > visCount) ? n - visCount : 0; }

    const float vRange = vTop - vBot;

    auto sampleToNorm = [&](uint16_t s) -> float {
        float v = static_cast<float>(s & adcMask) * m_vRef / adcMax;
        float norm = (v - vBot) / vRange;
        if (norm < 0.0f) norm = 0.0f;
        if (norm > 1.0f) norm = 1.0f;
        return norm;
    };
    auto normToY = [&](float norm) -> int {
        return rc.bottom - 1 - static_cast<int>(norm * h);
    };

    CPen wavePen(PS_SOLID, 1, RGB(0,90,180));
    CPen* pOp = dc.SelectObject(&wavePen);

    std::vector<POINT> pts;
    const size_t vis = i1 - i0;

    if (vis <= static_cast<size_t>(w)) {
        pts.resize(vis);
        for (size_t i = 0; i < vis; ++i) {
            int x = rc.left + 1 + (vis > 1 ? static_cast<int>((i * w) / (vis - 1)) : 0);
            pts[i] = { x, normToY(sampleToNorm(m_samples[i0 + i])) };
        }
    } else {
        pts.reserve(static_cast<size_t>(w) * 2);
        for (int col = 0; col < w; ++col) {
            size_t ci0 = i0 + (static_cast<size_t>(col) * vis) / w;
            size_t ci1 = i0 + (static_cast<size_t>(col + 1) * vis) / w;
            if (ci1 <= ci0) ci1 = ci0 + 1;
            if (ci1 > i1)   ci1 = i1;
            uint16_t mn = m_samples[ci0] & adcMask, mx = m_samples[ci0] & adcMask;
            for (size_t i = ci0 + 1; i < ci1; ++i) {
                uint16_t v = m_samples[i] & adcMask;
                if (v < mn) mn = v;
                if (v > mx) mx = v;
            }
            int x = rc.left + 1 + col;
            pts.push_back({ x, normToY(sampleToNorm(mx)) });
            pts.push_back({ x, normToY(sampleToNorm(mn)) });
        }
    }

    if (m_dotsMode) {
        // Dots mode: draw each sample as a small filled circle (2x2 pixels).
        for (const auto& pt : pts) {
            dc.FillSolidRect(pt.x - 1, pt.y - 1, 2, 2, RGB(0,90,180));
        }
    } else {
        // Line mode: connect samples with polyline.
        if (pts.size() >= 2)
            dc.Polyline(pts.data(), static_cast<int>(pts.size()));
    }

    dc.SelectObject(pOp);
}

