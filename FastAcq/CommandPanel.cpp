#include "pch.h"
#include "CommandPanel.h"
#include "AppMessages.h"
#include "resource.h"

BEGIN_MESSAGE_MAP(CommandPanel, CWnd)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_BN_CLICKED(IDC_BTN_CONNECT,    &CommandPanel::OnConnect)
    ON_BN_CLICKED(IDC_BTN_START,      &CommandPanel::OnStart)
    ON_BN_CLICKED(IDC_BTN_STOP,       &CommandPanel::OnStop)
    ON_BN_CLICKED(IDC_BTN_TRIGGER,    &CommandPanel::OnTrigger)
    ON_BN_CLICKED(IDC_BTN_ABORT,      &CommandPanel::OnAbort)
    ON_BN_CLICKED(IDC_BTN_SAVE_FRAME, &CommandPanel::OnSaveFrame)
    ON_BN_CLICKED(IDC_BTN_CLEAR,      &CommandPanel::OnClear)
    ON_CBN_DROPDOWN(IDC_CMB_COM,      &CommandPanel::OnComDropDown)
END_MESSAGE_MAP()

BOOL CommandPanel::CreatePanel(CWnd* parent, UINT id)
{
    // Standard dialog-face background (native Win32 look).
    LPCTSTR cls = AfxRegisterWndClass(0, ::LoadCursor(nullptr, IDC_ARROW),
                                      reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1),
                                      nullptr);
    return Create(cls, nullptr, WS_CHILD | WS_VISIBLE,
                  CRect(0, 0, 10, 10), parent, id);
}

int CommandPanel::OnCreate(LPCREATESTRUCT lpcs)
{
    if (CWnd::OnCreate(lpcs) == -1) return -1;

    m_font.CreatePointFont(90, _T("Segoe UI"));

    const DWORD bs = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON;
    const DWORD cs = WS_CHILD | WS_VISIBLE | CBS_DROPDOWN | WS_VSCROLL;

    CRect rc(0, 0, 100, 22);

    m_cmbCom.Create(cs, rc, this, IDC_CMB_COM);
    m_btnConnect.Create(_T("Connect"),      bs, rc, this, IDC_BTN_CONNECT);
    m_btnStart.Create  (_T("Start"),        bs, rc, this, IDC_BTN_START);
    m_btnStop.Create   (_T("Stop"),         bs, rc, this, IDC_BTN_STOP);
    m_btnTrigger.Create(_T("Trigger"),      bs, rc, this, IDC_BTN_TRIGGER);
    m_btnAbort.Create  (_T("Abort"),        bs, rc, this, IDC_BTN_ABORT);
    m_btnSaveFrame.Create(_T("Save Frame"), bs, rc, this, IDC_BTN_SAVE_FRAME);
    m_btnClear.Create  (_T("Clear"),        bs, rc, this, IDC_BTN_CLEAR);

    CWnd* kids[] = { &m_cmbCom, &m_btnConnect, &m_btnStart, &m_btnStop,
                     &m_btnTrigger, &m_btnAbort, &m_btnSaveFrame, &m_btnClear };
    for (auto* c : kids) c->SetFont(&m_font);

    SetConnected(false);
    return 0;
}

void CommandPanel::OnSize(UINT t, int cx, int cy)
{
    CWnd::OnSize(t, cx, cy);
    Relayout();
}

void CommandPanel::Relayout()
{
    const int pad = 4;
    const int h   = 24;

    int x = pad, y = pad;
    auto place = [&](CWnd& w, int ww, int hh) {
        if (w.GetSafeHwnd()) w.MoveWindow(x, y, ww, hh);
        x += ww + pad;
    };

    place(m_cmbCom,      130, 200);   // combo dropdown needs tall rect
    place(m_btnConnect,   95, h);
    place(m_btnStart,     75, h);
    place(m_btnStop,      70, h);
    place(m_btnTrigger,   75, h);
    place(m_btnAbort,     70, h);
    place(m_btnSaveFrame,100, h);
    place(m_btnClear,     70, h);
}

void CommandPanel::SetConnected(bool c)
{
    m_connected = c;
    if (m_btnConnect.GetSafeHwnd())
        m_btnConnect.SetWindowText(c ? _T("Disconnect") : _T("Connect"));
    m_btnStart.EnableWindow(c);
    m_btnStop.EnableWindow(c);
    m_btnTrigger.EnableWindow(c);
    m_btnAbort.EnableWindow(c);
}

void CommandPanel::PopulateComPorts(const std::vector<CString>& ports)
{
    if (!m_cmbCom.GetSafeHwnd()) return;
    CString cur;
    m_cmbCom.GetWindowText(cur);
    m_cmbCom.ResetContent();
    for (auto& p : ports) m_cmbCom.AddString(p);
    if (!cur.IsEmpty()) m_cmbCom.SetWindowText(cur);
    else if (!ports.empty()) m_cmbCom.SetCurSel(0);
}

CString CommandPanel::GetSelectedPort() const
{
    CString s;
    if (m_cmbCom.GetSafeHwnd())
        const_cast<CComboBox&>(m_cmbCom).GetWindowText(s);
    s.Trim();
    return s;
}

static void PostToParent(CWnd* self, UINT msg, WPARAM wp = 0, LPARAM lp = 0)
{
    if (CWnd* p = self->GetParent())
        if (::IsWindow(p->GetSafeHwnd()))
            p->PostMessage(msg, wp, lp);
}

void CommandPanel::OnConnect()
{
    PostToParent(this, m_connected ? WM_APP_CMD_DISCONNECT : WM_APP_CMD_CONNECT);
}
void CommandPanel::OnStart()     { PostToParent(this, WM_APP_CMD_START); }
void CommandPanel::OnStop()      { PostToParent(this, WM_APP_CMD_STOP); }
void CommandPanel::OnTrigger()   { PostToParent(this, WM_APP_CMD_TRIGGER); }
void CommandPanel::OnAbort()     { PostToParent(this, WM_APP_CMD_ABORT); }
void CommandPanel::OnSaveFrame() { PostToParent(this, WM_APP_CMD_SAVE_FRAME); }
void CommandPanel::OnClear()     { PostToParent(this, WM_APP_CMD_CLEAR); }

void CommandPanel::OnComDropDown()
{
    // Synchronous: repopulate the list before the dropdown becomes visible,
    // so a freshly plugged-in device appears without restarting the app.
    if (CWnd* p = GetParent())
        if (::IsWindow(p->GetSafeHwnd()))
            p->SendMessage(WM_APP_REFRESH_PORTS);
}
