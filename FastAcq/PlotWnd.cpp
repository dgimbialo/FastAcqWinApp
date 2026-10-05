#include "pch.h"
#include "PlotWnd.h"

BEGIN_MESSAGE_MAP(PlotWnd, CWnd)
    ON_WM_CREATE()
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
    ON_WM_SIZE()
    ON_WM_MOUSEMOVE()
    ON_WM_MOUSELEAVE()
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_WM_LBUTTONDBLCLK()
    ON_WM_RBUTTONUP()
    ON_WM_MOUSEWHEEL()
    ON_WM_KEYDOWN()
    ON_WM_GETDLGCODE()
    ON_MESSAGE(WM_DPICHANGED_AFTERPARENT, &PlotWnd::OnDpiChangedAfterParent)
END_MESSAGE_MAP()

BOOL PlotWnd::CreatePlot(CWnd* parent, UINT id, DWORD extraStyle)
{
    LPCTSTR cls = AfxRegisterWndClass(CS_DBLCLKS, ::LoadCursor(nullptr, IDC_ARROW), nullptr, nullptr);
    return Create(cls, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | extraStyle,
                  CRect(0, 0, 10, 10), parent, id);
}

int PlotWnd::OnCreate(LPCREATESTRUCT lpcs)
{
    if (CWnd::OnCreate(lpcs) == -1) return -1;
    RecreateFonts();
    return 0;
}

void PlotWnd::RecreateFonts()
{
    Dpi::MakeFont(m_fontAxis,  m_hWnd, 8);
    Dpi::MakeFont(m_fontLabel, m_hWnd, 9);
    Dpi::MakeFont(m_fontTitle, m_hWnd, 10, true);
    Dpi::MakeMonoFont(m_fontMono, m_hWnd, 9);
}

void PlotWnd::OnDpiChanged()
{
    RecreateFonts();
    Invalidate(FALSE);
}

LRESULT PlotWnd::OnDpiChangedAfterParent(WPARAM, LPARAM)
{
    OnDpiChanged();
    return 0;
}

void PlotWnd::OnPaint()
{
    CPaintDC dc(this);
    CRect rc; GetClientRect(&rc);
    if (rc.Width() <= 2 || rc.Height() <= 2) return;

    CDC mem;
    mem.CreateCompatibleDC(&dc);
    CBitmap bmp;
    bmp.CreateCompatibleBitmap(&dc, rc.Width(), rc.Height());
    CBitmap* pOld = mem.SelectObject(&bmp);
    mem.FillSolidRect(rc, Theme::Get().plot);
    Render(mem, rc);
    dc.BitBlt(0, 0, rc.Width(), rc.Height(), &mem, 0, 0, SRCCOPY);
    mem.SelectObject(pOld);
}

void PlotWnd::CopyImageToClipboard()
{
    CRect rc; GetClientRect(&rc);
    if (rc.Width() <= 2 || rc.Height() <= 2) return;
    CClientDC dc(this);
    CDC mem;
    mem.CreateCompatibleDC(&dc);
    CBitmap bmp;
    bmp.CreateCompatibleBitmap(&dc, rc.Width(), rc.Height());
    CBitmap* pOld = mem.SelectObject(&bmp);
    mem.FillSolidRect(rc, Theme::Get().plot);
    Render(mem, rc);
    mem.SelectObject(pOld);
    if (::OpenClipboard(m_hWnd)) {
        ::EmptyClipboard();
        ::SetClipboardData(CF_BITMAP, bmp.Detach());
        ::CloseClipboard();
    }
}

void PlotWnd::OnSize(UINT, int cx, int cy)
{
    OnPlotSize(cx, cy);
    Invalidate(FALSE);
}

void PlotWnd::OnMouseMove(UINT flags, CPoint pt)
{
    if (!m_tracking) {
        TRACKMOUSEEVENT tme{};
        tme.cbSize    = sizeof(tme);
        tme.dwFlags   = TME_LEAVE;
        tme.hwndTrack = m_hWnd;
        ::TrackMouseEvent(&tme);
        m_tracking = true;
    }
    m_mouse   = pt;
    m_mouseIn = true;
    OnPlotMouseMove(pt, flags);
}

void PlotWnd::OnMouseLeave()
{
    m_tracking = false;
    m_mouseIn  = false;
    m_mouse    = CPoint(-1, -1);
    OnPlotMouseLeave();
}

void PlotWnd::OnLButtonDown(UINT flags, CPoint pt)   { SetFocus(); OnPlotLButtonDown(pt, flags); }
void PlotWnd::OnLButtonUp(UINT flags, CPoint pt)     { OnPlotLButtonUp(pt, flags); }
void PlotWnd::OnLButtonDblClk(UINT flags, CPoint pt) { OnPlotLButtonDblClk(pt, flags); }
void PlotWnd::OnRButtonUp(UINT flags, CPoint pt)     { SetFocus(); OnPlotRButtonUp(pt, flags); }

