#pragma once
//
// PlotWnd -- base class for the custom-drawn (GDI, double-buffered) plots:
// fonts scaled by DPI, mouse tracking, axis helpers, clipboard copy.
//

#include "pch.h"
#include "Dpi.h"
#include "Theme.h"

struct LinearAxis {
    double v0{0.0}, v1{1.0};   // value at px0 / px1
    int    px0{0},  px1{1};
    int    ToPx(double v) const {
        if (v1 == v0) return px0;
        return px0 + static_cast<int>(std::lround((v - v0) / (v1 - v0) * (px1 - px0)));
    }
    double FromPx(int px) const {
        if (px1 == px0) return v0;
        return v0 + (v1 - v0) * (static_cast<double>(px - px0) / static_cast<double>(px1 - px0));
    }
};

class PlotWnd : public CWnd {
public:
    PlotWnd() = default;

    BOOL CreatePlot(CWnd* parent, UINT id, DWORD extraStyle = 0);
    void SetTitle(const CString& t) { m_title = t; if (GetSafeHwnd()) Invalidate(FALSE); }

    // Renders the current content into a bitmap and places it on the clipboard.
    void CopyImageToClipboard();

    // Formatting helpers (shared with the tabs' footers).
    static double  NiceStep(double range, int maxTicks);
    static CString FormatFreq(double hz);
    static CString FormatRange(double m);
    static CString FormatTime(double sec);
    static CString FormatDb(double db);
    static CString FormatVelocity(double mps);

protected:
    virtual void Render(CDC& dc, const CRect& rc) = 0;
    virtual void OnPlotMouseMove(CPoint pt, UINT flags);
    virtual void OnPlotMouseLeave();
    virtual void OnPlotLButtonDown(CPoint pt, UINT flags);
    virtual void OnPlotLButtonUp(CPoint pt, UINT flags);
    virtual void OnPlotLButtonDblClk(CPoint pt, UINT flags);
    virtual void OnPlotRButtonUp(CPoint pt, UINT flags);
    virtual BOOL OnPlotMouseWheel(UINT flags, short zDelta, CPoint pt);
    virtual void OnPlotKeyDown(UINT nChar, UINT flags);
    virtual void OnPlotSize(int cx, int cy);
    virtual void OnDpiChanged();

    void RecreateFonts();
    int  S(int px96) const { return Dpi::Scale(m_hWnd, px96); }

    // Horizontal ticks + labels under/over plotRc (labels drawn in axisRc).
    void DrawXAxis(CDC& dc, const CRect& plotRc, const CRect& axisRc, const LinearAxis& ax,
                   const std::function<CString(double)>& fmt, bool labelsAbove, bool gridLines);
    // Vertical ticks + labels left of plotRc.
    void DrawYAxis(CDC& dc, const CRect& plotRc, const CRect& axisRc, const LinearAxis& ax,
                   const std::function<CString(double)>& fmt, bool gridLines);
    // Multi-line readout box anchored near a point (kept inside rc).
    void DrawReadout(CDC& dc, const CRect& rc, CPoint anchor, const std::vector<CString>& lines);
    void DrawLegendItem(CDC& dc, int& x, int y, COLORREF color, const CString& text, bool dashed = false);

    CFont   m_fontAxis;
    CFont   m_fontLabel;
    CFont   m_fontTitle;
    CFont   m_fontMono;
    CString m_title;
    CPoint  m_mouse{-1, -1};
    bool    m_mouseIn{false};

    afx_msg int  OnCreate(LPCREATESTRUCT lpcs);
    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC*) { return TRUE; }
    afx_msg void OnSize(UINT, int cx, int cy);
    afx_msg void OnMouseMove(UINT flags, CPoint pt);
    afx_msg void OnMouseLeave();
    afx_msg void OnLButtonDown(UINT flags, CPoint pt);
    afx_msg void OnLButtonUp(UINT flags, CPoint pt);
    afx_msg void OnLButtonDblClk(UINT flags, CPoint pt);
    afx_msg void OnRButtonUp(UINT flags, CPoint pt);
    afx_msg BOOL OnMouseWheel(UINT flags, short zDelta, CPoint pt);
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg UINT OnGetDlgCode();
    afx_msg LRESULT OnDpiChangedAfterParent(WPARAM, LPARAM);
    DECLARE_MESSAGE_MAP()

private:
    bool m_tracking{false};
};
