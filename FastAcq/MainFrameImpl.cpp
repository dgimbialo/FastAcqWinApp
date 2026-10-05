#include "pch.h"
#include "MainFrame.h"
#include "AppMessages.h"
#include "Core/Export.h"
#include "Dpi.h"
#include "Screenshot.h"
#include "Theme.h"
#include "resource.h"

#include <chrono>
#include <filesystem>

static UINT s_statusIndicators[] = {
    IDS_PANE_CONN, IDS_PANE_MCU, IDS_PANE_RATE, IDS_PANE_LINK, IDS_PANE_DSP, IDS_PANE_BUF
};

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_DESTROY()
    ON_WM_CLOSE()
    ON_WM_TIMER()
    ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_CTRL, &CMainFrame::OnTabSelChange)
    ON_MESSAGE(WM_DPICHANGED,            &CMainFrame::OnDpiChanged)
    ON_MESSAGE(WM_APP_FRAME_READY,       &CMainFrame::OnFrameReady)
    ON_MESSAGE(WM_APP_PORT_STATUS,       &CMainFrame::OnPortStatus)
    ON_MESSAGE(WM_APP_LINK_STATS,        &CMainFrame::OnLinkStats)
    ON_MESSAGE(WM_APP_FRAME_SELECTED,    &CMainFrame::OnFrameSelected)
    ON_MESSAGE(WM_APP_RESULT_READY,      &CMainFrame::OnResultReady)
    ON_MESSAGE(WM_APP_COMM_LOG,          &CMainFrame::OnCommLog)
    ON_MESSAGE(WM_APP_SERVICE_FRAME,     &CMainFrame::OnServiceFrame)
    ON_MESSAGE(WM_APP_REFRESH_PORTS,     &CMainFrame::OnRefreshPorts)
    ON_MESSAGE(WM_APP_SETTINGS_CHANGED,  &CMainFrame::OnSettingsChanged)
    ON_MESSAGE(WM_APP_RESET_AVERAGES,    &CMainFrame::OnResetAverages)
    ON_MESSAGE(WM_APP_CMD_CONNECT,       &CMainFrame::OnCmdConnect)
    ON_MESSAGE(WM_APP_CMD_DISCONNECT,    &CMainFrame::OnCmdDisconnect)
    ON_MESSAGE(WM_APP_CMD_START,         &CMainFrame::OnCmdStart)
    ON_MESSAGE(WM_APP_CMD_STOP,          &CMainFrame::OnCmdStop)
    ON_MESSAGE(WM_APP_CMD_SET_FREQ,      &CMainFrame::OnCmdSetFreq)
    ON_MESSAGE(WM_APP_CMD_SET_SAMPLES,   &CMainFrame::OnCmdSetSamples)
    ON_MESSAGE(WM_APP_CMD_PING,          &CMainFrame::OnCmdPing)
    ON_MESSAGE(WM_APP_CMD_SAVE_FRAME,    &CMainFrame::OnCmdSaveFrame)
    ON_MESSAGE(WM_APP_CMD_CLEAR,         &CMainFrame::OnCmdClear)
    ON_MESSAGE(WM_APP_CMD_SET_MODE,      &CMainFrame::OnCmdSetMode)
    ON_MESSAGE(WM_APP_CMD_SET_DATA_MASK, &CMainFrame::OnCmdSetDataMask)
    ON_MESSAGE(WM_APP_CMD_SET_INTERVAL,  &CMainFrame::OnCmdSetInterval)
    ON_MESSAGE(WM_APP_CMD_TRIGGER,       &CMainFrame::OnCmdTrigger)
    ON_MESSAGE(WM_APP_CMD_GET_STATUS,    &CMainFrame::OnCmdGetStatus)
    ON_MESSAGE(WM_APP_CMD_SET_AMPLITUDE, &CMainFrame::OnCmdSetAmplitude)
    ON_MESSAGE(WM_APP_CMD_SET_BURST,     &CMainFrame::OnCmdSetBurst)
    ON_MESSAGE(WM_APP_CMD_ABORT,         &CMainFrame::OnCmdAbort)
    ON_MESSAGE(WM_APP_CMD_HOLD,          &CMainFrame::OnCmdHold)
    ON_MESSAGE(WM_APP_CMD_RECORD,        &CMainFrame::OnCmdRecord)
    ON_MESSAGE(WM_APP_CMD_OPEN_REPLAY,   &CMainFrame::OnCmdOpenReplay)
    ON_MESSAGE(WM_APP_REPLAY_CTRL,       &CMainFrame::OnReplayCtrl)
    ON_COMMAND(ID_FILE_OPEN_REPLAY,    &CMainFrame::OnFileOpenReplay)
    ON_COMMAND(ID_FILE_CLOSE_REPLAY,   &CMainFrame::OnFileCloseReplay)
    ON_COMMAND(ID_FILE_RECORD,         &CMainFrame::OnFileRecord)
    ON_COMMAND(ID_FILE_SAVE_FRAME,     &CMainFrame::OnFileSaveFrame)
    ON_COMMAND(ID_FILE_EXPORT_TARGETS, &CMainFrame::OnFileExportTargets)
    ON_COMMAND(ID_FILE_EXPORT_WAV,     &CMainFrame::OnFileExportWav)
    ON_COMMAND(ID_FILE_SCREENSHOT,     &CMainFrame::OnFileScreenshot)
    ON_COMMAND(ID_FILE_EXIT,           &CMainFrame::OnFileExit)
    ON_COMMAND_RANGE(ID_VIEW_RADAR, ID_VIEW_SETTINGS, &CMainFrame::OnViewTab)
    ON_COMMAND(ID_VIEW_DARK,           &CMainFrame::OnViewDark)
    ON_UPDATE_COMMAND_UI(ID_VIEW_DARK,         &CMainFrame::OnUpdateViewDark)
    ON_UPDATE_COMMAND_UI(ID_FILE_CLOSE_REPLAY, &CMainFrame::OnUpdateCloseReplay)
    ON_UPDATE_COMMAND_UI(ID_FILE_RECORD,       &CMainFrame::OnUpdateRecord)
    ON_UPDATE_COMMAND_UI(ID_ACQ_HOLD,          &CMainFrame::OnUpdateHold)
    ON_COMMAND(ID_ACQ_CONNECT,   &CMainFrame::OnAcqConnect)
    ON_COMMAND(ID_ACQ_START,     &CMainFrame::OnAcqStart)
    ON_COMMAND(ID_ACQ_STOP,      &CMainFrame::OnAcqStop)
    ON_COMMAND(ID_ACQ_TRIGGER,   &CMainFrame::OnAcqTrigger)
    ON_COMMAND(ID_ACQ_ABORT,     &CMainFrame::OnAcqAbort)
    ON_COMMAND(ID_ACQ_HOLD,      &CMainFrame::OnAcqHold)
    ON_COMMAND(ID_ACQ_RESET_AVG, &CMainFrame::OnAcqResetAvg)
    ON_COMMAND(ID_ACQ_PING,      &CMainFrame::OnAcqPing)
    ON_COMMAND(ID_ACQ_STATUS,    &CMainFrame::OnAcqStatus)
    ON_COMMAND(ID_HELP_ABOUT,    &CMainFrame::OnHelpAbout)
    ON_COMMAND(ID_HELP_KEYS,     &CMainFrame::OnHelpKeys)
