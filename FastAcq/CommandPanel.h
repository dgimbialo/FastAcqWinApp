#pragma once
//
// CommandPanel -- single-row main toolbar: COM port, Connect, Start/Stop,
// Trigger, Abort, Save Frame, Clear. Posts WM_APP_CMD_* to parent.
// All acquisition configuration lives in the "Settings" tab (SettingsTab.h).
// Native Win32 look: standard buttons, system colors.
//

#include "pch.h"

class CommandPanel : public CWnd {
public:
    CommandPanel() = default;

    BOOL CreatePanel(CWnd* parent, UINT id);

    void SetConnected(bool c);
    void PopulateComPorts(const std::vector<CString>& ports);
    CString GetSelectedPort() const;

protected:
    afx_msg int  OnCreate(LPCREATESTRUCT lpcs);
    afx_msg void OnSize(UINT, int, int);
    afx_msg void OnConnect();
    afx_msg void OnStart();
    afx_msg void OnStop();
    afx_msg void OnTrigger();
    afx_msg void OnAbort();
    afx_msg void OnSaveFrame();
    afx_msg void OnClear();
    afx_msg void OnComDropDown();
    DECLARE_MESSAGE_MAP()

private:
    void Relayout();

    CComboBox m_cmbCom;
    CButton   m_btnConnect;
    CButton   m_btnStart;
    CButton   m_btnStop;
    CButton   m_btnTrigger;
    CButton   m_btnAbort;
    CButton   m_btnSaveFrame;
    CButton   m_btnClear;
    CFont     m_font;
    bool      m_connected{false};
};
