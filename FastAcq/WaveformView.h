#pragma once
//
// WaveformView -- oscilloscope view of raw ADC samples with min/max column
// decimation, time axis, ramp segment shading (UP / DOWN / guard), A/B
// cursors, zoom (wheel, buttons, drag) and scrollbars.
//

#include "pch.h"
#include "Dsp/RadarDsp.h"
#include "PlotWnd.h"

class WaveformView : public PlotWnd {
public:
    WaveformView() = default;

    BOOL CreateView(CWnd* parent, UINT id);

    // data[0] corresponds to frame sample index `offset` (for segment shading).
    void SetSamples(const uint16_t* data, size_t n, size_t offset = 0);
    void SetSegments(const std::vector<dsp::Segment>& segs);
    // One caption per segment (same order as SetSegments), drawn inside the band.
    void SetSegmentLabels(const std::vector<CString>& labels);
    void SetSampleRate(double fsHz)          { m_fs = fsHz; Invalidate(FALSE); }
    void SetAdcConfig(int bits, float vRef, bool showVolts);
    void SetDotsMode(bool dots)              { m_dots = dots; Invalidate(FALSE); }
    void SetInfo(const CString& s)           { m_info = s; Invalidate(FALSE); }
    void ResetZoom();
    void ClearCursors();

protected:
    void Render(CDC& dc, const CRect& rc) override;
    void OnPlotMouseMove(CPoint pt, UINT flags) override;
    void OnPlotLButtonDown(CPoint pt, UINT flags) override;
    void OnPlotLButtonUp(CPoint pt, UINT flags) override;
    void OnPlotLButtonDblClk(CPoint pt, UINT flags) override;
    void OnPlotRButtonUp(CPoint pt, UINT flags) override;
    BOOL OnPlotMouseWheel(UINT flags, short zDelta, CPoint pt) override;
    void OnPlotKeyDown(UINT nChar, UINT flags) override;
    void OnPlotSize(int cx, int cy) override;

    afx_msg int  OnCreate(LPCREATESTRUCT lpcs);
    afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pBar);
    afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pBar);
    afx_msg void OnBtnXMinus();
    afx_msg void OnBtnXPlus();
    afx_msg void OnBtnYMinus();
    afx_msg void OnBtnYPlus();
    afx_msg void OnBtnReset();
    DECLARE_MESSAGE_MAP()

private:
    CRect   PlotRect() const;
    size_t  VisibleCount() const;
    void    ClampOffsets();
    void    UpdateScrollBars();
    void    ZoomX(double factor, double anchorSample);
    void    ZoomY(double factor);
    double  SampleAtX(int x, const CRect& plot) const;
    int     XOfSample(double s, const CRect& plot) const;
    double  CodesAtY(int y, const CRect& plot) const;
    double  CodeToUnit(double code) const;
    CString FormatUnit(double code) const;
    void    DrawCursorReadout(CDC& dc, const CRect& plot);

    static constexpr double kMaxZoomX = 4096.0;
    static constexpr double kMaxZoomY = 64.0;
    static constexpr int    kToolH96  = 26;
    static constexpr int    kAxisW96  = 60;
    static constexpr int    kAxisH96  = 18;
    static constexpr int    kBtnW96   = 28;
    static constexpr int    kBtnH96   = 20;

    CButton m_btnXm, m_btnXp, m_btnYm, m_btnYp, m_btnRst;
    CFont   m_btnFont;

    std::vector<uint16_t>     m_samples;
    size_t                    m_offset{0};       // frame index of m_samples[0]
    std::vector<dsp::Segment> m_segments;
    std::vector<CString>      m_segLabels;
    CString                   m_info;

    double m_zoomX{1.0};
    double m_zoomY{1.0};
    size_t m_offsetX{0};      // first visible sample (relative to m_samples)
    double m_offsetY{0.0};    // bottom of the visible window, in ADC codes

    int    m_adcBits{12};
    float  m_vRef{3.3f};
    bool   m_showVolts{true};
    double m_fs{60058600.0};
    bool   m_dots{false};

    struct Cursor { bool valid{false}; double sample{0.0}; };
    Cursor m_cA, m_cB;

    bool   m_dragging{false};
    bool   m_dragMoved{false};
    CPoint m_dragStart;
    size_t m_dragOffset{0};
};