END_MESSAGE_MAP()

namespace {

uint64_t UnixNowMs()
{
    using namespace std::chrono;
    return static_cast<uint64_t>(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
}

std::filesystem::path ToPath(const CString& s)
{
    return std::filesystem::path(std::wstring(CStringW(s)));
}

} // namespace

// ---------------------------------------------------------------------------
CMainFrame::CMainFrame()
{
    m_settings.Load(AppSettings::DefaultPath());
    if (m_settings.chirpsFromBurst) m_settings.dsp.chirpsInFrame = (std::max)(1, m_settings.acq.burst);
    Theme::SetDark(m_settings.display.darkTheme);
}

CMainFrame::~CMainFrame() = default;

int CMainFrame::OnCreate(LPCREATESTRUCT lpcs)
{
    if (CFrameWnd::OnCreate(lpcs) == -1)
        return -1;

    if (!m_status.Create(this) ||
        !m_status.SetIndicators(s_statusIndicators, sizeof(s_statusIndicators) / sizeof(UINT)))
    {
        TRACE0("Failed to create status bar\n");
    }
    const int S = static_cast<int>(Dpi::Of(m_hWnd));
    auto sc = [&](int px) { return ::MulDiv(px, S, 96); };
    m_status.SetPaneInfo(0, IDS_PANE_CONN, SBPS_STRETCH, 0);
    m_status.SetPaneInfo(1, IDS_PANE_MCU,  SBPS_NORMAL, sc(330));
    m_status.SetPaneInfo(2, IDS_PANE_RATE, SBPS_NORMAL, sc(300));
    m_status.SetPaneInfo(3, IDS_PANE_LINK, SBPS_NORMAL, sc(160));
    m_status.SetPaneInfo(4, IDS_PANE_DSP,  SBPS_NORMAL, sc(90));
    m_status.SetPaneInfo(5, IDS_PANE_BUF,  SBPS_NORMAL, sc(150));

    m_hIcon = AfxGetApp()->LoadIcon(IDI_APPICON);
    if (m_hIcon) { SetIcon(m_hIcon, TRUE); SetIcon(m_hIcon, FALSE); }

    m_list.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
                  CRect(0, 0, 220, 400), this, IDC_CHIRP_LIST);
    m_list.InitColumns();
    m_list.SetMaxRows(static_cast<int>(m_store.Capacity()));

    m_tab.Create(WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | TCS_TABS, CRect(0, 0, 10, 10), this, IDC_TAB_CTRL);
    Dpi::MakeFont(m_tabFont, m_hWnd, 9);
    m_tab.SetFont(&m_tabFont);
    TCITEM ti{}; ti.mask = TCIF_TEXT;
    ti.pszText = const_cast<LPTSTR>(_T("Radar"));          m_tab.InsertItem(0, &ti);
    ti.pszText = const_cast<LPTSTR>(_T("Scope"));          m_tab.InsertItem(1, &ti);
    ti.pszText = const_cast<LPTSTR>(_T("Communication"));  m_tab.InsertItem(2, &ti);
    ti.pszText = const_cast<LPTSTR>(_T("Settings"));       m_tab.InsertItem(3, &ti);

    m_radarTab.CreateTab(&m_tab, IDC_TAB_RADAR);
    m_scopeTab.CreateTab(&m_tab, IDC_TAB_SCOPE);
    m_logTab.CreateTab(&m_tab, IDC_TAB_COMM);
    m_settingsTab.CreateTab(&m_tab, IDC_TAB_SETTINGS);

    m_cmd.CreatePanel(this, IDC_CMD_PANEL);
    m_cmd.PopulateComPorts(SerialWorker::EnumPorts(), m_settings.lastPort);

    m_serial = std::make_unique<SerialWorker>(m_store, GetSafeHwnd());
    m_dsp    = std::make_unique<DspWorker>(GetSafeHwnd());
    m_dsp->Start();

    m_settingsTab.ApplySettings(m_settings);
    m_radarTab.SetSplits(m_settings.splitRadar1, m_settings.splitRadar2);
    m_scopeTab.SetSplit(m_settings.splitScope);
    ApplySettingsToAll(false);
    ApplyTheme();

    int tab = m_settings.activeTab;
    if (tab < 0 || tab > 3) tab = 0;
    m_tab.SetCurSel(tab);
    SelectTab(tab);

    SetTimer(kTimerStatus, 1000, nullptr);
    UpdateStatusBar();

    if (m_settings.autoConnect && !m_cmd.GetSelectedPort().IsEmpty())
        PostMessage(WM_APP_CMD_CONNECT);
    return 0;
}

void CMainFrame::OnDestroy()
{
    KillTimer(kTimerStatus);
    KillTimer(kTimerReplay);
    if (m_serial) m_serial->Close();
    if (m_dsp) m_dsp->Stop();
    if (m_recording) StopRecording();
    // Both workers are stopped: free the payloads of pointer-carrying messages
    // that are still queued for this window (they would never be dispatched).
    MSG msg;
    while (::PeekMessage(&msg, m_hWnd, WM_APP, WM_APP + 0xFF, PM_REMOVE)) {
        switch (msg.message) {
        case WM_APP_COMM_LOG:      delete reinterpret_cast<CString*>(msg.lParam); break;
        case WM_APP_LINK_STATS:    delete reinterpret_cast<LinkStats*>(msg.lParam); break;
        case WM_APP_SERVICE_FRAME: delete reinterpret_cast<ChirpFrame*>(msg.lParam); break;
        case WM_APP_RESULT_READY:  delete reinterpret_cast<std::shared_ptr<const dsp::FrameResult>*>(msg.lParam); break;
        default: break;
        }
    }
    CFrameWnd::OnDestroy();
}

void CMainFrame::OnClose()
{
    WINDOWPLACEMENT wp{}; wp.length = sizeof(wp);
    if (GetWindowPlacement(&wp)) {
        m_settings.windowRect = wp.rcNormalPosition;
        m_settings.windowMax  = (wp.showCmd == SW_SHOWMAXIMIZED);
    }
    m_settings.activeTab = m_tab.GetCurSel();
    m_radarTab.GetSplits(m_settings.splitRadar1, m_settings.splitRadar2);
    m_settings.splitScope = m_scopeTab.GetSplit();
    m_settings.lastPort   = m_cmd.GetSelectedPort();
    SaveSettings();
    if (m_recording) StopRecording();
    if (m_replayActive) CloseReplay();
    CFrameWnd::OnClose();
}

void CMainFrame::OnSize(UINT t, int cx, int cy)
{
    CFrameWnd::OnSize(t, cx, cy);
    RelayoutClient();
}

