#pragma once
//
// CommandPanel -- main toolbar: COM port, Connect, Start/Stop, Trigger,
// Abort, Hold/Live, Record, Open replay, Save frame, Clear; plus a second
// row with the replay transport controls when a session file is loaded.
//

#include "pch.h"

class CommandPanel : public CWnd {
public:
    CommandPanel() = default;

    BOOL CreatePanel(CWnd* parent, UINT id);

    int  DesiredHeight() const;

    void SetConnected(bool c);
    void SetRunning(bool running);
    void SetHold(bool hold);
    void SetRecording(bool rec, const CString& info);
    void SetReplay(bool active, const CString& name, int count);
    void SetReplayPos(int index, bool playing, const CString& info);
    void PopulateComPorts(const std::vector<CString>& ports, const CString& preferred);
    CString GetSelectedPort() const;
    bool ReplayActive() const { return m_replay; }
    void ApplyTheme();

protected:
    afx_msg int    OnCreate(LPCREATESTRUCT lpcs);
    afx_msg void   OnSize(UINT, int, int);
    afx_msg BOOL   OnEraseBkgnd(CDC* pDC);
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    afx_msg void   OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pBar);
    afx_msg void   OnConnect();
    afx_msg void   OnStart();
    afx_msg void   OnStop();
    afx_msg void   OnTrigger();
    afx_msg void   OnAbort();
    afx_msg void   OnHold();
    afx_msg void   OnRecord();
    afx_msg void   OnOpenReplay();
    afx_msg void   OnSaveFrame();
    afx_msg void   OnClear();
    afx_msg void   OnComDropDown();
    afx_msg void   OnRpPlay();
    afx_msg void   OnRpFirst();
    afx_msg void   OnRpPrev();
    afx_msg void   OnRpNext();
    afx_msg void   OnRpLast();
    afx_msg void   OnRpClose();
    afx_msg void   OnRpSpeed();
    DECLARE_MESSAGE_MAP()

private:
    void Relayout();
    void Post(UINT msg, WPARAM wp = 0, LPARAM lp = 0);

    CComboBox m_cmbCom;
    CButton   m_btnConnect, m_btnStart, m_btnStop, m_btnTrigger, m_btnAbort;
    CButton   m_btnHold, m_btnRecord, m_btnOpen, m_btnSaveFrame, m_btnClear;
    CStatic   m_lblRec;
    // Replay row
    CStatic     m_lblRpName;
    CButton     m_btnRpFirst, m_btnRpPrev, m_btnRpPlay, m_btnRpNext, m_btnRpLast, m_btnRpClose;
    CSliderCtrl m_sldRp;
    CComboBox   m_cmbRpSpeed;
    CStatic     m_lblRpInfo;

    CFont   m_font;
    CBrush  m_bgBrush;
    bool    m_connected{false};
    bool    m_running{false};
    bool    m_hold{false};
    bool    m_recording{false};
    bool    m_replay{false};
    bool    m_playing{false};
    int     m_rpCount{0};
};
