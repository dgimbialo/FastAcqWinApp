#pragma once
//
// RadarTab -- main measurement view: range profile (top), waterfall and
// range-Doppler map (middle), target table (bottom), a footer with the
// trace mode / palette / visibility controls, and the processing /
// detection / display settings column on the right. Two draggable splitters.
//

#include "pch.h"
#include "Tips.h"
#include "AppSettings.h"
#include "Dsp/RadarDsp.h"
#include "ProcPanel.h"
#include "RangeDopplerView.h"
#include "RangeProfileView.h"
#include "TargetListCtrl.h"
#include "WaterfallView.h"

class RadarTab : public CWnd {
public:
    BOOL CreateTab(CWnd* parent, UINT id);

    void ShowResult(std::shared_ptr<const dsp::FrameResult> r, bool pushWaterfall);
    void ApplySettings(const AppSettings& s);
    void ReadFooter(AppSettings& s) const;
    void ReadProcPanel(AppSettings& s) const { m_proc.ReadInto(s); }
    void ClearHistory();
    void ResetZoom();
    void ClearMarkers();
    void SetSplits(float a, float b);
    void GetSplits(float& a, float& b) const { a = m_split1; b = m_split2; }
    std::shared_ptr<const dsp::FrameResult> CurrentResult() const { return m_res; }
    void ApplyTheme();

    BOOL PreTranslateMessage(MSG* pMsg) override;

protected:
    afx_msg int    OnCreate(LPCREATESTRUCT lpcs);
    afx_msg void   OnSize(UINT, int cx, int cy);
    afx_msg void   OnPaint();
    afx_msg BOOL   OnEraseBkgnd(CDC* pDC);
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    afx_msg void   OnLButtonDown(UINT, CPoint pt);
    afx_msg void   OnLButtonUp(UINT, CPoint pt);
    afx_msg void   OnMouseMove(UINT, CPoint pt);
    afx_msg BOOL   OnSetCursor(CWnd*, UINT, UINT);
    afx_msg void   OnFooterChanged();
    afx_msg void   OnResetAvg();
    afx_msg void   OnAutoscale();
    afx_msg void   OnClearWf();
    afx_msg LRESULT OnXRangeChanged(WPARAM src, LPARAM);
    afx_msg LRESULT OnLearnSpurs(WPARAM, LPARAM);
    DECLARE_MESSAGE_MAP()

private:
    void Relayout();
    int  HitSplitter(CPoint pt) const;   // 0 none, 1 first, 2 second
    void PostSettingsChanged();
    void FollowZoomWithRangeOfInterest();

    static constexpr int kSplitH96  = 6;
    static constexpr int kFooterH96 = 30;
    static constexpr int kMinPane96 = 60;

    RangeProfileView m_profile;
    WaterfallView    m_waterfall;
    RangeDopplerView m_rd;
    TargetListCtrl   m_targets;
    ProcPanel        m_proc;

    CStatic   m_lblTrace;
    CComboBox m_cmbTrace;
    CStatic   m_lblPalette;
    CComboBox m_cmbPalette;
    CButton   m_chkUp, m_chkDn, m_chkThr, m_chkNoise, m_chkRd;
    CButton   m_btnResetAvg, m_btnAutoscale, m_btnClearWf;
    CStatic   m_lblPhase;
    CFont     m_font;
    CBrush    m_bgBrush;

    std::shared_ptr<const dsp::FrameResult> m_res;
    DisplaySettings m_disp;
    double m_maxRangeM{100.0};           // dsp.maxRangeM as shown / driven by the zoom
    float m_split1{0.42f};
    float m_split2{0.78f};
    int   m_dragging{0};
    CRect m_rcSplit1, m_rcSplit2;
    bool  m_showRd{true};
    bool  m_rdVisible{false};
    bool  m_suppress{false};
    FieldTips m_tips;
};