LRESULT CMainFrame::OnDpiChanged(WPARAM, LPARAM lp)
{
    const RECT* prc = reinterpret_cast<const RECT*>(lp);
    if (prc) ::SetWindowPos(m_hWnd, nullptr, prc->left, prc->top, prc->right - prc->left, prc->bottom - prc->top,
                            SWP_NOZORDER | SWP_NOACTIVATE);
    Dpi::MakeFont(m_tabFont, m_hWnd, 9);
    m_tab.SetFont(&m_tabFont);
    RelayoutClient();
    return 0;
}

void CMainFrame::RelayoutClient()
{
    RepositionBars(AFX_IDW_CONTROLBAR_FIRST, AFX_IDW_CONTROLBAR_LAST, 0);
    CRect client; GetClientRect(&client);
    RepositionBars(AFX_IDW_CONTROLBAR_FIRST, AFX_IDW_CONTROLBAR_LAST, 0, reposQuery, &client);

    const int cmdH  = m_cmd.GetSafeHwnd() ? m_cmd.DesiredHeight() : Dpi::Scale(m_hWnd, 32);
    const int listW = Dpi::Scale(m_hWnd, 296);
    const int pad   = Dpi::Scale(m_hWnd, 2);

    int areaTop    = client.top;
    int areaBottom = client.bottom - cmdH;
    if (areaBottom < areaTop + 10) areaBottom = areaTop + 10;

    if (m_list.GetSafeHwnd())
        m_list.MoveWindow(client.left, areaTop, listW, areaBottom - areaTop);

    int tabX = client.left + listW + pad;
    int tabW = client.right - tabX;
    if (tabW < 10) tabW = 10;
    if (m_tab.GetSafeHwnd()) {
        m_tab.MoveWindow(tabX, areaTop, tabW, areaBottom - areaTop);
        CRect trc; m_tab.GetClientRect(&trc);
        m_tab.AdjustRect(FALSE, &trc);
        for (CWnd* w : { static_cast<CWnd*>(&m_radarTab), static_cast<CWnd*>(&m_scopeTab),
                         static_cast<CWnd*>(&m_logTab), static_cast<CWnd*>(&m_settingsTab) })
            if (w->GetSafeHwnd()) w->MoveWindow(trc);
    }
    if (m_cmd.GetSafeHwnd())
        m_cmd.MoveWindow(client.left, areaBottom, client.Width(), cmdH);
}

void CMainFrame::SelectTab(int idx)
{
    m_radarTab.ShowWindow(idx == 0 ? SW_SHOW : SW_HIDE);
    m_scopeTab.ShowWindow(idx == 1 ? SW_SHOW : SW_HIDE);
    m_logTab.ShowWindow(idx == 2 ? SW_SHOW : SW_HIDE);
    m_settingsTab.ShowWindow(idx == 3 ? SW_SHOW : SW_HIDE);
}

void CMainFrame::OnTabSelChange(NMHDR*, LRESULT* pResult)
{
    SelectTab(m_tab.GetCurSel());
    *pResult = 0;
}

void CMainFrame::OnViewTab(UINT id)
{
    int idx = static_cast<int>(id - ID_VIEW_RADAR);
    if (idx < 0 || idx > 3) return;
    m_tab.SetCurSel(idx);
    SelectTab(idx);
}

// ---------------------------------------------------------------------------
BOOL CMainFrame::PreTranslateMessage(MSG* pMsg)
{
    if (pMsg->message == WM_MOUSEWHEEL) {
        // Route the wheel to the window under the cursor (not the focused one).
        POINT pt{ static_cast<short>(LOWORD(pMsg->lParam)), static_cast<short>(HIWORD(pMsg->lParam)) };
        HWND h = ::WindowFromPoint(pt);
        if (h && h != pMsg->hwnd && ::IsChild(m_hWnd, h)) {
            ::SendMessage(h, WM_MOUSEWHEEL, pMsg->wParam, pMsg->lParam);
            return TRUE;
        }
    }
    if (pMsg->message == WM_KEYDOWN) {
        TCHAR cls[32]{};
        ::GetClassName(pMsg->hwnd, cls, 31);
        const bool typing = (_tcsicmp(cls, _T("Edit")) == 0 || _tcsicmp(cls, _T("ComboBox")) == 0);
        const bool ctrl   = ::GetKeyState(VK_CONTROL) < 0;
        switch (pMsg->wParam) {
        case VK_SPACE:
            if (!typing) { if (m_running) OnAcqStop(); else OnAcqStart(); return TRUE; }
            break;
        case 'T': if (!typing && !ctrl) { OnAcqTrigger(); return TRUE; } break;
        case 'H': if (!typing && !ctrl) { OnAcqHold(); return TRUE; } break;
        case 'R': if (!typing && !ctrl) { OnFileRecord(); return TRUE; } break;
        case 'S': if (ctrl) { OnFileSaveFrame(); return TRUE; } break;
        case 'O': if (ctrl) { OnFileOpenReplay(); return TRUE; } break;
        case '1': case '2': case '3': case '4':
            if (ctrl) { OnViewTab(ID_VIEW_RADAR + static_cast<UINT>(pMsg->wParam - '1')); return TRUE; }
            break;
        case VK_F5:  OnAcqConnect(); return TRUE;
        case VK_F12: OnFileScreenshot(); return TRUE;
        case VK_PRIOR: if (!typing) { if (m_list.SelectRelative(-1)) return TRUE; } break;
        case VK_NEXT:  if (!typing) { if (m_list.SelectRelative(+1)) return TRUE; } break;
        case VK_LEFT:  if (ctrl) { if (m_replayActive) OnReplayCtrl(REPLAY_PREV, 0); else m_list.SelectRelative(-1); return TRUE; } break;
        case VK_RIGHT: if (ctrl) { if (m_replayActive) OnReplayCtrl(REPLAY_NEXT, 0); else m_list.SelectRelative(+1); return TRUE; } break;
        default: break;
        }
    }
    return CFrameWnd::PreTranslateMessage(pMsg);
}

// ---------------------------------------------------------------------------
void CMainFrame::SendCmd(uint8_t cmd, uint16_t a1, uint16_t a2, uint16_t a3)
{
    if (!Connected()) return;
    m_serial->SendCommand(cmd, a1, a2, a3);
}

LRESULT CMainFrame::OnFrameReady(WPARAM wp, LPARAM)
{
    const uint64_t seq = static_cast<uint64_t>(wp);
    ChirpFramePtr f = m_store.Get(seq);
    if (!f) return 0;

    m_latestSeq = seq;
    m_list.AddFrame(seq, f->header.frame_id, f->header.timestamp_ms, f->raw.size());

    if (m_recording && !m_replayActive) {
        if (!m_recorder.Write(*f)) {
            m_logTab.AppendLine(LOG_ERR, _T("Recording: write failed, stopping"));
            StopRecording();
        } else if (m_recorder.FramesWritten() % 10 == 1) {
            CString info;
            info.Format(_T("REC %s  %llu frames  %.1f MB"), m_recordPath.GetString(),
                        static_cast<unsigned long long>(m_recorder.FramesWritten()), m_recorder.BytesWritten() / 1e6);
            m_cmd.SetRecording(true, info);
        }
    }

    if (f->header.sample_rate_hz > 0) m_obsFs = f->header.sample_rate_hz;
    m_obsSamples = f->raw.size();
    m_obsMcuFft  = f->fft.size() * 2;

    m_dsp->Submit(f, true);
    return 0;
}

