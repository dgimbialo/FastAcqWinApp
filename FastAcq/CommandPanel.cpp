#include "pch.h"
#include "CommandPanel.h"
#include "AppMessages.h"
#include "Dpi.h"
#include "Theme.h"
#include "resource.h"

BEGIN_MESSAGE_MAP(CommandPanel, CWnd)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_ERASEBKGND()
    ON_WM_CTLCOLOR()
    ON_WM_HSCROLL()
    ON_BN_CLICKED(IDC_BTN_CONNECT,     &CommandPanel::OnConnect)
    ON_BN_CLICKED(IDC_BTN_START,       &CommandPanel::OnStart)
    ON_BN_CLICKED(IDC_BTN_STOP,        &CommandPanel::OnStop)
    ON_BN_CLICKED(IDC_BTN_TRIGGER,     &CommandPanel::OnTrigger)
    ON_BN_CLICKED(IDC_BTN_ABORT,       &CommandPanel::OnAbort)
    ON_BN_CLICKED(IDC_BTN_HOLD,        &CommandPanel::OnHold)
    ON_BN_CLICKED(IDC_BTN_RECORD,      &CommandPanel::OnRecord)
    ON_BN_CLICKED(IDC_BTN_OPEN_REPLAY, &CommandPanel::OnOpenReplay)
    ON_BN_CLICKED(IDC_BTN_SAVE_FRAME,  &CommandPanel::OnSaveFrame)
    ON_BN_CLICKED(IDC_BTN_CLEAR,       &CommandPanel::OnClear)
    ON_CBN_DROPDOWN(IDC_CMB_COM,       &CommandPanel::OnComDropDown)
    ON_BN_CLICKED(IDC_BTN_RP_PLAY,     &CommandPanel::OnRpPlay)
    ON_BN_CLICKED(IDC_BTN_RP_FIRST,    &CommandPanel::OnRpFirst)
    ON_BN_CLICKED(IDC_BTN_RP_PREV,     &CommandPanel::OnRpPrev)
    ON_BN_CLICKED(IDC_BTN_RP_NEXT,     &CommandPanel::OnRpNext)
    ON_BN_CLICKED(IDC_BTN_RP_LAST,     &CommandPanel::OnRpLast)
    ON_BN_CLICKED(IDC_BTN_RP_CLOSE,    &CommandPanel::OnRpClose)
    ON_CBN_SELCHANGE(IDC_CMB_RP_SPEED, &CommandPanel::OnRpSpeed)
END_MESSAGE_MAP()

BOOL CommandPanel::CreatePanel(CWnd* parent, UINT id)
{
    LPCTSTR cls = AfxRegisterWndClass(0, ::LoadCursor(nullptr, IDC_ARROW), nullptr, nullptr);
    return Create(cls, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN, CRect(0, 0, 10, 10), parent, id);
}

int CommandPanel::DesiredHeight() const
{
    const int rowH = Dpi::Scale(m_hWnd, 32);
    return m_replay ? 2 * rowH : rowH;
}

