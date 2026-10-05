#pragma once
//
// ScopeTab -- raw IF waveform: whole frame with ramp shading on top, one
// selected ramp (UP/DOWN of a chirp) in detail below.
//

#include "pch.h"
#include "AppSettings.h"
#include "ChirpStore.h"
#include "Dsp/RadarDsp.h"
#include "WaveformView.h"

class ScopeTab : public CWnd {
public:
    BOOL CreateTab(CWnd* parent, UINT id);

    void ShowFrame(ChirpFramePtr f, std::shared_ptr<const dsp::FrameResult> r);
    void ApplySettings(const AppSettings& s);
    void ReadFooter(AppSettings& s) const;
    void SetSplit(float r) { if (r > 0.1f && r < 0.9f) m_split = r; Relayout(); }
    float GetSplit() const { return m_split; }
    void ResetZoom();
    void ClearCursors();
    void ApplyTheme();

protected:
    afx_msg int    OnCreate(LPCREATESTRUCT lpcs);
    afx_msg void   OnSize(UINT, int, int);
    afx_msg void   OnPaint();
    afx_msg BOOL   OnEraseBkgnd(CDC* pDC);
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    afx_msg void   OnLButtonDown(UINT, CPoint pt);
    afx_msg void   OnLButtonUp(UINT, CPoint pt);
    afx_msg void   OnMouseMove(UINT, CPoint pt);
    afx_msg BOOL   OnSetCursor(CWnd*, UINT, UINT);
    afx_msg void   OnFooterChanged();
    afx_msg void   OnRampSelChanged();
    DECLARE_MESSAGE_MAP()

private:
    void Relayout();
    bool HitSplitter(CPoint pt) const;
    void RebuildRampCombo();
    void UpdateRampView();

    static constexpr int kSplitH96  = 6;
    static constexpr int kFooterH96 = 30;

    WaveformView m_frame;
    WaveformView m_ramp;
    CButton   m_chkDots;
    CButton   m_rdoVolts, m_rdoCodes;
    CStatic   m_lblRamp;
    CComboBox m_cmbRamp;
    CStatic   m_lblInfo;
    CFont     m_font;
    CBrush    m_bgBrush;

    ChirpFramePtr m_f;
    std::shared_ptr<const dsp::FrameResult> m_res;
    std::vector<size_t> m_rampSegIdx;    // indices into m_res->segments (non-guard)
    float m_split{0.5f};
    bool  m_dragging{false};
    CRect m_rcSplit;
    bool  m_suppress{false};
};