LRESULT CMainFrame::OnResultReady(WPARAM wp, LPARAM lp)
{
    std::unique_ptr<std::shared_ptr<const dsp::FrameResult>> payload(
        reinterpret_cast<std::shared_ptr<const dsp::FrameResult>*>(lp));
    if (!payload || !*payload) return 0;
    std::shared_ptr<const dsp::FrameResult> r = *payload;
    const bool live = (wp != 0);

    m_lastDspMs = r->processingMs;
    m_list.UpdateResult(*r);

    if (live && !m_live) { UpdateStatusBar(); return 0; }   // holding: keep the selected frame

    m_lastResult = r;
    m_currentSeq = r->seq;
    ChirpFramePtr f = m_store.Get(r->seq);
    m_radarTab.ShowResult(r, live);
    if (f) m_scopeTab.ShowFrame(f, r);

    static double   s_fs = 0.0; static size_t s_n = 0;
    if (r->fsHz != s_fs || r->samplesPerFrame != s_n) {
        s_fs = r->fsHz; s_n = r->samplesPerFrame;
        m_settingsTab.SetObserved(r->fsHz, r->samplesPerFrame, m_obsMcuFft);
    }
    UpdateStatusBar();
    return 0;
}

LRESULT CMainFrame::OnPortStatus(WPARAM wp, LPARAM)
{
    m_connected = (wp != 0);
    if (!m_connected) {
        // The reader thread exited on its own (device unplugged / read error):
        // release the handle so Connected() reflects reality and the next
        // Connect re-opens the port instead of "disconnecting" first.
        if (m_serial && m_serial->IsOpen()) { m_serial->Close(); return 0; }   // Close() re-posts status 0
        m_running = false; m_haveDevice = false; m_mcuStatus.Empty();
    }
    m_cmd.SetConnected(m_connected);
    m_settingsTab.SetConnected(m_connected);
    if (m_connected) SetTimer(kTimerPostConnect, 300, nullptr);
    UpdateStatusBar();
    return 0;
}

LRESULT CMainFrame::OnLinkStats(WPARAM, LPARAM lp)
{
    std::unique_ptr<LinkStats> st(reinterpret_cast<LinkStats*>(lp));
    if (st) m_link = *st;
    UpdateStatusBar();
    return 0;
}

LRESULT CMainFrame::OnFrameSelected(WPARAM wp, LPARAM)
{
    const uint64_t seq = static_cast<uint64_t>(wp);
    if (m_replayActive && m_replayPlaying) ReplaySetPlaying(false);
    SetLive(false);
    ShowSelectedFrame(seq);
    return 0;
}

void CMainFrame::ShowSelectedFrame(uint64_t seq)
{
    ChirpFramePtr f = m_store.Get(seq);
    if (!f) return;
    m_currentSeq = seq;
    m_dsp->Submit(f, false);
}

void CMainFrame::SetLive(bool live)
{
    m_live = live;
    m_cmd.SetHold(!live);
    if (live) {
        ChirpFramePtr f = m_store.Latest();
        if (f) m_dsp->Submit(f, true);
    }
}

LRESULT CMainFrame::OnCmdHold(WPARAM wp, LPARAM)
{
    SetLive(wp == 0);
    return 0;
}

void CMainFrame::ReprocessCurrent()
{
    ChirpFramePtr f = (m_currentSeq != ChirpStore::kInvalidSeq) ? m_store.Get(m_currentSeq) : m_store.Latest();
    if (f) m_dsp->Submit(f, m_live);
}

LRESULT CMainFrame::OnResetAverages(WPARAM, LPARAM)
{
    m_dsp->ResetState();
    ReprocessCurrent();
    return 0;
}

// ---------------------------------------------------------------------------
LRESULT CMainFrame::OnCmdConnect(WPARAM, LPARAM)
{
    if (!m_serial) return 0;
    if (m_replayActive) CloseReplay();
    CString port = m_cmd.GetSelectedPort();
    if (port.IsEmpty()) { AfxMessageBox(_T("Select a COM port first.")); return 0; }
    if (!m_serial->Open(port)) {
        CString err; err.Format(_T("Failed to open %s"), port.GetString());
        AfxMessageBox(err);
        return 0;
    }
    m_portName = port;
    m_settings.lastPort = port;
    m_serial->SetVerbose(m_settings.verboseLog);
    return 0;
}

LRESULT CMainFrame::OnCmdDisconnect(WPARAM, LPARAM)
{
    if (m_serial) m_serial->Close();
    return 0;
}

LRESULT CMainFrame::OnCmdStart(WPARAM, LPARAM)
{
    if (!Connected()) return 0;
    uint16_t mode = m_settingsTab.GetModeSel();
    if (mode == MODE_IDLE) mode = MODE_CONTINUOUS;
    SendCmd(CMD_START_CHIRP, m_settingsTab.GetFreqHz(), 0, 0);
    SendCmd(CMD_SET_MODE, mode, 0, 0);
    m_running = true;
    m_cmd.SetRunning(true);
    UpdateStatusBar();
    return 0;
}

LRESULT CMainFrame::OnCmdStop(WPARAM, LPARAM)
{
    SendCmd(CMD_SET_MODE, MODE_IDLE, 0, 0);
    m_running = false;
    m_cmd.SetRunning(false);
    UpdateStatusBar();
    return 0;
}

LRESULT CMainFrame::OnCmdSetFreq(WPARAM wp, LPARAM)     { SendCmd(CMD_START_CHIRP, static_cast<uint16_t>(wp)); return 0; }
LRESULT CMainFrame::OnCmdSetSamples(WPARAM wp, LPARAM)
{
    const uint32_t v = static_cast<uint32_t>(wp);
    SendCmd(CMD_SET_SAMPLES, static_cast<uint16_t>(v & 0xFFFF), static_cast<uint16_t>((v >> 16) & 0xFFFF));
    return 0;
}
LRESULT CMainFrame::OnCmdPing(WPARAM, LPARAM)
{
    if (!Connected()) return 0;
    m_pingSentTick = ::GetTickCount();
    m_pingPending  = true;
    SendCmd(CMD_PING);
    return 0;
}
LRESULT CMainFrame::OnCmdSetMode(WPARAM wp, LPARAM)     { SendCmd(CMD_SET_MODE, static_cast<uint16_t>(wp)); m_running = (wp != MODE_IDLE); return 0; }
LRESULT CMainFrame::OnCmdSetDataMask(WPARAM wp, LPARAM) { SendCmd(CMD_SET_DATA_MASK, static_cast<uint16_t>(wp & 0x03)); return 0; }
LRESULT CMainFrame::OnCmdSetInterval(WPARAM wp, LPARAM) { SendCmd(CMD_SET_INTERVAL, static_cast<uint16_t>(wp)); return 0; }
LRESULT CMainFrame::OnCmdTrigger(WPARAM, LPARAM)        { SendCmd(CMD_TRIGGER); return 0; }
LRESULT CMainFrame::OnCmdGetStatus(WPARAM, LPARAM)      { SendCmd(CMD_GET_STATUS); return 0; }
LRESULT CMainFrame::OnCmdSetAmplitude(WPARAM wp, LPARAM){ SendCmd(CMD_SET_AMPLITUDE, static_cast<uint16_t>(wp)); return 0; }
LRESULT CMainFrame::OnCmdSetBurst(WPARAM wp, LPARAM)    { SendCmd(CMD_SET_BURST, static_cast<uint16_t>(wp)); return 0; }
LRESULT CMainFrame::OnCmdAbort(WPARAM, LPARAM)          { SendCmd(CMD_ABORT); return 0; }

