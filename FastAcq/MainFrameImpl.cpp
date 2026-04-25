#include "pch.h"
#include "MainFrame.h"
#include "AppMessages.h"
#include "resource.h"

static UINT s_statusIndicators[] = { ID_SEPARATOR };

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_DESTROY()
    ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_CTRL, &CMainFrame::OnTabSelChange)
    ON_MESSAGE(WM_APP_FRAME_READY,     &CMainFrame::OnFrameReady)
    ON_MESSAGE(WM_APP_PORT_STATUS,     &CMainFrame::OnPortStatus)
    ON_MESSAGE(WM_APP_PARSE_STATS,     &CMainFrame::OnParseStats)
    ON_MESSAGE(WM_APP_PONG_RX,         &CMainFrame::OnPongRx)
    ON_MESSAGE(WM_APP_FRAME_SELECTED,  &CMainFrame::OnFrameSelected)
    ON_MESSAGE(WM_APP_CMD_CONNECT,     &CMainFrame::OnCmdConnect)
    ON_MESSAGE(WM_APP_CMD_DISCONNECT,  &CMainFrame::OnCmdDisconnect)
    ON_MESSAGE(WM_APP_CMD_START,       &CMainFrame::OnCmdStart)
    ON_MESSAGE(WM_APP_CMD_STOP,        &CMainFrame::OnCmdStop)
    ON_MESSAGE(WM_APP_CMD_SET_FREQ,    &CMainFrame::OnCmdSetFreq)
    ON_MESSAGE(WM_APP_CMD_SET_SAMPLES, &CMainFrame::OnCmdSetSamples)
    ON_MESSAGE(WM_APP_CMD_PING,        &CMainFrame::OnCmdPing)
    ON_MESSAGE(WM_APP_CMD_SAVE_FRAME,  &CMainFrame::OnCmdSaveFrame)
    ON_MESSAGE(WM_APP_CMD_CLEAR,       &CMainFrame::OnCmdClear)
    ON_MESSAGE(WM_APP_COMM_LOG,        &CMainFrame::OnCommLog)
    ON_MESSAGE(WM_APP_CMD_SET_MODE,      &CMainFrame::OnCmdSetMode)
    ON_MESSAGE(WM_APP_CMD_SET_DATA_MASK, &CMainFrame::OnCmdSetDataMask)
    ON_MESSAGE(WM_APP_CMD_SET_INTERVAL,  &CMainFrame::OnCmdSetInterval)
    ON_MESSAGE(WM_APP_CMD_TRIGGER,       &CMainFrame::OnCmdTrigger)
    ON_MESSAGE(WM_APP_CMD_GET_STATUS,    &CMainFrame::OnCmdGetStatus)
    ON_MESSAGE(WM_APP_ACQ_MODE,          &CMainFrame::OnAcqMode)
    ON_MESSAGE(WM_APP_FFT_SETTINGS,      &CMainFrame::OnFftSettings)
END_MESSAGE_MAP()

CMainFrame::CMainFrame() = default;
CMainFrame::~CMainFrame() = default;

int CMainFrame::OnCreate(LPCREATESTRUCT lpcs)
{
    if (CFrameWnd::OnCreate(lpcs) == -1)
        return -1;

    if (!m_status.Create(this) ||
        !m_status.SetIndicators(s_statusIndicators,
                                sizeof(s_statusIndicators) / sizeof(UINT)))
    {
        TRACE0("Failed to create status bar\n");
    }
    m_status.SetPaneInfo(0, ID_SEPARATOR, SBPS_STRETCH, 0);

    m_list.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT | LVS_SINGLESEL,
                  CRect(0, 0, 220, 400), this, IDC_CHIRP_LIST);
    m_list.InitColumns();

    m_tab.Create(WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | TCS_TABS,
                 CRect(0, 0, 10, 10), this, IDC_TAB_CTRL);

    // Custom font for tab labels (Segoe UI 10pt bold).
    m_tabFont.CreateFont(-13, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                         DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                         CLEARTYPE_QUALITY, VARIABLE_PITCH | FF_SWISS, _T("Segoe UI"));
    m_tab.SetFont(&m_tabFont);

    TCITEM ti{}; ti.mask = TCIF_TEXT;
    ti.pszText = const_cast<LPTSTR>(_T("Waveform"));        m_tab.InsertItem(0, &ti);
    ti.pszText = const_cast<LPTSTR>(_T("FFT / Waterfall")); m_tab.InsertItem(1, &ti);
    ti.pszText = const_cast<LPTSTR>(_T("Communication"));   m_tab.InsertItem(2, &ti);

    m_tab1.CreateTab(&m_tab, IDC_TAB1_WND);
    m_tab2.CreateTab(&m_tab, IDC_TAB2_WND);
    m_tab3.CreateTab(&m_tab, IDC_TAB3_WND);
    m_tab1.ShowWindow(SW_SHOW);
    m_tab2.ShowWindow(SW_HIDE);
    m_tab3.ShowWindow(SW_HIDE);

    m_cmd.CreatePanel(this, IDC_CMD_PANEL);
    m_cmd.PopulateComPorts(SerialWorker::EnumPorts());

    m_serial = std::make_unique<SerialWorker>(m_store, GetSafeHwnd());

    UpdateStatusBar();
    return 0;
}

