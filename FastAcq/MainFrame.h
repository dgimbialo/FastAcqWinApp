#pragma once
//
// MainFrame -- top-level CFrameWnd: frame list, tabs (Radar / Scope /
// Communication / Settings / Trace), command panel, status bar. Owns the store, the
// serial reader, the DSP worker, the session recorder and the replay player.
//

#include "pch.h"
#include "AppSettings.h"
#include "ChirpListCtrl.h"
#include "ChirpStore.h"
#include "CommLogWnd.h"
#include "CommandPanel.h"
#include "Core/SessionFile.h"
#include "DspWorker.h"
#include "RadarTab.h"
#include "ScopeTab.h"
#include "SerialWorker.h"
#include "SettingsTab.h"
#include "TraceTab.h"

class CMainFrame : public CFrameWnd {
public:
    CMainFrame();
    ~CMainFrame() override;

    BOOL PreTranslateMessage(MSG* pMsg) override;
    bool WantMaximized() const { return m_settings.windowMax; }

protected:
    afx_msg int  OnCreate(LPCREATESTRUCT lpcs);
    afx_msg void OnSize(UINT, int, int);
    afx_msg void OnDestroy();
    afx_msg void OnClose();
    afx_msg void OnTimer(UINT_PTR id);
    afx_msg void OnTabSelChange(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg LRESULT OnDpiChanged(WPARAM wp, LPARAM lp);

    afx_msg LRESULT OnFrameReady    (WPARAM wp, LPARAM lp);
    afx_msg LRESULT OnPortStatus    (WPARAM wp, LPARAM lp);
    afx_msg LRESULT OnLinkStats     (WPARAM wp, LPARAM lp);
    afx_msg LRESULT OnFrameSelected (WPARAM wp, LPARAM lp);
    afx_msg LRESULT OnResultReady   (WPARAM wp, LPARAM lp);
    afx_msg LRESULT OnCommLog       (WPARAM wp, LPARAM lp);
    afx_msg LRESULT OnServiceFrame  (WPARAM wp, LPARAM lp);
    afx_msg LRESULT OnRefreshPorts  (WPARAM, LPARAM);
    afx_msg LRESULT OnSettingsChanged(WPARAM wp, LPARAM);
    afx_msg LRESULT OnResetAverages (WPARAM, LPARAM);

    afx_msg LRESULT OnCmdConnect     (WPARAM, LPARAM);
    afx_msg LRESULT OnCmdDisconnect  (WPARAM, LPARAM);
    afx_msg LRESULT OnCmdStart       (WPARAM, LPARAM);
    afx_msg LRESULT OnCmdStop        (WPARAM, LPARAM);
    afx_msg LRESULT OnCmdSetFreq     (WPARAM wp, LPARAM);
    afx_msg LRESULT OnCmdSetSamples  (WPARAM wp, LPARAM);
    afx_msg LRESULT OnCmdPing        (WPARAM, LPARAM);
    afx_msg LRESULT OnCmdSaveFrame   (WPARAM, LPARAM);
    afx_msg LRESULT OnCmdClear       (WPARAM, LPARAM);
    afx_msg LRESULT OnCmdSetMode     (WPARAM wp, LPARAM);
    afx_msg LRESULT OnCmdSetDataMask (WPARAM wp, LPARAM);
    afx_msg LRESULT OnCmdSetInterval (WPARAM wp, LPARAM);
    afx_msg LRESULT OnCmdTrigger     (WPARAM, LPARAM);
    afx_msg LRESULT OnCmdGetStatus   (WPARAM, LPARAM);
    afx_msg LRESULT OnCmdSetAmplitude(WPARAM wp, LPARAM);
    afx_msg LRESULT OnCmdSetBurst    (WPARAM wp, LPARAM);
    afx_msg LRESULT OnCmdAbort       (WPARAM, LPARAM);
    afx_msg LRESULT OnCmdHold        (WPARAM wp, LPARAM);
    afx_msg LRESULT OnCmdRecord      (WPARAM, LPARAM);
    afx_msg LRESULT OnCmdOpenReplay  (WPARAM, LPARAM);
    afx_msg LRESULT OnReplayCtrl     (WPARAM wp, LPARAM lp);
    afx_msg LRESULT OnCmdSetTrace    (WPARAM wp, LPARAM);
    afx_msg LRESULT OnCmdSetRamp     (WPARAM wp, LPARAM);
    afx_msg LRESULT OnCmdGetTrace    (WPARAM, LPARAM);
    afx_msg LRESULT OnCmdSingleShot  (WPARAM, LPARAM);

    // Menu
    afx_msg void OnFileOpenReplay();
    afx_msg void OnFileCloseReplay();
    afx_msg void OnFileRecord();
    afx_msg void OnFileSaveFrame();
    afx_msg void OnFileExportTargets();
    afx_msg void OnFileExportWav();
    afx_msg void OnFileScreenshot();
    afx_msg void OnFileExit();
    afx_msg void OnViewTab(UINT id);
    afx_msg void OnViewDark();
    afx_msg void OnUpdateViewDark(CCmdUI* pCmdUI);
    afx_msg void OnUpdateCloseReplay(CCmdUI* pCmdUI);
    afx_msg void OnUpdateRecord(CCmdUI* pCmdUI);
    afx_msg void OnUpdateHold(CCmdUI* pCmdUI);
    afx_msg void OnAcqConnect();
    afx_msg void OnAcqStart();
    afx_msg void OnAcqStop();
    afx_msg void OnAcqTrigger();
    afx_msg void OnAcqAbort();
    afx_msg void OnAcqHold();
    afx_msg void OnAcqResetAvg();
    afx_msg void OnAcqPing();
    afx_msg void OnAcqStatus();
    afx_msg void OnHelpAbout();
    afx_msg void OnHelpKeys();

    DECLARE_MESSAGE_MAP()

private:
    enum { kTimerReplay = 1, kTimerPostConnect = 2, kTimerStatus = 3 };

    void RelayoutClient();
    void SelectTab(int idx);
    void UpdateStatusBar();
    void ApplySettingsToAll(bool syncSettingsTab);
    void ApplyTheme();
    void SaveSettings();
    void ReprocessCurrent();
    void SetLive(bool live);
    void ShowSelectedFrame(uint64_t seq);
    void SendCmd(uint8_t cmd, uint16_t a1 = 0, uint16_t a2 = 0, uint16_t a3 = 0);
    bool Connected() const { return m_serial && m_serial->IsOpen(); }

    // Recording / replay
    void StartRecording();
    void StopRecording();
    bool OpenReplay(const CString& path);
    void CloseReplay();
    void ReplayShow(size_t index, bool fromTimer);
    void ReplayArmTimer();
    void ReplaySetPlaying(bool playing);

    // Exports
    ChirpFramePtr CurrentFrame() const;
    void ExportTargetsCsv(const CString& path);
    void ExportWav(const CString& path);
    CString DefaultFileName(LPCTSTR prefix, LPCTSTR ext) const;

    CStatusBar     m_status;
    CTabCtrl       m_tab;
    CFont          m_tabFont;
    HICON          m_hIcon{nullptr};
    ChirpListCtrl  m_list;
    CommandPanel   m_cmd;
    RadarTab       m_radarTab;
    ScopeTab       m_scopeTab;
    CommLogWnd     m_logTab;
    SettingsTab    m_settingsTab;
    TraceTab       m_traceTab;

    AppSettings                   m_settings;
    ChirpStore                    m_store;
    std::unique_ptr<SerialWorker> m_serial;
    std::unique_ptr<DspWorker>    m_dsp;

    // Connection / device
    bool     m_connected{false};
    bool     m_running{false};
    CString  m_portName;
    CString  m_mcuStatus;
    DeviceStatus m_device;
    bool     m_haveDevice{false};
    DWORD    m_pingSentTick{0};
    bool     m_pingPending{false};
    DWORD    m_lastRttMs{0};
    LinkStats m_link;
    CString  m_lastWarn;          // last MCU command rejection (status bar, a few seconds)
    DWORD    m_lastWarnTick{0};

    // Display state
    bool     m_live{true};
    uint64_t m_latestSeq{ChirpStore::kInvalidSeq};
    uint64_t m_currentSeq{ChirpStore::kInvalidSeq};
    std::shared_ptr<const dsp::FrameResult> m_lastResult;
    double   m_lastDspMs{0.0};
    double   m_obsFs{0.0};
    size_t   m_obsSamples{0};
    size_t   m_obsMcuFft{0};

    // Recording
    core::SessionWriter m_recorder;
    bool     m_recording{false};
    CString  m_recordPath;

    // Replay
    core::SessionReader m_replay;
    bool     m_replayActive{false};
    bool     m_replayPlaying{false};
    size_t   m_replayIndex{0};
    int      m_replaySpeedPct{100};   // 0 = as fast as possible
    CString  m_replayName;
};

CFrameWnd* CreateMainFrame();
