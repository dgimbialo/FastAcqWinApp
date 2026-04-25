#pragma once
//
// CommandPanel -- row of buttons & edit boxes. Posts WM_APP_CMD_* to parent.
//

#include "pch.h"
#include "LocalFft.h"

class CommandPanel : public CWnd {
public:
    CommandPanel() = default;

    BOOL CreatePanel(CWnd* parent, UINT id);

    void SetConnected(bool c);
    void PopulateComPorts(const std::vector<CString>& ports);
    CString  GetSelectedPort() const;
    uint16_t GetFreqHz() const;
    uint32_t GetSamples() const;
    uint16_t GetModeSel() const;
    uint8_t  GetDataMask() const;
    uint16_t GetIntervalMs() const;
    bool     IsPcRawMode() const;     // true = PC processes RAW data locally

protected:
    afx_msg int  OnCreate(LPCREATESTRUCT lpcs);
    afx_msg void OnSize(UINT, int, int);
    afx_msg void OnConnect();
    afx_msg void OnStart();
    afx_msg void OnStop();
    afx_msg void OnSetFreq();
    afx_msg void OnSetSamples();
    afx_msg void OnPing();
    afx_msg void OnSaveFrame();
    afx_msg void OnClear();
    afx_msg void OnApplyMode();
    afx_msg void OnApplyData();
    afx_msg void OnApplyInterval();
    afx_msg void OnTrigger();
    afx_msg void OnGetStatus();
    afx_msg void OnPcModeChanged();
    DECLARE_MESSAGE_MAP()

private:
    void Relayout();

    CComboBox m_cmbCom;
    CButton   m_btnConnect;
    CButton   m_btnStart;
    CButton   m_btnStop;
    CEdit     m_edtFreq;
    CButton   m_btnSetFreq;
    CEdit     m_edtSamples;
    CButton   m_btnSetSamples;
    CButton   m_btnPing;
    CButton   m_btnSaveFrame;
    CButton   m_btnClear;
    CStatic   m_lblMode;
    CComboBox m_cmbMode;
    CButton   m_btnApplyMode;
    CButton   m_chkRaw;
    CButton   m_chkFft;
    CButton   m_btnApplyData;
    CStatic   m_lblInterval;
    CEdit     m_edtInterval;
    CButton   m_btnApplyInterval;
    CButton   m_btnTrigger;
    CButton   m_btnGetStatus;
    // Row 3: PC-side mode.
    CStatic   m_lblPcMode;
    CButton   m_rdoPcRaw;
    CButton   m_rdoPcFft;
    CFont     m_font;
    bool      m_connected{false};
};