void CMainFrame::OnDestroy()
{
    if (m_serial) m_serial->Close();
    CFrameWnd::OnDestroy();
}

void CMainFrame::OnSize(UINT t, int cx, int cy)
{
    CFrameWnd::OnSize(t, cx, cy);
    RelayoutClient();
}

void CMainFrame::RelayoutClient()
{
    RepositionBars(AFX_IDW_CONTROLBAR_FIRST, AFX_IDW_CONTROLBAR_LAST, 0);
    CRect client; GetClientRect(&client);
    RepositionBars(AFX_IDW_CONTROLBAR_FIRST, AFX_IDW_CONTROLBAR_LAST,
                   0, reposQuery, &client);

    const int cmdH  = 58;   // 2 visible rows * (24+4)px + 2px margin
    const int listW = 240;
    const int pad   = 2;

    int areaTop    = client.top;
    int areaBottom = client.bottom - cmdH;
    if (areaBottom < areaTop + 10) areaBottom = areaTop + 10;

    if (m_list.GetSafeHwnd())
        m_list.MoveWindow(client.left, areaTop, listW, areaBottom - areaTop);

    int tabX = client.left + listW + pad;
    int tabW = client.right - tabX;
    if (tabW < 10) tabW = 10;
    if (m_tab.GetSafeHwnd())
        m_tab.MoveWindow(tabX, areaTop, tabW, areaBottom - areaTop);

    if (m_tab.GetSafeHwnd()) {
        CRect trc; m_tab.GetClientRect(&trc);
        m_tab.AdjustRect(FALSE, &trc);
        if (m_tab1.GetSafeHwnd()) m_tab1.MoveWindow(trc);
        if (m_tab2.GetSafeHwnd()) m_tab2.MoveWindow(trc);
        if (m_tab3.GetSafeHwnd()) m_tab3.MoveWindow(trc);
    }

    if (m_cmd.GetSafeHwnd())
        m_cmd.MoveWindow(client.left, areaBottom, client.Width(), cmdH);
}

void CMainFrame::OnTabSelChange(NMHDR*, LRESULT* pResult)
{
    int sel = m_tab.GetCurSel();
    m_tab1.ShowWindow(sel == 0 ? SW_SHOW : SW_HIDE);
    m_tab2.ShowWindow(sel == 1 ? SW_SHOW : SW_HIDE);
    m_tab3.ShowWindow(sel == 2 ? SW_SHOW : SW_HIDE);
    *pResult = 0;
}

LRESULT CMainFrame::OnFrameReady(WPARAM wp, LPARAM)
{
    size_t idx = static_cast<size_t>(wp);
    ChirpFrame f;
    if (!m_store.GetAt(idx, f)) return 0;

    m_framesShown++;
    m_lastTsMs   = f.header.timestamp_ms;
    m_lastPeakHz = f.header.fft_freq_res_hz * static_cast<float>(f.header.fft_peak_bin);

    m_list.AddChirp(idx, f.header.frame_id, f.header.timestamp_ms, m_lastPeakHz);

    if (m_pingPending) {
        DWORD now = ::GetTickCount();
        m_lastRttMs   = now - m_pingSentTick;
        m_pingPending = false;
        PostMessage(WM_APP_PONG_RX, 0, static_cast<LPARAM>(m_lastRttMs));
    }

    ShowFrameAt(idx);
    UpdateStatusBar();
    return 0;
}

LRESULT CMainFrame::OnPortStatus(WPARAM wp, LPARAM)
{
    m_connected = (wp != 0);
    m_cmd.SetConnected(m_connected);
    UpdateStatusBar();
    return 0;
}

LRESULT CMainFrame::OnParseStats(WPARAM, LPARAM) { UpdateStatusBar(); return 0; }

LRESULT CMainFrame::OnPongRx(WPARAM, LPARAM lp)
{
    m_lastRttMs = static_cast<DWORD>(lp);
    UpdateStatusBar();
    return 0;
}