LRESULT CMainFrame::OnCmdClear(WPARAM, LPARAM)
{
    m_store.Clear();
    m_list.ClearAll();
    m_radarTab.ClearHistory();
    m_dsp->ResetState();
    m_currentSeq = m_latestSeq = ChirpStore::kInvalidSeq;
    m_lastResult.reset();
    UpdateStatusBar();
    return 0;
}

LRESULT CMainFrame::OnRefreshPorts(WPARAM, LPARAM)
{
    m_cmd.PopulateComPorts(SerialWorker::EnumPorts(), m_settings.lastPort);
    return 0;
}

LRESULT CMainFrame::OnCommLog(WPARAM wp, LPARAM lp)
{
    CString* pLine = reinterpret_cast<CString*>(lp);
    if (pLine) { m_logTab.AppendLine(static_cast<LogKind>(wp), *pLine); delete pLine; }
    return 0;
}

LRESULT CMainFrame::OnServiceFrame(WPARAM wp, LPARAM lp)
{
    std::unique_ptr<ChirpFrame> f(reinterpret_cast<ChirpFrame*>(lp));
    if (!f) return 0;
    const FrameHeader& h = f->header;
    CString line;

    switch (wp) {
    case SVC_FRAME_PONG:
        if (m_pingPending) { m_lastRttMs = ::GetTickCount() - m_pingSentTick; m_pingPending = false; }
        line.Format(_T("PONG  RTT = %lu ms"), m_lastRttMs);
        break;

    case SVC_FRAME_STATUS: {
        m_device = DecodeStatusFrame(h);
        m_haveDevice = true;
        LPCTSTR modeName = (m_device.mode == MODE_IDLE) ? _T("IDLE")
                         : (m_device.mode == MODE_CONTINUOUS) ? _T("CONT")
                         : (m_device.mode == MODE_SINGLE) ? _T("SINGLE") : _T("?");
        CString samples;
        if (m_device.samples == 0) samples = _T("auto"); else samples.Format(_T("%u"), m_device.samples);
        m_mcuStatus.Format(_T("MCU: %s %uHz amp=%u burst=%u int=%ums smp=%s %s err=%u"),
                           modeName, m_device.chirpFreqHz, m_device.amplitude, m_device.burst, m_device.intervalMs,
                           samples.GetString(), m_device.fsmState ? _T("CAPTURING") : _T("IDLE"), m_device.lastError);
        line.Format(_T("STATUS mode=%s mask=0x%02X interval=%u ms samples=%s freq=%u Hz amp=%u burst=%u state=%s err=%u"),
                    modeName, m_device.dataMask, m_device.intervalMs, samples.GetString(), m_device.chirpFreqHz,
                    m_device.amplitude, m_device.burst, m_device.fsmState ? _T("CAPTURING") : _T("IDLE"), m_device.lastError);
        m_running = (m_device.mode != MODE_IDLE);
        // The device is the source of truth: mirror its settings into the UI.
        bool changed = false;
        if (m_settings.acq.chirpFreqHz != m_device.chirpFreqHz && m_device.chirpFreqHz > 0) { m_settings.acq.chirpFreqHz = m_device.chirpFreqHz; changed = true; }
        if (m_settings.acq.burst != m_device.burst)            { m_settings.acq.burst = m_device.burst; changed = true; }
        if (m_settings.acq.amplitude != m_device.amplitude && m_device.amplitude > 0) { m_settings.acq.amplitude = m_device.amplitude; changed = true; }
        if (m_settings.acq.intervalMs != static_cast<int>(m_device.intervalMs) && m_device.intervalMs > 0) { m_settings.acq.intervalMs = static_cast<int>(m_device.intervalMs); changed = true; }
        if (m_settings.acq.samples != static_cast<int>(m_device.samples)) { m_settings.acq.samples = static_cast<int>(m_device.samples); changed = true; }
        if (static_cast<int>(m_device.mode) <= 2 && m_settings.acq.mode != static_cast<int>(m_device.mode)) { m_settings.acq.mode = static_cast<int>(m_device.mode); changed = true; }
        m_settings.acq.sendRaw = (m_device.dataMask & 0x01) != 0;
        m_settings.acq.sendFft = (m_device.dataMask & 0x02) != 0;
        if (m_settings.chirpsFromBurst && m_settings.dsp.chirpsInFrame != m_device.burst) {
            m_settings.dsp.chirpsInFrame = m_device.burst;
            changed = true;
        }
        if (changed) { m_settingsTab.ApplySettings(m_settings); ApplySettingsToAll(false); }
        break;
    }

    case SVC_FRAME_ACK: {
        static LPCTSTR kStatus[] = { _T("OK"), _T("BAD_ARG"), _T("BAD_STATE"), _T("HW_FAIL") };
        LPCTSTR st = (h.fft_peak_bin < 4) ? kStatus[h.fft_peak_bin] : _T("?");
        line.Format(_T("ACK cmd=0x%02X status=%s applied=%u"), h.fft_size, st, h.actual_samples);
        if (h.fft_peak_bin != ACK_OK) m_logTab.AppendLine(LOG_ERR, line);
        break;
    }
    default:
        return 0;
    }
    m_logTab.AppendLine(LOG_SVC, line);
    UpdateStatusBar();
    return 0;
}

// ---------------------------------------------------------------------------
LRESULT CMainFrame::OnSettingsChanged(WPARAM wp, LPARAM)
{
    AppSettings s = m_settings;
    switch (wp) {
    case SETTINGS_FROM_TAB:   m_settingsTab.ReadInto(s); break;
    case SETTINGS_FROM_RADAR: m_radarTab.ReadFooter(s);  break;
    case SETTINGS_FROM_SCOPE: m_scopeTab.ReadFooter(s);  break;
    case SETTINGS_FROM_LOG:   m_logTab.ReadFooter(s);    break;
    default: break;
    }
    if (s.chirpsFromBurst) s.dsp.chirpsInFrame = (std::max)(1, m_haveDevice ? static_cast<int>(m_device.burst) : s.acq.burst);
    const bool themeChanged = (s.display.darkTheme != m_settings.display.darkTheme);
    m_settings = s;
    ApplySettingsToAll(wp != SETTINGS_FROM_TAB);
    if (themeChanged) ApplyTheme();
    SaveSettings();
    ReprocessCurrent();
    return 0;
}