int CommandPanel::OnCreate(LPCREATESTRUCT lpcs)
{
    if (CWnd::OnCreate(lpcs) == -1) return -1;
    m_bgBrush.CreateSolidBrush(Theme::Get().panel);
    Dpi::MakeFont(m_font, m_hWnd, 9);

    const DWORD bs = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON;
    const DWORD cs = WS_CHILD | WS_VISIBLE | CBS_DROPDOWN | WS_VSCROLL;
    CRect rc(0, 0, 100, 22);

    m_cmbCom.Create(cs, rc, this, IDC_CMB_COM);
    m_btnConnect.Create(_T("Connect"),       bs, rc, this, IDC_BTN_CONNECT);
    m_btnStart.Create(_T("Start"),           bs, rc, this, IDC_BTN_START);
    m_btnStop.Create(_T("Stop"),             bs, rc, this, IDC_BTN_STOP);
    m_btnTrigger.Create(_T("Trigger"),       bs, rc, this, IDC_BTN_TRIGGER);
    m_btnAbort.Create(_T("Abort"),           bs, rc, this, IDC_BTN_ABORT);
    m_btnHold.Create(_T("Hold"),             bs, rc, this, IDC_BTN_HOLD);
    m_btnRecord.Create(_T("\u25CF Record"),  bs, rc, this, IDC_BTN_RECORD);
    m_btnOpen.Create(_T("Open..."),          bs, rc, this, IDC_BTN_OPEN_REPLAY);
    m_btnSaveFrame.Create(_T("Save frame"),  bs, rc, this, IDC_BTN_SAVE_FRAME);
    m_btnClear.Create(_T("Clear"),           bs, rc, this, IDC_BTN_CLEAR);
    m_lblRec.Create(_T(""), WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE | SS_ENDELLIPSIS, rc, this);

    const DWORD ss = WS_CHILD | SS_LEFT | SS_CENTERIMAGE | SS_ENDELLIPSIS;
    m_lblRpName.Create(_T(""), ss, rc, this, IDC_LBL_RP_INFO);
    m_btnRpFirst.Create(_T("|<"),   WS_CHILD | BS_PUSHBUTTON, rc, this, IDC_BTN_RP_FIRST);
    m_btnRpPrev.Create(_T("<"),     WS_CHILD | BS_PUSHBUTTON, rc, this, IDC_BTN_RP_PREV);
    m_btnRpPlay.Create(_T("Play"),  WS_CHILD | BS_PUSHBUTTON, rc, this, IDC_BTN_RP_PLAY);
    m_btnRpNext.Create(_T(">"),     WS_CHILD | BS_PUSHBUTTON, rc, this, IDC_BTN_RP_NEXT);
    m_btnRpLast.Create(_T(">|"),    WS_CHILD | BS_PUSHBUTTON, rc, this, IDC_BTN_RP_LAST);
    m_sldRp.Create(WS_CHILD | TBS_HORZ | TBS_NOTICKS, rc, this, IDC_SLD_RP_POS);
    m_cmbRpSpeed.Create(WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL, rc, this, IDC_CMB_RP_SPEED);
    for (LPCTSTR s : { _T("x0.25"), _T("x0.5"), _T("x1"), _T("x2"), _T("x4"), _T("x8"), _T("max") }) m_cmbRpSpeed.AddString(s);
    m_cmbRpSpeed.SetCurSel(2);
    m_lblRpInfo.Create(_T(""), ss, rc, this);
    m_btnRpClose.Create(_T("Close"), WS_CHILD | BS_PUSHBUTTON, rc, this, IDC_BTN_RP_CLOSE);

    CWnd* pw = GetWindow(GW_CHILD);
    while (pw) { pw->SetFont(&m_font); pw = pw->GetWindow(GW_HWNDNEXT); }

    SetConnected(false);
    SetHold(false);
    return 0;
}

void CommandPanel::ApplyTheme()
{
    if (m_bgBrush.GetSafeHandle()) m_bgBrush.DeleteObject();
    m_bgBrush.CreateSolidBrush(Theme::Get().panel);
    Invalidate();
}

void CommandPanel::OnSize(UINT, int, int) { Relayout(); }

void CommandPanel::Relayout()
{
    if (!m_cmbCom.GetSafeHwnd()) return;
    const int S = static_cast<int>(Dpi::Of(m_hWnd));
    auto sc = [&](int px) { return ::MulDiv(px, S, 96); };
    CRect rc; GetClientRect(&rc);
    const int pad = sc(4), h = sc(24), rowH = sc(32);
    int x = pad, y = (rowH - h) / 2;
    auto place = [&](CWnd& w, int wpx, int hh = 0) {
        if (w.GetSafeHwnd()) w.MoveWindow(x, y, sc(wpx), hh ? hh : h);
        x += sc(wpx) + pad;
    };
    place(m_cmbCom, 120, sc(200));
    place(m_btnConnect, 90);
    place(m_btnStart, 66);
    place(m_btnStop, 60);
    place(m_btnTrigger, 66);
    place(m_btnAbort, 60);
    x += sc(8);
    place(m_btnHold, 62);
    place(m_btnRecord, 80);
    place(m_btnOpen, 66);
    x += sc(8);
    place(m_btnSaveFrame, 84);
    place(m_btnClear, 60);
    x += sc(8);
    if (m_lblRec.GetSafeHwnd()) m_lblRec.MoveWindow(x, y, (std::max)(10, rc.Width() - x - pad), h);

    // Replay row.
    const int show = m_replay ? SW_SHOW : SW_HIDE;
    x = pad; y = rowH + (rowH - h) / 2;
    place(m_lblRpName, 160);
    place(m_btnRpFirst, 34);
    place(m_btnRpPrev, 34);
    place(m_btnRpPlay, 60);
    place(m_btnRpNext, 34);
    place(m_btnRpLast, 34);
    const int rightW = sc(70) + pad + sc(170) + pad + sc(60) + pad;
    int sldW = rc.Width() - x - rightW - pad;
    if (sldW < sc(60)) sldW = sc(60);
    if (m_sldRp.GetSafeHwnd()) m_sldRp.MoveWindow(x, y, sldW, h);
    x += sldW + pad;
    place(m_cmbRpSpeed, 70, sc(200));
    place(m_lblRpInfo, 170);
    place(m_btnRpClose, 60);
    for (CWnd* w : { static_cast<CWnd*>(&m_lblRpName), static_cast<CWnd*>(&m_btnRpFirst), static_cast<CWnd*>(&m_btnRpPrev),
                     static_cast<CWnd*>(&m_btnRpPlay), static_cast<CWnd*>(&m_btnRpNext), static_cast<CWnd*>(&m_btnRpLast),
                     static_cast<CWnd*>(&m_sldRp), static_cast<CWnd*>(&m_cmbRpSpeed), static_cast<CWnd*>(&m_lblRpInfo),
                     static_cast<CWnd*>(&m_btnRpClose) })
        if (w->GetSafeHwnd()) w->ShowWindow(show);
}