LRESULT CMainFrame::OnFrameSelected(WPARAM wp, LPARAM)
{
    ShowFrameAt(static_cast<size_t>(wp));
    return 0;
}

void CMainFrame::ShowFrameAt(size_t logicalIdx)
{
    ChirpFrame f;
    if (!m_store.GetAt(logicalIdx, f)) return;
    m_currentIdx = logicalIdx;

    // Update sample rate from frame header when available.
    if (f.header.sample_rate_hz > 0)
        m_sampleRateHz = f.header.sample_rate_hz;

    m_tab1.ShowFrame(f, m_rawMode, m_fftSettings, m_sampleRateHz);
    m_tab2.ShowFrame(f, m_rawMode, m_fftSettings, m_sampleRateHz);
}

LRESULT CMainFrame::OnCmdConnect(WPARAM, LPARAM)
{
    if (!m_serial) return 0;
    CString port = m_cmd.GetSelectedPort();
    if (port.IsEmpty()) { AfxMessageBox(_T("Select COM port first.")); return 0; }
    if (!m_serial->Open(port)) {
        CString err; err.Format(_T("Failed to open %s"), port.GetString());
        AfxMessageBox(err);
        return 0;
    }
    m_portName = port;
    return 0;
}

LRESULT CMainFrame::OnCmdDisconnect(WPARAM, LPARAM)
{
    if (m_serial) m_serial->Close();
    return 0;
}

LRESULT CMainFrame::OnCmdStart(WPARAM, LPARAM)
{
    if (!m_serial || !m_serial->IsOpen()) return 0;
    m_serial->SendCommand(CMD_START_CHIRP, m_cmd.GetFreqHz(), 0, 0);
    return 0;
}

LRESULT CMainFrame::OnCmdStop(WPARAM, LPARAM)
{
    if (!m_serial || !m_serial->IsOpen()) return 0;
    m_serial->SendCommand(CMD_SET_MODE, MODE_IDLE, 0, 0);
    return 0;
}

LRESULT CMainFrame::OnCmdSetFreq(WPARAM wp, LPARAM)
{
    if (!m_serial || !m_serial->IsOpen()) return 0;
    m_serial->SendCommand(CMD_START_CHIRP, static_cast<uint16_t>(wp), 0, 0);
    return 0;
}

LRESULT CMainFrame::OnCmdSetSamples(WPARAM wp, LPARAM)
{
    if (!m_serial || !m_serial->IsOpen()) return 0;
    uint32_t v = static_cast<uint32_t>(wp);
    m_serial->SendCommand(CMD_SET_SAMPLES, static_cast<uint16_t>(v & 0xFFFF), 0, 0);
    return 0;
}

LRESULT CMainFrame::OnCmdPing(WPARAM, LPARAM)
{
    if (!m_serial || !m_serial->IsOpen()) return 0;
    m_pingSentTick = ::GetTickCount();
    m_pingPending  = true;
    m_serial->SendCommand(CMD_PING);
    return 0;
}

LRESULT CMainFrame::OnCmdSaveFrame(WPARAM, LPARAM)
{
    if (m_currentIdx == static_cast<size_t>(-1)) {
        AfxMessageBox(_T("Select a frame first."));
        return 0;
    }
    ChirpFrame f;
    if (!m_store.GetAt(m_currentIdx, f)) return 0;

    CString defName;
    defName.Format(_T("chirp_%u.csv"), f.header.frame_id);
    CFileDialog dlg(FALSE, _T("csv"), defName,
                    OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
                    _T("CSV files (*.csv)|*.csv|All files (*.*)|*.*||"));
    if (dlg.DoModal() != IDOK) return 0;
    if (!SaveFrameCsv(f, dlg.GetPathName()))
        AfxMessageBox(_T("Save failed."));
    return 0;
}

LRESULT CMainFrame::OnCmdClear(WPARAM, LPARAM)
{
    m_store.Clear();
    m_list.ClearAll();
    m_tab2.ClearHistory();
    m_tab3.Clear();
    m_currentIdx  = static_cast<size_t>(-1);
    m_framesShown = 0;
    UpdateStatusBar();
    return 0;
}

LRESULT CMainFrame::OnCmdSetMode(WPARAM wp, LPARAM)
{
    if (!m_serial || !m_serial->IsOpen()) return 0;
    m_serial->SendCommand(CMD_SET_MODE, static_cast<uint16_t>(wp), 0, 0);
    return 0;
}

LRESULT CMainFrame::OnCmdSetDataMask(WPARAM wp, LPARAM)
{
    if (!m_serial || !m_serial->IsOpen()) return 0;
    m_serial->SendCommand(CMD_SET_DATA_MASK, static_cast<uint16_t>(wp & 0x03), 0, 0);
    return 0;
}