void CMainFrame::ApplySettingsToAll(bool syncSettingsTab)
{
    if (m_dsp) m_dsp->Configure(m_settings.dsp, m_settings.radar, static_cast<double>(m_settings.sampleRateCalHz));
    if (m_serial) m_serial->SetVerbose(m_settings.verboseLog);
    m_radarTab.ApplySettings(m_settings);
    m_scopeTab.ApplySettings(m_settings);
    m_logTab.ApplySettings(m_settings);
    if (syncSettingsTab) m_settingsTab.ApplySettings(m_settings);
    m_settingsTab.SetObserved(m_obsFs, m_obsSamples, m_obsMcuFft);
}

void CMainFrame::ApplyTheme()
{
    Theme::SetDark(m_settings.display.darkTheme);
    m_radarTab.ApplyTheme();
    m_scopeTab.ApplyTheme();
    m_logTab.ApplyTheme();
    m_settingsTab.ApplyTheme();
    m_cmd.ApplyTheme();
    m_list.ApplyTheme();
    Invalidate();
    RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_ERASE);
}

void CMainFrame::SaveSettings()
{
    m_settings.Save(AppSettings::DefaultPath());
}

void CMainFrame::OnViewDark()
{
    m_settings.display.darkTheme = !m_settings.display.darkTheme;
    m_settingsTab.ApplySettings(m_settings);
    ApplyTheme();
    SaveSettings();
}

void CMainFrame::OnUpdateViewDark(CCmdUI* pCmdUI)      { pCmdUI->SetCheck(m_settings.display.darkTheme ? 1 : 0); }
void CMainFrame::OnUpdateCloseReplay(CCmdUI* pCmdUI)   { pCmdUI->Enable(m_replayActive ? TRUE : FALSE); }
void CMainFrame::OnUpdateRecord(CCmdUI* pCmdUI)        { pCmdUI->SetCheck(m_recording ? 1 : 0); }
void CMainFrame::OnUpdateHold(CCmdUI* pCmdUI)          { pCmdUI->SetCheck(m_live ? 0 : 1); }

// ---------------------------------------------------------------------------
void CMainFrame::OnTimer(UINT_PTR id)
{
    switch (id) {
    case kTimerPostConnect:
        KillTimer(kTimerPostConnect);
        SendCmd(CMD_GET_STATUS);
        break;
    case kTimerStatus:
        UpdateStatusBar();
        break;
    case kTimerReplay:
        KillTimer(kTimerReplay);
        if (m_replayActive && m_replayPlaying) {
            if (m_replayIndex + 1 < m_replay.Count()) ReplayShow(m_replayIndex + 1, true);
            else ReplaySetPlaying(false);
        }
        break;
    default:
        CFrameWnd::OnTimer(id);
        break;
    }
}

void CMainFrame::UpdateStatusBar()
{
    if (!m_status.GetSafeHwnd()) return;
    CString s;
    if (m_replayActive)
        s.Format(_T("Replay %s  (%zu / %zu)"), m_replayName.GetString(), m_replayIndex + 1, m_replay.Count());
    else if (m_connected)
        s.Format(_T("Connected %s  %s%s"), m_portName.GetString(), m_running ? _T("RUNNING") : _T("idle"),
                 m_live ? _T("") : _T("  [HOLD]"));
    else
        s = _T("Disconnected");
    if (m_recording) s += _T("  \u25CF REC");
    m_status.SetPaneText(0, s);
    m_status.SetPaneText(1, m_mcuStatus.IsEmpty() ? CString(_T("MCU: -")) : m_mcuStatus);

    CString rate;
    rate.Format(_T("Fs %.3f MS/s  %zu smp  %.1f fps  %.2f MB/s  RTT %lu ms"),
                (m_obsFs > 0.0 ? m_obsFs : m_settings.sampleRateCalHz) / 1e6, m_obsSamples,
                m_link.framesPerSec, m_link.bytesPerSec / 1e6, m_lastRttMs);
    m_status.SetPaneText(2, rate);

    CString link;
    link.Format(_T("lost %llu  CRC %llu  hdr %llu"),
                static_cast<unsigned long long>(m_link.framesLost),
                static_cast<unsigned long long>(m_link.framesBadCrc),
                static_cast<unsigned long long>(m_link.framesBadHeader));
    m_status.SetPaneText(3, link);

    CString dspTxt; dspTxt.Format(_T("DSP %.1f ms"), m_lastDspMs);
    m_status.SetPaneText(4, dspTxt);

    CString buf;
    buf.Format(_T("buf %zu/%zu  %.0f MB"), m_store.Size(), m_store.Capacity(), m_store.Bytes() / 1e6);
    m_status.SetPaneText(5, buf);
}

// ---------------------------------------------------------------------------
ChirpFramePtr CMainFrame::CurrentFrame() const
{
    if (m_currentSeq != ChirpStore::kInvalidSeq) {
        ChirpFramePtr f = m_store.Get(m_currentSeq);
        if (f) return f;
    }
    return m_store.Latest();
}

CString CMainFrame::DefaultFileName(LPCTSTR prefix, LPCTSTR ext) const
{
    SYSTEMTIME st; ::GetLocalTime(&st);
    CString s;
    s.Format(_T("%s_%04d%02d%02d_%02d%02d%02d.%s"), prefix, st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, ext);
    return s;
}

LRESULT CMainFrame::OnCmdSaveFrame(WPARAM, LPARAM) { OnFileSaveFrame(); return 0; }

void CMainFrame::OnFileSaveFrame()
{
    ChirpFramePtr f = CurrentFrame();
    if (!f) { AfxMessageBox(_T("No frame to save.")); return; }
    CString defName; defName.Format(_T("chirp_%u.csv"), f->header.frame_id);
    CFileDialog dlg(FALSE, _T("csv"), defName, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
                    _T("CSV files (*.csv)|*.csv|All files (*.*)|*.*||"), this);
    if (!m_settings.lastDir.IsEmpty()) dlg.m_ofn.lpstrInitialDir = m_settings.lastDir;
    if (dlg.DoModal() != IDOK) return;
    std::string err;
    if (!core::WriteFrameCsv(ToPath(dlg.GetPathName()), *f, &err)) AfxMessageBox(_T("Save failed."));
    m_settings.lastDir = dlg.GetPathName().Left(dlg.GetPathName().ReverseFind(_T('\\')));
}

void CMainFrame::OnFileExportTargets()
{
    if (m_store.Size() == 0) { AfxMessageBox(_T("No frames in the buffer.")); return; }
    CFileDialog dlg(FALSE, _T("csv"), DefaultFileName(_T("targets"), _T("csv")), OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
                    _T("CSV files (*.csv)|*.csv|All files (*.*)|*.*||"), this);
    if (!m_settings.lastDir.IsEmpty()) dlg.m_ofn.lpstrInitialDir = m_settings.lastDir;
    if (dlg.DoModal() != IDOK) return;
    ExportTargetsCsv(dlg.GetPathName());
}