BOOL PlotWnd::OnMouseWheel(UINT flags, short zDelta, CPoint pt)
{
    ScreenToClient(&pt);
    if (OnPlotMouseWheel(flags, zDelta, pt)) return TRUE;
    return CWnd::OnMouseWheel(flags, zDelta, pt);
}

void PlotWnd::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    OnPlotKeyDown(nChar, nFlags);
    CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
}

UINT PlotWnd::OnGetDlgCode() { return DLGC_WANTARROWS | DLGC_WANTCHARS; }

// Default overridables.
void PlotWnd::OnPlotMouseMove(CPoint, UINT)     { Invalidate(FALSE); }
void PlotWnd::OnPlotMouseLeave()                { Invalidate(FALSE); }
void PlotWnd::OnPlotLButtonDown(CPoint, UINT)   {}
void PlotWnd::OnPlotLButtonUp(CPoint, UINT)     {}
void PlotWnd::OnPlotLButtonDblClk(CPoint, UINT) {}
void PlotWnd::OnPlotRButtonUp(CPoint, UINT)     {}
BOOL PlotWnd::OnPlotMouseWheel(UINT, short, CPoint) { return FALSE; }
void PlotWnd::OnPlotKeyDown(UINT, UINT)         {}
void PlotWnd::OnPlotSize(int, int)              {}

// ---------------------------------------------------------------------------
double PlotWnd::NiceStep(double range, int maxTicks)
{
    if (range <= 0.0 || maxTicks < 1) return 1.0;
    double raw = range / maxTicks;
    double mag = std::pow(10.0, std::floor(std::log10(raw)));
    double n   = raw / mag;
    double nice = (n <= 1.0) ? 1.0 : (n <= 2.0) ? 2.0 : (n <= 2.5) ? 2.5 : (n <= 5.0) ? 5.0 : 10.0;
    return nice * mag;
}

CString PlotWnd::FormatFreq(double hz)
{
    CString s;
    double a = std::fabs(hz);
    if (a >= 1e6)      s.Format(_T("%.3f MHz"), hz / 1e6);
    else if (a >= 1e3) s.Format(_T("%.3f kHz"), hz / 1e3);
    else               s.Format(_T("%.1f Hz"), hz);
    return s;
}

CString PlotWnd::FormatRange(double m)
{
    CString s;
    double a = std::fabs(m);
    if (a >= 1000.0)   s.Format(_T("%.3f km"), m / 1000.0);
    else if (a >= 1.0) s.Format(_T("%.2f m"), m);
    else if (a >= 0.01)s.Format(_T("%.1f cm"), m * 100.0);
    else               s.Format(_T("%.2f mm"), m * 1000.0);
    return s;
}

CString PlotWnd::FormatTime(double sec)
{
    CString s;
    if (std::fabs(sec) < 1e-12) sec = 0.0;      // no "-0.0"
    double a = std::fabs(sec);
    if (a >= 1.0)       s.Format(_T("%.3f s"), sec);
    else if (a >= 1e-3) s.Format(_T("%.3f ms"), sec * 1e3);
    else if (a >= 1e-6) s.Format(_T("%.2f \u00B5s"), sec * 1e6);
    else                s.Format(_T("%.1f ns"), sec * 1e9);
    return s;
}

CString PlotWnd::FormatDb(double db)
{
    CString s; s.Format(_T("%.1f dB"), db); return s;
}

CString PlotWnd::FormatVelocity(double mps)
{
    CString s; s.Format(_T("%.2f m/s"), mps); return s;
}

void PlotWnd::DrawXAxis(CDC& dc, const CRect& plotRc, const CRect& axisRc, const LinearAxis& ax,
                        const std::function<CString(double)>& fmt, bool labelsAbove, bool gridLines)
{
    const Theme::Palette& th = Theme::Get();
    const double lo = (std::min)(ax.v0, ax.v1), hi = (std::max)(ax.v0, ax.v1);
    if (hi <= lo) return;
    const int maxTicks = (std::max)(2, plotRc.Width() / S(90));
    const double step = NiceStep(hi - lo, maxTicks);
    CFont* pOld = dc.SelectObject(&m_fontAxis);
    dc.SetBkMode(TRANSPARENT);
    CPen gridPen(PS_SOLID, 1, th.grid);
    CPen tickPen(PS_SOLID, 1, th.axis);
    double first = std::ceil(lo / step) * step;
    for (double v = first; v <= hi + step * 1e-6; v += step) {
        int x = ax.ToPx(v);
        if (x < plotRc.left || x > plotRc.right) continue;
        if (gridLines) {
            CPen* p = dc.SelectObject(&gridPen);
            dc.MoveTo(x, plotRc.top); dc.LineTo(x, plotRc.bottom);
            dc.SelectObject(p);
        }
        CPen* p = dc.SelectObject(&tickPen);
        if (labelsAbove) { dc.MoveTo(x, plotRc.top); dc.LineTo(x, plotRc.top - S(3)); }
        else             { dc.MoveTo(x, plotRc.bottom); dc.LineTo(x, plotRc.bottom + S(3)); }
        dc.SelectObject(p);
        CString lbl = fmt(std::fabs(v) < step * 1e-9 ? 0.0 : v);
        // Centre the label on the tick, but keep it inside the plot's horizontal
        // extent so the first / last labels do not run into the y-axis strip
        // (or off the right edge) and collide with other text.
        const int tw = dc.GetTextExtent(lbl).cx;
        int left = x - tw / 2;
        if (left < plotRc.left - S(2)) left = plotRc.left - S(2);
        if (left + tw > plotRc.right + S(2)) left = plotRc.right + S(2) - tw;
        CRect r(left, axisRc.top, left + tw + S(2), axisRc.bottom);
        dc.SetTextColor(th.axisText);
        dc.DrawText(lbl, r, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP);
    }
    dc.SelectObject(pOld);
}