BOOL CommandPanel::OnEraseBkgnd(CDC* pDC)
{
    CRect rc; GetClientRect(&rc);
    pDC->FillSolidRect(rc, Theme::Get().panel);
    return TRUE;
}

HBRUSH CommandPanel::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    if (nCtlColor == CTLCOLOR_STATIC || nCtlColor == CTLCOLOR_BTN) {
        pDC->SetBkMode(TRANSPARENT);
        pDC->SetTextColor(pWnd == &m_lblRec && m_recording ? Theme::Get().danger : Theme::Get().text);
        return static_cast<HBRUSH>(m_bgBrush.GetSafeHandle());
    }
    // Edit fields, list boxes and combo drop-downs: keep text readable in the dark theme.
    // Editable fields: white (light theme) so they stand out from read-only
    // read-outs, which arrive as CTLCOLOR_STATIC and keep the grey background.
    pDC->SetTextColor(Theme::Get().text);
    pDC->SetBkColor(Theme::Get().plot);
    return Theme::FieldBrush();
}

// ---------------------------------------------------------------------------
void CommandPanel::SetConnected(bool c)
{
    m_connected = c;
    if (m_btnConnect.GetSafeHwnd()) m_btnConnect.SetWindowText(c ? _T("Disconnect") : _T("Connect"));
    m_btnStart.EnableWindow(c);
    m_btnStop.EnableWindow(c);
    m_btnTrigger.EnableWindow(c);
    m_btnAbort.EnableWindow(c);
    if (!c) SetRunning(false);
}

void CommandPanel::SetRunning(bool running)
{
    m_running = running;
}

void CommandPanel::SetHold(bool hold)
{
    m_hold = hold;
    if (m_btnHold.GetSafeHwnd()) m_btnHold.SetWindowText(hold ? _T("\u25B6 Live") : _T("\u23F8 Hold"));
}

void CommandPanel::SetRecording(bool rec, const CString& info)
{
    m_recording = rec;
    if (m_btnRecord.GetSafeHwnd()) m_btnRecord.SetWindowText(rec ? _T("\u25A0 Stop rec") : _T("\u25CF Record"));
    if (m_lblRec.GetSafeHwnd()) { m_lblRec.SetWindowText(info); m_lblRec.Invalidate(); }
}

void CommandPanel::SetReplay(bool active, const CString& name, int count)
{
    const bool changed = (active != m_replay);
    m_replay  = active;
    m_rpCount = count;
    if (m_lblRpName.GetSafeHwnd()) m_lblRpName.SetWindowText(name);
    if (m_sldRp.GetSafeHwnd()) {
        m_sldRp.SetRange(0, count > 0 ? count - 1 : 0, TRUE);
        m_sldRp.SetPos(0);
    }
    m_btnOpen.SetWindowText(active ? _T("Replay") : _T("Open..."));
    Relayout();
    if (changed && GetParent()) GetParent()->PostMessage(WM_SIZE);
}

void CommandPanel::SetReplayPos(int index, bool playing, const CString& info)
{
    m_playing = playing;
    if (m_sldRp.GetSafeHwnd() && m_sldRp.GetPos() != index) m_sldRp.SetPos(index);
    if (m_btnRpPlay.GetSafeHwnd()) m_btnRpPlay.SetWindowText(playing ? _T("Pause") : _T("Play"));
    if (m_lblRpInfo.GetSafeHwnd()) m_lblRpInfo.SetWindowText(info);
}

