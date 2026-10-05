#pragma once
//
// ProcPanel -- the processing, detection and display settings as a column
// on the right of the Radar tab, so every change is seen at once on the
// range profile, waterfall and target table. Scrolls vertically when the
// window is short. Values are applied immediately (no Apply button needed).
//

#include "pch.h"
#include "AppSettings.h"

class ProcPanel : public CWnd {
public:
    static constexpr int kWidth96 = 300;      // panel width at 96 dpi

    BOOL CreatePanel(CWnd* parent, UINT id);

    void ApplySettings(const AppSettings& s);     // settings -> controls
    void ReadInto(AppSettings& s) const;          // controls -> settings (only the fields owned here)
    void ApplyTheme();

protected:
    afx_msg int    OnCreate(LPCREATESTRUCT lpcs);
    afx_msg void   OnSize(UINT, int, int);
    afx_msg BOOL   OnEraseBkgnd(CDC* pDC);
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    afx_msg void   OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pBar);
    afx_msg BOOL   OnMouseWheel(UINT flags, short zDelta, CPoint pt);
    afx_msg void   OnAutoApply();
    afx_msg void   OnAutoApplyRange(UINT id);
    afx_msg void   OnEditKillFocus(UINT id);
    afx_msg void   OnDefaults();
    DECLARE_MESSAGE_MAP()

private:
    void Relayout();
    void SetScrollPos(int pos);
    void NotifyChanged();
    static int    GetInt(const CEdit& e, int def);
    static double GetDouble(const CEdit& e, double def);
    static void   SetInt(CEdit& e, long long v);
    static void   SetDouble(CEdit& e, double v, LPCTSTR fmt = _T("%g"));

    // --- Source + processing
    CStatic m_hdrProc;
    CStatic m_lblSource;    CButton m_rdoSrcRaw;      CButton m_rdoSrcMcu;
    CStatic m_lblGuard;     CEdit m_edtGuard;
    CStatic m_lblDetrend;   CButton m_chkDetrend;
    CStatic m_lblDecim;     CComboBox m_cmbDecim;
    CStatic m_lblMaxRange;  CEdit m_edtMaxRange;
    CStatic m_lblWindow;    CComboBox m_cmbWindow;
    CStatic m_lblKaiser;    CEdit m_edtKaiser;
    CStatic m_lblZeroPad;   CComboBox m_cmbZeroPad;
    CStatic m_lblRangeGain; CComboBox m_cmbRangeGain;
    CStatic m_lblFwGeom;    CButton m_chkFwGeom;
    CStatic m_lblTone;      CButton m_chkTone;
    // --- Detection
    CStatic m_hdrDetect;
    CStatic m_lblDetector;  CComboBox m_cmbDetector;
    CStatic m_lblThresh;    CEdit m_edtThresh;
    CStatic m_lblPfa;       CComboBox m_cmbPfa;
    CStatic m_lblCfarGuard; CEdit m_edtCfarGuard;
    CStatic m_lblCfarTrain; CEdit m_edtCfarTrain;
    CStatic m_lblInterp;    CComboBox m_cmbInterp;
    CStatic m_lblMaxPeaks;  CEdit m_edtMaxPeaks;
    CStatic m_lblMti;       CButton m_chkMti;
    CStatic m_lblTrack;     CButton m_chkTrack;
    // --- Display
    CStatic m_hdrDisplay;
    CStatic m_lblDbTop;     CEdit m_edtDbTop;
    CStatic m_lblDbBottom;  CEdit m_edtDbBottom;
    CStatic m_lblWfRows;    CEdit m_edtWfRows;
    CStatic m_lblAdcBits;   CEdit m_edtAdcBits;
    CStatic m_lblVref;      CEdit m_edtVref;
    CButton m_btnDefaults;

    CFont   m_font;
    CFont   m_hdrFont;
    CBrush  m_bgBrush;
    bool    m_suppress{false};
    int     m_scroll{0};          // vertical scroll offset, px
    int     m_contentH{0};
    AppSettings m_last;           // last applied settings (fields not owned here are passed through)
};
