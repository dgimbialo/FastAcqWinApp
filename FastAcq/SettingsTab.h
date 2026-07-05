#pragma once
//
// SettingsTabWnd -- "Settings" tab hosting all MCU acquisition configuration
// (mode, chirp frequency, samples, interval, data mask, amplitude, burst)
// plus PC-side processing mode and diagnostics (Ping / Status).
// Posts the same WM_APP_CMD_* messages as CommandPanel, to the main frame.
// Native Win32 look: standard buttons, system colors.
//

#include "pch.h"

class SettingsTabWnd : public CWnd {
public:
    BOOL CreateTab(CWnd* parent, UINT id);

    void SetConnected(bool c);

    uint16_t GetFreqHz() const;
    uint32_t GetSamples() const;
    uint16_t GetModeSel() const;
    uint8_t  GetDataMask() const;
    uint16_t GetIntervalMs() const;
    uint16_t GetAmplitude() const;
    uint16_t GetBurst() const;
    bool     IsPcRawMode() const;

protected:
    afx_msg int  OnCreate(LPCREATESTRUCT lpcs);
    afx_msg void OnSize(UINT, int, int);
    afx_msg void OnApplyMode();
    afx_msg void OnSetFreq();
    afx_msg void OnSetSamples();
    afx_msg void OnApplyInterval();
    afx_msg void OnApplyData();
    afx_msg void OnSetAmplitude();
    afx_msg void OnSetBurst();
    afx_msg void OnPing();
    afx_msg void OnGetStatus();
    afx_msg void OnPcModeChanged();
    DECLARE_MESSAGE_MAP()

private:
    void Relayout();
    void PostToMain(UINT msg, WPARAM wp = 0, LPARAM lp = 0);

    // MCU acquisition settings
    CStatic   m_lblMode;
    CComboBox m_cmbMode;
    CButton   m_btnApplyMode;
    CStatic   m_lblFreq;
    CEdit     m_edtFreq;
    CButton   m_btnSetFreq;
    CStatic   m_lblSamples;
    CEdit     m_edtSamples;
    CButton   m_btnSetSamples;
    CStatic   m_lblInterval;
    CEdit     m_edtInterval;
    CButton   m_btnApplyInterval;
    CStatic   m_lblAmplitude;
    CEdit     m_edtAmplitude;
    CButton   m_btnSetAmp;
    CStatic   m_lblBurst;
    CEdit     m_edtBurst;
    CButton   m_btnSetBurst;

    // Data selection
    CStatic   m_lblData;
    CButton   m_chkRaw;
    CButton   m_chkFft;
    CButton   m_btnApplyData;

    // Diagnostics
    CButton   m_btnPing;
    CButton   m_btnGetStatus;

    // PC-side processing
    CStatic   m_lblPcMode;
    CButton   m_rdoPcRaw;
    CButton   m_rdoPcFft;

    // Section headers
    CStatic   m_hdrMcu;
    CStatic   m_hdrData;
    CStatic   m_hdrPc;

    CFont     m_font;
    CFont     m_hdrFont;
    bool      m_connected{false};
};