void CommandPanel::PopulateComPorts(const std::vector<CString>& ports, const CString& preferred)
{
    if (!m_cmbCom.GetSafeHwnd()) return;
    CString cur;
    m_cmbCom.GetWindowText(cur);
    if (cur.IsEmpty()) cur = preferred;
    m_cmbCom.ResetContent();
    for (auto& p : ports) m_cmbCom.AddString(p);
    int idx = cur.IsEmpty() ? CB_ERR : m_cmbCom.FindStringExact(-1, cur);
    if (idx != CB_ERR) m_cmbCom.SetCurSel(idx);
    else if (!cur.IsEmpty() && ports.empty()) m_cmbCom.SetWindowText(cur);
    else if (!ports.empty()) m_cmbCom.SetCurSel(0);
}

CString CommandPanel::GetSelectedPort() const
{
    CString s;
    if (m_cmbCom.GetSafeHwnd()) const_cast<CComboBox&>(m_cmbCom).GetWindowText(s);
    s.Trim();
    return s;
}

void CommandPanel::Post(UINT msg, WPARAM wp, LPARAM lp)
{
    if (CWnd* p = GetParent())
        if (::IsWindow(p->GetSafeHwnd()))
            p->PostMessage(msg, wp, lp);
}

void CommandPanel::OnConnect()    { Post(m_connected ? WM_APP_CMD_DISCONNECT : WM_APP_CMD_CONNECT); }
void CommandPanel::OnStart()      { Post(WM_APP_CMD_START); }
void CommandPanel::OnStop()       { Post(WM_APP_CMD_STOP); }
void CommandPanel::OnTrigger()    { Post(WM_APP_CMD_TRIGGER); }
void CommandPanel::OnAbort()      { Post(WM_APP_CMD_ABORT); }
void CommandPanel::OnHold()       { Post(WM_APP_CMD_HOLD, m_hold ? 0 : 1); }
void CommandPanel::OnRecord()     { Post(WM_APP_CMD_RECORD); }
void CommandPanel::OnOpenReplay() { Post(WM_APP_CMD_OPEN_REPLAY); }
void CommandPanel::OnSaveFrame()  { Post(WM_APP_CMD_SAVE_FRAME); }
void CommandPanel::OnClear()      { Post(WM_APP_CMD_CLEAR); }
void CommandPanel::OnRpPlay()     { Post(WM_APP_REPLAY_CTRL, m_playing ? REPLAY_PAUSE : REPLAY_PLAY); }
void CommandPanel::OnRpFirst()    { Post(WM_APP_REPLAY_CTRL, REPLAY_FIRST); }
void CommandPanel::OnRpPrev()     { Post(WM_APP_REPLAY_CTRL, REPLAY_PREV); }
void CommandPanel::OnRpNext()     { Post(WM_APP_REPLAY_CTRL, REPLAY_NEXT); }
void CommandPanel::OnRpLast()     { Post(WM_APP_REPLAY_CTRL, REPLAY_LAST); }
void CommandPanel::OnRpClose()    { Post(WM_APP_REPLAY_CTRL, REPLAY_CLOSE); }

void CommandPanel::OnRpSpeed()
{
    static const int kSpeed[] = { 25, 50, 100, 200, 400, 800, 0 };
    int sel = m_cmbRpSpeed.GetCurSel();
    if (sel < 0 || sel > 6) sel = 2;
    Post(WM_APP_REPLAY_CTRL, REPLAY_SPEED, kSpeed[sel]);
}

void CommandPanel::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pBar)
{
    if (pBar && pBar->GetSafeHwnd() == m_sldRp.GetSafeHwnd()) {
        if (nSBCode == TB_THUMBTRACK || nSBCode == TB_THUMBPOSITION || nSBCode == TB_ENDTRACK ||
            nSBCode == TB_LINEUP || nSBCode == TB_LINEDOWN || nSBCode == TB_PAGEUP || nSBCode == TB_PAGEDOWN)
            Post(WM_APP_REPLAY_CTRL, REPLAY_SEEK, m_sldRp.GetPos());
        return;
    }
    CWnd::OnHScroll(nSBCode, nPos, pBar);
}

void CommandPanel::OnComDropDown()
{
    if (CWnd* p = GetParent())
        if (::IsWindow(p->GetSafeHwnd()))
            p->SendMessage(WM_APP_REFRESH_PORTS);
}