LRESULT CMainFrame::OnCmdSetInterval(WPARAM wp, LPARAM)
{
    if (!m_serial || !m_serial->IsOpen()) return 0;
    m_serial->SendCommand(CMD_SET_INTERVAL, static_cast<uint16_t>(wp), 0, 0);
    return 0;
}

LRESULT CMainFrame::OnCmdTrigger(WPARAM, LPARAM)
{
    if (!m_serial || !m_serial->IsOpen()) return 0;
    m_serial->SendCommand(CMD_TRIGGER, 0, 0, 0);
    return 0;
}

LRESULT CMainFrame::OnCmdGetStatus(WPARAM, LPARAM)
{
    if (!m_serial || !m_serial->IsOpen()) return 0;
    m_serial->SendCommand(CMD_GET_STATUS, 0, 0, 0);
    return 0;
}

LRESULT CMainFrame::OnCommLog(WPARAM, LPARAM lp)
{
    CString* pLine = reinterpret_cast<CString*>(lp);
    if (pLine) { m_tab3.AppendLine(*pLine); delete pLine; }
    return 0;
}

LRESULT CMainFrame::OnAcqMode(WPARAM wp, LPARAM)
{
    m_rawMode = (wp == 0);   // 0 = RAW (local FFT), 1 = FFT (from MCU)
    m_tab1.SetAcqMode(m_rawMode);
    m_tab2.SetAcqMode(m_rawMode);
    return 0;
}

LRESULT CMainFrame::OnFftSettings(WPARAM, LPARAM lp)
{
    FftSettings* p = reinterpret_cast<FftSettings*>(lp);
    if (p) { m_fftSettings = *p; delete p; }
    m_tab2.SetFftSettings(m_fftSettings);
    return 0;
}

void CMainFrame::UpdateStatusBar()
{
    if (!m_status.GetSafeHwnd()) return;
    CString s;
    s.Format(_T("%s  %s  |  frames: %llu  |  peak: %.0f Hz  |  t=%u ms  |  RTT: %u ms  |  badCRC: %llu"),
             m_connected ? _T("Connected") : _T("Disconnected"),
             m_portName.IsEmpty() ? _T("-") : m_portName.GetString(),
             static_cast<unsigned long long>(m_framesShown),
             m_lastPeakHz, m_lastTsMs, m_lastRttMs,
             static_cast<unsigned long long>(m_serial ? m_serial->FramesBadCrc() : 0));
    m_status.SetPaneText(0, s);
}

bool CMainFrame::SaveFrameCsv(const ChirpFrame& f, const CString& path)
{
    CStdioFile fp;
    CFileException ex;
    if (!fp.Open(path, CFile::modeCreate | CFile::modeWrite | CFile::typeText, &ex))
        return false;

    CString line;
    line.Format(_T("# frame_id,%u\n"),          f.header.frame_id);          fp.WriteString(line);
    line.Format(_T("# timestamp_ms,%u\n"),      f.header.timestamp_ms);      fp.WriteString(line);
    line.Format(_T("# sample_rate_hz,%u\n"),    f.header.sample_rate_hz);    fp.WriteString(line);
    line.Format(_T("# chirp_freq_hz,%u\n"),     f.header.chirp_freq_hz);     fp.WriteString(line);
    line.Format(_T("# fft_size,%u\n"),          f.header.fft_size);          fp.WriteString(line);
    line.Format(_T("# fft_peak_bin,%u\n"),      f.header.fft_peak_bin);      fp.WriteString(line);
    line.Format(_T("# fft_freq_res_hz,%.6f\n"), f.header.fft_freq_res_hz);   fp.WriteString(line);
    fp.WriteString(_T("index,raw,fft_mag\n"));

    size_t n = (std::max)(f.raw.size(), f.fft.size());
    for (size_t i = 0; i < n; ++i) {
        line.Format(_T("%zu,"), i);
        if (i < f.raw.size()) { CString v; v.Format(_T("%u"),   f.raw[i]); line += v; }
        line += _T(",");
        if (i < f.fft.size()) { CString v; v.Format(_T("%.6f"), f.fft[i]); line += v; }
        line += _T("\n");
        fp.WriteString(line);
    }
    fp.Close();
    return true;
}

CFrameWnd* CreateMainFrame()
{
    auto* pFrame = new CMainFrame();
    if (!pFrame->Create(nullptr, _T("FastAcq"),
                        WS_OVERLAPPEDWINDOW,
                        CRect(100, 100, 1380, 900)))
    {
        delete pFrame;
        return nullptr;
    }
    return pFrame;
}
