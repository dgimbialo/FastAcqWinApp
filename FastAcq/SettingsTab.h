#pragma once
//
// SettingsTab -- MCU acquisition, radar geometry (VCO) and application
// settings, with a live "derived values" read-out and the chirp preview.
// Processing, detection and display settings live on the Radar tab
// (ProcPanel) so their effect is seen immediately.
//

#include "pch.h"
#include "AppSettings.h"
#include "ChirpPreview.h"
#include "Tips.h"

class SettingsTab : public CWnd {
public:
    BOOL CreateTab(CWnd* parent, UINT id);

    void SetConnected(bool c);
    void ApplySettings(const AppSettings& s);     // settings -> controls
    void ReadInto(AppSettings& s) const;          // controls -> settings
    void SetObserved(double fsHz, size_t samplesPerFrame, size_t mcuFftSize);
    void RefreshDerived();
    void ApplyTheme();
    BOOL PreTranslateMessage(MSG* pMsg) override;

    // MCU values as currently typed (used by Start / Send all).
    uint16_t GetFreqHz() const;
    uint32_t GetSamples() const;
    uint16_t GetModeSel() const;
    uint8_t  GetDataMask() const;
    uint16_t GetIntervalMs() const;
    uint16_t GetAmplitude() const;
    uint16_t GetOffset() const;       // chirp DAC offset (base level)
    uint16_t GetBurst() const;
    uint16_t GetRiseUs() const;       // 0 when the chirp is defined by its frequency
    uint16_t GetFallUs() const;
    double   GetFsPpm() const;

protected:
    afx_msg int    OnCreate(LPCREATESTRUCT lpcs);
    afx_msg void   OnSize(UINT, int, int);
    afx_msg BOOL   OnEraseBkgnd(CDC* pDC);
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    afx_msg void   OnApplyMode();
    afx_msg void   OnSetFreq();
    afx_msg void   OnSetSamples();
    afx_msg void   OnApplyInterval();
    afx_msg void   OnApplyData();
    afx_msg void   OnSetAmplitude();
    afx_msg void   OnSetOffset();
    afx_msg void   OnSetBurst();
    afx_msg void   OnSetRamp();
    afx_msg void   OnApplyPpm();
    afx_msg void   OnFreqChanged();               // mirror frequency -> rise/fall
    afx_msg void   OnRampChanged();               // mirror rise/fall -> frequency
    afx_msg void   OnPreviewInput(UINT id);       // any chirp parameter typed: refresh the preview
    afx_msg void   OnModeSelChanged();
    afx_msg void   OnLangChanged();
    afx_msg void   OnPing();
    afx_msg void   OnGetStatus();
    afx_msg void   OnSendAll();
    afx_msg void   OnAutoApply();                 // combos / checkboxes / radios
    afx_msg void   OnAutoApplyRange(UINT id);
    afx_msg void   OnEditKillFocus(UINT id);
    DECLARE_MESSAGE_MAP()

private:
    void Relayout();
    void PostToMain(UINT msg, WPARAM wp = 0, LPARAM lp = 0);
    void NotifyChanged();
    void UpdatePreview();
    void UpdateVcoDerived(AppSettings& s);   // f0 / B from the curve into the read-only fields
    static int    GetInt(const CEdit& e, int def);
    static double GetDouble(const CEdit& e, double def);
    static void   SetInt(CEdit& e, long long v);
    static void   SetDouble(CEdit& e, double v, LPCTSTR fmt = _T("%g"));

    // --- MCU acquisition
    CStatic m_hdrMcu;
    CStatic m_lblMode;      CComboBox m_cmbMode;      CButton m_btnApplyMode;
    CStatic m_lblFreq;      CEdit m_edtFreq;          CButton m_btnSetFreq;
    CStatic m_lblSamples;   CEdit m_edtSamples;       CButton m_btnSetSamples;
    CStatic m_lblInterval;  CEdit m_edtInterval;      CButton m_btnApplyInterval;
    CStatic m_lblAmplitude; CEdit m_edtAmplitude;     CButton m_btnSetAmp;
    CStatic m_lblOffset;    CEdit m_edtOffset;        CButton m_btnSetOffset;
    CStatic m_lblBurst;     CEdit m_edtBurst;         CButton m_btnSetBurst;
    CStatic m_lblRamp;      CEdit m_edtRise;          CEdit m_edtFall;       CButton m_btnSetRamp;
    CStatic m_lblData;      CButton m_chkRaw;         CButton m_chkFft;      CButton m_btnApplyData;
    CButton m_btnPing;      CButton m_btnGetStatus;   CButton m_btnSendAll;
    // --- Radar
    CStatic m_hdrRadar;
    CStatic m_lblVco;       CButton m_chkVco;
    CStatic m_lblVtune;     CEdit m_edtVtuneLo;       CEdit m_edtVtuneHi;
    CStatic m_lblCurve;     CEdit m_edtCurve;
    CStatic m_lblF0;        CEdit m_edtF0;
    CStatic m_lblBw;        CEdit m_edtBw;
    CStatic m_lblTramp;     CEdit m_edtTramp;
    CStatic m_lblRoff;      CEdit m_edtRoff;
    CStatic m_lblShape;     CComboBox m_cmbShape;
    CStatic m_lblChirps;    CButton m_chkChirpsAuto;  CEdit m_edtChirps;
    CStatic m_lblPairV;     CEdit m_edtPairV;
    // --- Application
    CStatic m_hdrDisplay;
    CStatic m_lblDark;      CButton m_chkDark;
    CStatic m_lblLang;      CComboBox m_cmbLang;
    CStatic m_lblFsCal;     CEdit m_edtFsCal;
    CStatic m_lblPpm;       CEdit m_edtPpm;           CButton m_btnApplyPpm;
    CStatic m_lblVerbose;   CButton m_chkVerbose;
    CStatic m_lblAutoConn;  CButton m_chkAutoConnect;
    CStatic m_hdrDerived;   CStatic m_lblDerived;
    ChirpPreviewCtrl m_preview;
    FieldTips m_tips;

    CFont   m_font;
    CFont   m_hdrFont;
    CBrush  m_bgBrush;
    AppSettings m_last;               // last applied settings (fields owned elsewhere pass through)
    bool    m_connected{false};
    bool    m_suppress{false};
    bool    m_syncing{false};         // inside a freq <-> rise/fall mirror update
    bool    m_rampMode{false};        // rise/fall typed explicitly (CMD_SET_RAMP)
    double  m_obsFs{0.0};
    size_t  m_obsSamples{0};
    size_t  m_obsMcuFft{0};
};