void CMainFrame::ExportTargetsCsv(const CString& path)
{
    // Re-run the pipeline over every buffered frame with the current settings.
    dsp::RadarDsp local;
    local.SetParams(m_settings.radar);
    local.SetSettings(m_settings.dsp);
    local.SetFallbackSampleRate(static_cast<double>(m_settings.sampleRateCalHz));
    CStdioFile f;
    if (!f.Open(path, CFile::modeCreate | CFile::modeWrite | CFile::typeText)) { AfxMessageBox(_T("Cannot create file.")); return; }
    f.WriteString(TargetListCtrl::CsvHeader());
    CWaitCursor wait;
    const uint64_t first = m_store.FirstSeq();
    const uint64_t next  = m_store.NextSeq();
    if (first != ChirpStore::kInvalidSeq) {
        for (uint64_t seq = first; seq < next; ++seq) {
            ChirpFramePtr fr = m_store.Get(seq);
            if (!fr) continue;
            dsp::FrameResult r = local.Process(*fr, true);
            for (const auto& t : r.targets) f.WriteString(TargetListCtrl::CsvRow(r.frameId, r.timestampMs, t));
        }
    }
    f.Close();
}

void CMainFrame::OnFileExportWav()
{
    ChirpFramePtr fr = CurrentFrame();
    if (!fr || fr->raw.empty()) { AfxMessageBox(_T("No raw samples in the current frame.")); return; }
    CString defName; defName.Format(_T("if_frame_%u.wav"), fr->header.frame_id);
    CFileDialog dlg(FALSE, _T("wav"), defName, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
                    _T("WAV files (*.wav)|*.wav|All files (*.*)|*.*||"), this);
    if (!m_settings.lastDir.IsEmpty()) dlg.m_ofn.lpstrInitialDir = m_settings.lastDir;
    if (dlg.DoModal() != IDOK) return;
    ExportWav(dlg.GetPathName());
}

void CMainFrame::ExportWav(const CString& path)
{
    ChirpFramePtr fr = CurrentFrame();
    if (!fr) return;
    const uint32_t fs = fr->header.sample_rate_hz > 0 ? fr->header.sample_rate_hz : m_settings.sampleRateCalHz;
    std::string err;
    if (!core::WriteWav16FromCodes(ToPath(path), fr->raw.data(), fr->raw.size(), fs, &err))
        AfxMessageBox(_T("WAV export failed."));
}

void CMainFrame::OnFileScreenshot()
{
    CFileDialog dlg(FALSE, _T("png"), DefaultFileName(_T("fastacq"), _T("png")), OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
                    _T("PNG images (*.png)|*.png||"), this);
    if (!m_settings.lastDir.IsEmpty()) dlg.m_ofn.lpstrInitialDir = m_settings.lastDir;
    if (dlg.DoModal() != IDOK) return;
    if (!SaveWindowPng(m_hWnd, dlg.GetPathName())) AfxMessageBox(_T("Screenshot failed."));
}

void CMainFrame::OnFileExit() { PostMessage(WM_CLOSE); }

// ---------------------------------------------------------------------------
LRESULT CMainFrame::OnCmdRecord(WPARAM, LPARAM) { OnFileRecord(); return 0; }

void CMainFrame::OnFileRecord()
{
    if (m_recording) StopRecording(); else StartRecording();
}

void CMainFrame::StartRecording()
{
    if (m_replayActive) { AfxMessageBox(_T("Close the replay before recording.")); return; }
    CFileDialog dlg(FALSE, _T("facq"), DefaultFileName(_T("session"), _T("facq")), OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
                    _T("FastAcq sessions (*.facq)|*.facq|All files (*.*)|*.*||"), this);
    if (!m_settings.lastDir.IsEmpty()) dlg.m_ofn.lpstrInitialDir = m_settings.lastDir;
    if (dlg.DoModal() != IDOK) return;
    const CString path = dlg.GetPathName();
    const uint32_t fs = m_obsFs > 0.0 ? static_cast<uint32_t>(m_obsFs) : m_settings.sampleRateCalHz;
    if (!m_recorder.Open(ToPath(path), fs, UnixNowMs(), "FastAcq session")) {
        AfxMessageBox(_T("Cannot create the session file."));
        return;
    }
    m_recording  = true;
    m_recordPath = path.Mid(path.ReverseFind(_T('\\')) + 1);
    m_settings.lastDir = path.Left(path.ReverseFind(_T('\\')));
    m_cmd.SetRecording(true, _T("REC ") + m_recordPath);
    m_logTab.AppendLine(LOG_INFO, _T("Recording started: ") + path);
    UpdateStatusBar();
}

void CMainFrame::StopRecording()
{
    if (!m_recording) return;
    const uint64_t n = m_recorder.FramesWritten();
    m_recorder.Close();
    m_recording = false;
    m_cmd.SetRecording(false, _T(""));
    CString msg; msg.Format(_T("Recording stopped: %llu frames"), static_cast<unsigned long long>(n));
    m_logTab.AppendLine(LOG_INFO, msg);
    UpdateStatusBar();
}

LRESULT CMainFrame::OnCmdOpenReplay(WPARAM, LPARAM) { OnFileOpenReplay(); return 0; }

void CMainFrame::OnFileOpenReplay()
{
    CFileDialog dlg(TRUE, _T("facq"), nullptr, OFN_HIDEREADONLY | OFN_FILEMUSTEXIST,
                    _T("FastAcq sessions (*.facq)|*.facq|All files (*.*)|*.*||"), this);
    if (!m_settings.lastDir.IsEmpty()) dlg.m_ofn.lpstrInitialDir = m_settings.lastDir;
    if (dlg.DoModal() != IDOK) return;
    if (!OpenReplay(dlg.GetPathName())) AfxMessageBox(_T("Cannot open the session file."));
}

void CMainFrame::OnFileCloseReplay() { CloseReplay(); }

bool CMainFrame::OpenReplay(const CString& path)
{
    if (m_recording) StopRecording();
    if (Connected()) m_serial->Close();
    CloseReplay();
    if (!m_replay.Open(ToPath(path))) return false;
    if (m_replay.Count() == 0) { m_replay.Close(); return false; }

    OnCmdClear(0, 0);
    m_replayActive  = true;
    m_replayPlaying = false;
    m_replayIndex   = 0;
    m_replayName    = path.Mid(path.ReverseFind(_T('\\')) + 1);
    m_settings.lastDir = path.Left(path.ReverseFind(_T('\\')));
    m_cmd.SetReplay(true, m_replayName, static_cast<int>(m_replay.Count()));
    RelayoutClient();
    CString msg; msg.Format(_T("Replay opened: %s (%zu frames, Fs %u Hz)"), path.GetString(), m_replay.Count(), m_replay.Header().sampleRateHz);
    m_logTab.AppendLine(LOG_INFO, msg);
    SetLive(true);
    ReplayShow(0, false);
    return true;
}

