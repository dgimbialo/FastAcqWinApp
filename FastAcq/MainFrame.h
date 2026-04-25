#pragma once
//
// MainFrame -- top-level CFrameWnd hosting list, tabs, command panel, status bar.
//

#include "pch.h"
#include "ChirpStore.h"
#include "ChirpListCtrl.h"
#include "CommandPanel.h"
#include "Tab1Wnd.h"
#include "CommLogWnd.h"
#include "SerialWorker.h"
#include "LocalFft.h"
#include <memory>

class CMainFrame : public CFrameWnd {
public:
    CMainFrame();
    ~CMainFrame() override;

protected:
    afx_msg int  OnCreate(LPCREATESTRUCT lpcs);
    afx_msg void OnSize(UINT, int, int);
    afx_msg void OnDestroy();
    afx_msg void OnTabSelChange(NMHDR* pNMHDR, LRESULT* pResult);

    afx_msg LRESULT OnFrameReady   (WPARAM wp, LPARAM lp);
    afx_msg LRESULT OnPortStatus   (WPARAM wp, LPARAM lp);
    afx_msg LRESULT OnParseStats   (WPARAM wp, LPARAM lp);
    afx_msg LRESULT OnPongRx       (WPARAM wp, LPARAM lp);
    afx_msg LRESULT OnFrameSelected(WPARAM wp, LPARAM lp);

    afx_msg LRESULT OnCmdConnect    (WPARAM, LPARAM);
    afx_msg LRESULT OnCmdDisconnect (WPARAM, LPARAM);
    afx_msg LRESULT OnCmdStart      (WPARAM, LPARAM);
    afx_msg LRESULT OnCmdStop       (WPARAM, LPARAM);
    afx_msg LRESULT OnCmdSetFreq    (WPARAM, LPARAM);
    afx_msg LRESULT OnCmdSetSamples (WPARAM, LPARAM);
    afx_msg LRESULT OnCmdPing       (WPARAM, LPARAM);
    afx_msg LRESULT OnCmdSaveFrame  (WPARAM, LPARAM);
    afx_msg LRESULT OnCmdClear      (WPARAM, LPARAM);
    afx_msg LRESULT OnCommLog       (WPARAM, LPARAM);
    afx_msg LRESULT OnAcqMode       (WPARAM, LPARAM); // wParam=0 RAW, 1 FFT
    afx_msg LRESULT OnFftSettings   (WPARAM, LPARAM); // lParam=new FftSettings*
    afx_msg LRESULT OnCmdSetMode     (WPARAM, LPARAM);
    afx_msg LRESULT OnCmdSetDataMask (WPARAM, LPARAM);
    afx_msg LRESULT OnCmdSetInterval (WPARAM, LPARAM);
    afx_msg LRESULT OnCmdTrigger     (WPARAM, LPARAM);
    afx_msg LRESULT OnCmdGetStatus   (WPARAM, LPARAM);

    DECLARE_MESSAGE_MAP()

private:
    void RelayoutClient();
    void UpdateStatusBar();
    void ShowFrameAt(size_t logicalIdx);
    bool SaveFrameCsv(const ChirpFrame& f, const CString& path);

    CStatusBar     m_status;
    CTabCtrl       m_tab;
    CFont          m_tabFont;
    ChirpListCtrl  m_list;
    CommandPanel   m_cmd;
    Tab1Wnd        m_tab1;
    Tab2Wnd        m_tab2;
    CommLogWnd     m_tab3;

    ChirpStore                    m_store;
    std::unique_ptr<SerialWorker> m_serial;

    // Status tracking
    bool     m_connected{false};
    CString  m_portName;
    uint64_t m_framesShown{0};
    float    m_lastPeakHz{0.0f};
    uint32_t m_lastTsMs{0};

    // Ping tracking
    DWORD    m_pingSentTick{0};
    bool     m_pingPending{false};
    DWORD    m_lastRttMs{0};

    // Currently displayed frame (for Save)
    size_t   m_currentIdx{static_cast<size_t>(-1)};

    // PC-side acquisition mode
    bool        m_rawMode{true};
    FftSettings m_fftSettings;
    uint32_t    m_sampleRateHz{60000000};  // 60 MS/s from firmware
};

CFrameWnd* CreateMainFrame();