void PlotWnd::DrawYAxis(CDC& dc, const CRect& plotRc, const CRect& axisRc, const LinearAxis& ax,
                        const std::function<CString(double)>& fmt, bool gridLines)
{
    const Theme::Palette& th = Theme::Get();
    const double lo = (std::min)(ax.v0, ax.v1), hi = (std::max)(ax.v0, ax.v1);
    if (hi <= lo) return;
    const int maxTicks = (std::max)(2, plotRc.Height() / S(40));
    const double step = NiceStep(hi - lo, maxTicks);
    CFont* pOld = dc.SelectObject(&m_fontAxis);
    dc.SetBkMode(TRANSPARENT);
    CPen gridPen(PS_SOLID, 1, th.grid);
    CPen tickPen(PS_SOLID, 1, th.axis);
    double first = std::ceil(lo / step) * step;
    for (double v = first; v <= hi + step * 1e-6; v += step) {
        int y = ax.ToPx(v);
        if (y < plotRc.top || y > plotRc.bottom) continue;
        if (gridLines) {
            CPen* p = dc.SelectObject(&gridPen);
            dc.MoveTo(plotRc.left, y); dc.LineTo(plotRc.right, y);
            dc.SelectObject(p);
        }
        CPen* p = dc.SelectObject(&tickPen);
        dc.MoveTo(plotRc.left - S(3), y); dc.LineTo(plotRc.left, y);
        dc.SelectObject(p);
        CString lbl = fmt(std::fabs(v) < step * 1e-9 ? 0.0 : v);
        CRect r(axisRc.left, y - S(8), axisRc.right - S(4), y + S(8));
        dc.SetTextColor(th.axisText);
        dc.DrawText(lbl, r, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP);
    }
    dc.SelectObject(pOld);
}

void PlotWnd::DrawReadout(CDC& dc, const CRect& rc, CPoint anchor, const std::vector<CString>& lines)
{
    if (lines.empty()) return;
    const Theme::Palette& th = Theme::Get();
    CFont* pOld = dc.SelectObject(&m_fontMono);
    int w = 0, h = 0;
    for (const auto& l : lines) {
        CSize sz = dc.GetTextExtent(l);
        w = (std::max)(w, static_cast<int>(sz.cx));
        h += sz.cy;
    }
    const int pad = S(5);
    CRect box(anchor.x + S(14), anchor.y + S(14), anchor.x + S(14) + w + 2 * pad, anchor.y + S(14) + h + 2 * pad);
    if (box.right > rc.right)  box.OffsetRect(rc.right - box.right - S(2), 0);
    if (box.bottom > rc.bottom) box.OffsetRect(0, (anchor.y - S(14) - box.Height()) - box.top);
    if (box.left < rc.left)    box.OffsetRect(rc.left - box.left + S(2), 0);
    if (box.top < rc.top)      box.OffsetRect(0, rc.top - box.top + S(2));
    dc.FillSolidRect(box, th.readoutBg);
    CPen pen(PS_SOLID, 1, th.border);
    CPen* pp = dc.SelectObject(&pen);
    CBrush* pb = static_cast<CBrush*>(dc.SelectStockObject(NULL_BRUSH));
    dc.Rectangle(box);
    if (pb) dc.SelectObject(pb);
    dc.SelectObject(pp);
    dc.SetBkMode(TRANSPARENT);
    dc.SetTextColor(th.readoutText);
    int y = box.top + pad;
    for (const auto& l : lines) {
        dc.TextOut(box.left + pad, y, l);
        y += dc.GetTextExtent(l).cy;
    }
    dc.SelectObject(pOld);
}

void PlotWnd::DrawLegendItem(CDC& dc, int& x, int y, COLORREF color, const CString& text, bool dashed)
{
    const Theme::Palette& th = Theme::Get();
    CFont* pOld = dc.SelectObject(&m_fontAxis);
    dc.SetBkMode(TRANSPARENT);
    CPen pen(dashed ? PS_DOT : PS_SOLID, 1, color);
    CPen* pp = dc.SelectObject(&pen);
    dc.MoveTo(x, y); dc.LineTo(x + S(16), y);
    dc.SelectObject(pp);
    x += S(20);
    dc.SetTextColor(th.textDim);
    dc.TextOut(x, y - S(7), text);
    x += dc.GetTextExtent(text).cx + S(12);
    dc.SelectObject(pOld);
}
