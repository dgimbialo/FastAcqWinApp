#pragma once
//
// Tab1Wnd / Tab2Wnd -- containers for Waveform and FFT/Waterfall tabs.
//

#include "pch.h"
#include "WaveformView.h"
#include "WaterfallView.h"
#include "SpectrumView.h"
#include "ChirpStore.h"
#include "LocalFft.h"

class Tab1Wnd : public CWnd {
public:
    BOOL CreateTab(CWnd* parent, UINT id);
    void ShowFrame(const ChirpFrame& f, bool rawMode,
                   const FftSettings& cfg, uint32_t sampleRateHz);
    void SetAcqMode(bool rawMode);
    void SetDotsMode(bool dots);

protected:
    afx_msg int  OnCreate(LPCREATESTRUCT);
    afx_msg void OnSize(UINT, int, int);
    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC*) { return TRUE; }
    afx_msg void OnLButtonDown(UINT, CPoint);
    afx_msg void OnLButtonUp(UINT, CPoint);
    afx_msg void OnMouseMove(UINT, CPoint);
    afx_msg BOOL OnSetCursor(CWnd*, UINT, UINT);
    afx_msg void OnDotsChanged();
    DECLARE_MESSAGE_MAP()

private:
    void ApplySplit(int cy);
    bool HitSplitter(CPoint pt) const;

    static constexpr int kSplitH   = 6;
    static constexpr int kFooterH = 28;

    WaveformView m_up;
    WaveformView m_dn;

    CButton m_chkDots;
    CFont   m_footerFont;

    float m_splitRatio{0.5f};
    bool  m_dragging{false};
    int   m_lastCy{0};
    bool  m_rawMode{true};
};

class Tab2Wnd : public CWnd {
public:
    BOOL CreateTab(CWnd* parent, UINT id);
    void ShowFrame(const ChirpFrame& f, bool rawMode,
                   const FftSettings& cfg, uint32_t sampleRateHz);
    void SetAcqMode(bool rawMode);
    void SetFftSettings(const FftSettings& cfg) { m_fftCfg = cfg; }
    void ClearHistory();

protected:
    afx_msg int  OnCreate(LPCREATESTRUCT);
    afx_msg void OnSize(UINT, int, int);
    afx_msg void OnApplyFft();
    DECLARE_MESSAGE_MAP()

private:
    static constexpr int kFooterH = 28;

    WaterfallView m_waterfall;
    SpectrumView  m_specUp;
    SpectrumView  m_specDn;

    CStatic   m_lblFftSize;
    CComboBox m_cmbFftSize;
    CStatic   m_lblFftWin;
    CComboBox m_cmbFftWin;
    CButton   m_btnApplyFft;
    CFont     m_footerFont;

    bool        m_rawMode{true};
    FftSettings m_fftCfg;
};