void CMainFrame::CloseReplay()
{
    if (!m_replayActive) return;
    KillTimer(kTimerReplay);
    m_replay.Close();
    m_replayActive = false;
    m_replayPlaying = false;
    m_cmd.SetReplay(false, _T(""), 0);
    RelayoutClient();
    UpdateStatusBar();
}

void CMainFrame::ReplayShow(size_t index, bool fromTimer)
{
    if (!m_replayActive || index >= m_replay.Count()) return;
    ChirpFrame f;
    if (!m_replay.Read(index, f)) { ReplaySetPlaying(false); return; }
    if (f.header.sample_rate_hz == 0) f.header.sample_rate_hz = m_replay.Header().sampleRateHz;
    m_replayIndex = index;
    // A replay step is an explicit request for this frame: leave HOLD, otherwise
    // OnResultReady discards the (live) result and the plots never update.
    if (!m_live) { m_live = true; m_cmd.SetHold(false); }
    const uint64_t seq = m_store.Push(std::move(f));
    OnFrameReady(static_cast<WPARAM>(seq), 0);
    CString info;
    info.Format(_T("%zu / %zu   t = %u ms"), index + 1, m_replay.Count(), m_replay.TimestampMs(index));
    m_cmd.SetReplayPos(static_cast<int>(index), m_replayPlaying, info);
    UpdateStatusBar();
    if (m_replayPlaying && fromTimer) ReplayArmTimer();
    else if (m_replayPlaying) ReplayArmTimer();
}

void CMainFrame::ReplayArmTimer()
{
    if (!m_replayActive || !m_replayPlaying) return;
    UINT ms = 1;
    if (m_replaySpeedPct > 0 && m_replayIndex + 1 < m_replay.Count()) {
        const uint32_t t0 = m_replay.TimestampMs(m_replayIndex);
        const uint32_t t1 = m_replay.TimestampMs(m_replayIndex + 1);
        double dt = (t1 > t0) ? static_cast<double>(t1 - t0) : 100.0;
        dt = dt * 100.0 / m_replaySpeedPct;
        if (dt < 1.0) dt = 1.0;
        if (dt > 5000.0) dt = 5000.0;
        ms = static_cast<UINT>(dt);
    }
    SetTimer(kTimerReplay, ms, nullptr);
}

void CMainFrame::ReplaySetPlaying(bool playing)
{
    m_replayPlaying = playing;
    if (playing) { SetLive(true); ReplayArmTimer(); }
    else KillTimer(kTimerReplay);
    CString info;
    info.Format(_T("%zu / %zu   t = %u ms"), m_replayIndex + 1, m_replay.Count(), m_replay.TimestampMs(m_replayIndex));
    m_cmd.SetReplayPos(static_cast<int>(m_replayIndex), m_replayPlaying, info);
}

LRESULT CMainFrame::OnReplayCtrl(WPARAM wp, LPARAM lp)
{
    if (!m_replayActive) return 0;
    const size_t n = m_replay.Count();
    switch (wp) {
    case REPLAY_PLAY:
        if (m_replayIndex + 1 >= n) ReplayShow(0, false);
        ReplaySetPlaying(true);
        break;
    case REPLAY_PAUSE: ReplaySetPlaying(false); break;
    case REPLAY_FIRST: ReplaySetPlaying(false); ReplayShow(0, false); break;
    case REPLAY_LAST:  ReplaySetPlaying(false); ReplayShow(n ? n - 1 : 0, false); break;
    case REPLAY_PREV:  ReplaySetPlaying(false); if (m_replayIndex > 0) ReplayShow(m_replayIndex - 1, false); break;
    case REPLAY_NEXT:  ReplaySetPlaying(false); if (m_replayIndex + 1 < n) ReplayShow(m_replayIndex + 1, false); break;
    case REPLAY_SEEK: {
        const bool wasPlaying = m_replayPlaying;
        if (wasPlaying) KillTimer(kTimerReplay);
        size_t idx = static_cast<size_t>(lp < 0 ? 0 : lp);
        if (idx >= n) idx = n ? n - 1 : 0;
        if (idx != m_replayIndex) ReplayShow(idx, false);
        if (wasPlaying) ReplayArmTimer();
        break;
    }
    case REPLAY_SPEED: m_replaySpeedPct = static_cast<int>(lp); if (m_replayPlaying) ReplayArmTimer(); break;
    case REPLAY_CLOSE: CloseReplay(); break;
    default: break;
    }
    return 0;
}

// ---------------------------------------------------------------------------
void CMainFrame::OnAcqConnect()  { if (Connected()) OnCmdDisconnect(0, 0); else OnCmdConnect(0, 0); }
void CMainFrame::OnAcqStart()    { OnCmdStart(0, 0); }
void CMainFrame::OnAcqStop()     { OnCmdStop(0, 0); }
void CMainFrame::OnAcqTrigger()  { OnCmdTrigger(0, 0); }
void CMainFrame::OnAcqAbort()    { OnCmdAbort(0, 0); }
void CMainFrame::OnAcqHold()     { SetLive(!m_live); }
void CMainFrame::OnAcqResetAvg() { OnResetAverages(0, 0); }
void CMainFrame::OnAcqPing()     { OnCmdPing(0, 0); }
void CMainFrame::OnAcqStatus()   { OnCmdGetStatus(0, 0); }

void CMainFrame::OnHelpAbout()
{
    AfxMessageBox(_T("FastAcq 2.0\n\nFMCW radar IF-signal analyzer for the STM32H7 Fast Acquisition Device.\n")
                  _T("C++17 / MFC / GDI, no third-party dependencies.\n\nMIT License."), MB_ICONINFORMATION);
}

void CMainFrame::OnHelpKeys()
{
    AfxMessageBox(
        _T("Space\tStart / Stop acquisition\n")
        _T("T\tTrigger a single capture\n")
        _T("H\tHold / Live display\n")
        _T("R\tStart / stop recording\n")
        _T("F5\tConnect / disconnect\n")
        _T("Ctrl+O\tOpen replay\n")
        _T("Ctrl+S\tSave frame as CSV\n")
        _T("F12\tSave screenshot (PNG)\n")
        _T("Ctrl+1..4\tSwitch tab\n")
        _T("PgUp / PgDn, Ctrl+Left / Right\tPrevious / next frame\n\n")
        _T("In plots: wheel = zoom at cursor, Shift+wheel = pan, drag = pan, double-click = reset,\n")
        _T("click = marker A, Shift+click = marker B, Esc = clear markers, Home = reset zoom, A = autoscale dB."),
        MB_ICONINFORMATION);
}

CFrameWnd* CreateMainFrame()
{
    auto* pFrame = new CMainFrame();
    CRect rc(100, 100, 1500, 950);
    if (!pFrame->Create(nullptr, _T("FastAcq"), WS_OVERLAPPEDWINDOW, rc, nullptr, MAKEINTRESOURCE(IDR_MAINMENU)))
    {
        delete pFrame;
        return nullptr;
    }
    return pFrame;
}
