#pragma once
//
// WaveformView -- GDI oscilloscope with H/V zoom toolbar and voltage Y-axis.
//

#include "pch.h"

class WaveformView : public CWnd {
public:
    WaveformView() = default;

    BOOL CreateView(CWnd* parent, UINT id);

    void SetSamples(const uint16_t* data, size_t n);
    void SetTitle(const CString& t)    { m_title     = t;  Invalidate(FALSE); }
    void SetFrequency(float hz)        { m_freqHz    = hz; Invalidate(FALSE); }
    void SetSampleRate(uint32_t rateHz){ m_sampleRateHz = rateHz; }
    void SetDotsMode(bool dots)        { m_dotsMode = dots; Invalidate(FALSE); }

    // ADC hardware parameters (must match firmware).
    // Default: 12-bit AD9226, Vref = 3.3 V.
    void SetAdcConfig(int bits, float vRef) { m_adcBits = bits; m_vRef = vRef; Invalidate(FALSE); }

protected:
    afx_msg int  OnCreate(LPCREATESTRUCT);
    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC*) { return TRUE; }
    afx_msg void OnSize(UINT, int, int);
    afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pBar);
    afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pBar);
    afx_msg BOOL OnMouseWheel(UINT fFlags, short zDelta, CPoint pt);
    afx_msg void OnBtnXMinus();
    afx_msg void OnBtnXPlus();
    afx_msg void OnBtnYMinus();
    afx_msg void OnBtnYPlus();
    afx_msg void OnBtnReset();
    DECLARE_MESSAGE_MAP()

private:
    void    Render(CDC& dc, const CRect& rc);
    void    DrawYAxis(CDC& dc, const CRect& plotRc, float vTop, float vBot);
    void    UpdateScrollBar();
    void    UpdateVScrollBar();
    void    ClampOffsetY();
    void    ApplyDefaultHorizontalZoom();
    size_t  DefaultVisibleSamples() const;
    CRect   PlotRect() const;

    static constexpr float kMaxZoomX = 1024.0f;  // horizontal: up to x1024
    static constexpr float kMaxZoomY =   32.0f;  // vertical:   up to x32
    static constexpr int   kToolH    = 26;
    static constexpr int   kAxisW    = 52;
    static constexpr int   kBtnW     = 26;
    static constexpr int   kBtnH     = 20;

    enum : UINT { ID_XM = 1, ID_XP, ID_YM, ID_YP, ID_RST };

    CButton m_btnXm, m_btnXp, m_btnYm, m_btnYp, m_btnRst;
    CFont   m_btnFont;
    CFont   m_axisFont;
    CFont   m_freqFont;

    std::vector<uint16_t> m_samples;
    CString               m_title;

    float  m_zoomX{1.0f};
    float  m_zoomY{1.0f};
    size_t m_offsetX{0};
    float  m_offsetY{0.0f};  // bottom of vertical view window, in volts

    // ADC config
    int      m_adcBits{12};        // AD9226 = 12-bit
    float    m_vRef{3.3f};         // 3.3 V reference
    float    m_freqHz{0.0f};       // measured dominant frequency (0 = unknown)
    uint32_t m_sampleRateHz{60000000}; // 60 MS/s
    bool     m_dotsMode{false};    // true = draw dots only, false = lines
};
