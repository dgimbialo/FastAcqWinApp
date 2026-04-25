#include "pch.h"
#include "CommandPanel.h"
#include "AppMessages.h"
#include "resource.h"

BEGIN_MESSAGE_MAP(CommandPanel, CWnd)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_BN_CLICKED(IDC_BTN_CONNECT,     &CommandPanel::OnConnect)
    ON_BN_CLICKED(IDC_BTN_START,       &CommandPanel::OnStart)
    ON_BN_CLICKED(IDC_BTN_STOP,        &CommandPanel::OnStop)
    ON_BN_CLICKED(IDC_BTN_SET_FREQ,    &CommandPanel::OnSetFreq)
    ON_BN_CLICKED(IDC_BTN_SET_SAMPLES, &CommandPanel::OnSetSamples)
    ON_BN_CLICKED(IDC_BTN_PING,        &CommandPanel::OnPing)
    ON_BN_CLICKED(IDC_BTN_SAVE_FRAME,  &CommandPanel::OnSaveFrame)
    ON_BN_CLICKED(IDC_BTN_CLEAR,       &CommandPanel::OnClear)
    ON_BN_CLICKED(IDC_BTN_APPLY_MODE,  &CommandPanel::OnApplyMode)
    ON_BN_CLICKED(IDC_BTN_APPLY_DATA,  &CommandPanel::OnApplyData)
    ON_BN_CLICKED(IDC_BTN_APPLY_INT,   &CommandPanel::OnApplyInterval)
    ON_BN_CLICKED(IDC_BTN_TRIGGER,     &CommandPanel::OnTrigger)
    ON_BN_CLICKED(IDC_BTN_GET_STATUS,  &CommandPanel::OnGetStatus)
    ON_BN_CLICKED(IDC_RDO_PC_RAW,      &CommandPanel::OnPcModeChanged)
    ON_BN_CLICKED(IDC_RDO_PC_FFT,      &CommandPanel::OnPcModeChanged)
    ON_BN_CLICKED(IDC_BTN_APPLY_FFT,   &CommandPanel::OnApplyFft)
END_MESSAGE_MAP()

BOOL CommandPanel::CreatePanel(CWnd* parent, UINT id)
{
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
    const DWORD es = WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_NUMBER;
    const DWORD cs = WS_CHILD | WS_VISIBLE | CBS_DROPDOWN | WS_VSCROLL;

    CRect rc(0, 0, 100, 22);

    m_cmbCom.Create(cs, rc, this, IDC_CMB_COM);
    m_btnConnect.Create(_T("Connect"),     bs, rc, this, IDC_BTN_CONNECT);
    m_btnStart.Create  (_T("Start"),       bs, rc, this, IDC_BTN_START);
    m_btnStop.Create   (_T("Stop"),        bs, rc, this, IDC_BTN_STOP);
    m_edtFreq.Create   (es, rc, this, IDC_EDT_FREQ);
    m_btnSetFreq.Create(_T("Set Freq"),    bs, rc, this, IDC_BTN_SET_FREQ);
    m_edtSamples.Create(es, rc, this, IDC_EDT_SAMPLES);
    m_btnSetSamples.Create(_T("Set Samples"), bs, rc, this, IDC_BTN_SET_SAMPLES);
    m_btnPing.Create   (_T("Ping"),        bs, rc, this, IDC_BTN_PING);
    m_btnSaveFrame.Create(_T("Save Frame"), bs, rc, this, IDC_BTN_SAVE_FRAME);
    m_btnClear.Create  (_T("Clear"),       bs, rc, this, IDC_BTN_CLEAR);

    // New FMCW / capture-mode controls (second row).
    DWORD ss = WS_CHILD | WS_VISIBLE | SS_LEFT;
    DWORD chk = WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX;
    DWORD cs2 = WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL;

    m_lblMode.Create(_T("Mode:"), ss, rc, this);
    m_cmbMode.Create(cs2, rc, this, IDC_CMB_MODE);
    m_cmbMode.AddString(_T("Idle"));
    m_cmbMode.AddString(_T("Continuous"));
    m_cmbMode.AddString(_T("Single"));
    m_cmbMode.SetCurSel(1);
    m_btnApplyMode.Create(_T("Apply"), bs, rc, this, IDC_BTN_APPLY_MODE);

    m_chkRaw.Create(_T("Raw"), chk, rc, this, IDC_CHK_RAW);
    m_chkRaw.SetCheck(BST_CHECKED);
    m_chkFft.Create(_T("FFT"), chk, rc, this, IDC_CHK_FFT);
    m_chkFft.SetCheck(BST_CHECKED);
    m_btnApplyData.Create(_T("Data Mask"), bs, rc, this, IDC_BTN_APPLY_DATA);

    m_lblInterval.Create(_T("Interval ms:"), ss, rc, this);
    m_edtInterval.Create(es, rc, this, IDC_EDT_INTERVAL);
    m_edtInterval.SetWindowText(_T("30"));
    m_btnApplyInterval.Create(_T("Set"), bs, rc, this, IDC_BTN_APPLY_INT);

    m_btnTrigger.Create(_T("Trigger"), bs, rc, this, IDC_BTN_TRIGGER);
    m_btnGetStatus.Create(_T("Status"), bs, rc, this, IDC_BTN_GET_STATUS);

    // Row 3: PC-side processing mode + FFT settings.
    m_lblPcMode.Create(_T("PC Mode:"), ss, rc, this);
    m_rdoPcRaw.Create(_T("RAW (local FFT)"),  WS_CHILD|WS_VISIBLE|BS_AUTORADIOBUTTON|WS_GROUP, rc, this, IDC_RDO_PC_RAW);
    m_rdoPcFft.Create(_T("FFT (from MCU)"),   WS_CHILD|WS_VISIBLE|BS_AUTORADIOBUTTON,           rc, this, IDC_RDO_PC_FFT);
    m_rdoPcRaw.SetCheck(BST_CHECKED);

    m_lblFftSize.Create(_T("FFT:"), ss, rc, this);
    m_cmbFftSize.Create(WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST|WS_VSCROLL, rc, this, IDC_CMB_FFT_SIZE);
    m_cmbFftSize.AddString(_T("512"));
    m_cmbFftSize.AddString(_T("1024"));
    m_cmbFftSize.AddString(_T("2048"));
    m_cmbFftSize.AddString(_T("4096"));
    m_cmbFftSize.SetCurSel(3);   // default 4096

    m_lblFftWin.Create(_T("Win:"), ss, rc, this);
    m_cmbFftWin.Create(WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST|WS_VSCROLL, rc, this, IDC_CMB_FFT_WIN);
    m_cmbFftWin.AddString(_T("Rectangular"));
    m_cmbFftWin.AddString(_T("Hann"));
    m_cmbFftWin.AddString(_T("Hamming"));
    m_cmbFftWin.AddString(_T("Blackman"));
    m_cmbFftWin.SetCurSel(1);   // default Hann

    m_chkFftLog.Create(_T("Log"), WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX, rc, this, IDC_CHK_FFT_LOG);
    m_btnApplyFft.Create(_T("Apply FFT"), bs, rc, this, IDC_BTN_APPLY_FFT);

    // Defaults.
    m_edtFreq.SetWindowText(_T("500"));
    m_edtSamples.SetWindowText(_T("65536"));

    CWnd* kids[] = { &m_cmbCom, &m_btnConnect, &m_btnStart, &m_btnStop,
                     &m_edtFreq, &m_btnSetFreq, &m_edtSamples, &m_btnSetSamples,
                     &m_btnPing, &m_btnSaveFrame, &m_btnClear,
                     &m_lblMode, &m_cmbMode, &m_btnApplyMode,
                     &m_chkRaw, &m_chkFft, &m_btnApplyData,
                     &m_lblInterval, &m_edtInterval, &m_btnApplyInterval,
                     &m_btnTrigger, &m_btnGetStatus,
                     &m_lblPcMode, &m_rdoPcRaw, &m_rdoPcFft,
                     &m_lblFftSize, &m_cmbFftSize, &m_lblFftWin, &m_cmbFftWin,
                     &m_chkFftLog, &m_btnApplyFft };
    for (auto* c : kids) c->SetFont(&m_font);

    // Hide MCU controls in row 1 (not needed now).
    for (CWnd* w : std::initializer_list<CWnd*>{
            &m_btnStart, &m_btnStop,
            &m_edtFreq, &m_btnSetFreq,
            &m_edtSamples, &m_btnSetSamples,
            &m_btnPing })
        w->ShowWindow(SW_HIDE);

    // Hide entire row 2 (MCU mode / data mask / interval / trigger).
    for (CWnd* w : std::initializer_list<CWnd*>{
            &m_lblMode, &m_cmbMode, &m_btnApplyMode,
            &m_chkRaw, &m_chkFft, &m_btnApplyData,
            &m_lblInterval, &m_edtInterval, &m_btnApplyInterval,
            &m_btnTrigger, &m_btnGetStatus })
        w->ShowWindow(SW_HIDE);

    SetConnected(false);
    // Apply initial FFT-controls enable state (RAW is default ? controls enabled).
    OnPcModeChanged();
    return 0;
}

void CommandPanel::OnSize(UINT t, int cx, int cy)
{
    CWnd::OnSize(t, cx, cy);
    Relayout();
}

void CommandPanel::Relayout()
{
    CRect rc; GetClientRect(&rc);
    const int pad  = 4;
    const int h    = 24;    // control height
    const int lh   = 16;   // label height

    // Helper: place a control at current x,y and advance x.
    // ctrlH = actual window height (200 for combo dropdown, h for others).
    int x = 0, y = 0;
    auto place = [&](CWnd& w, int ww, int hh) {
        if (w.GetSafeHwnd()) w.MoveWindow(x, y, ww, hh);
        x += ww + pad;
    };
    // Place a label vertically centred inside the control row.
    auto placeLabel = [&](CStatic& w, int ww) {
        if (w.GetSafeHwnd()) w.MoveWindow(x, y + (h - lh) / 2, ww, lh);
        x += ww + pad;
    };

    // ?? Row 1: COM port + Connect + Save Frame + Clear ??????????????????????
    y = pad; x = pad;
    place(m_cmbCom,      120, 200);   // combo dropdown needs tall rect
    place(m_btnConnect,   90, h);
    place(m_btnSaveFrame,100, h);
    place(m_btnClear,     70, h);

    // Hidden in row 1 (MCU controls – kept but not shown):
    // m_btnStart, m_btnStop, m_edtFreq, m_btnSetFreq,
    // m_edtSamples, m_btnSetSamples, m_btnPing
    // (already SW_HIDE from OnCreate)

    // ?? Row 2: PC mode radios + FFT settings ????????????????????????????????
    y = pad + h + pad; x = pad;
    placeLabel(m_lblPcMode,  58);
    place(m_rdoPcRaw,       130, h);
    place(m_rdoPcFft,       120, h);
    x += 8;
    placeLabel(m_lblFftSize,  30);
    place(m_cmbFftSize,       72, 200);
    x += 4;
    placeLabel(m_lblFftWin,   32);
    place(m_cmbFftWin,       116, 200);
    x += 4;
    place(m_chkFftLog,        46, h);
    place(m_btnApplyFft,      82, h);

    // Row 2 (MCU mode/interval/trigger) – all hidden, no layout needed.
}

void CommandPanel::SetConnected(bool c)
{
    m_connected = c;
    if (m_btnConnect.GetSafeHwnd())
        m_btnConnect.SetWindowText(c ? _T("Disconnect") : _T("Connect"));
    m_btnStart.EnableWindow(c);
    m_btnStop.EnableWindow(c);
    m_btnSetFreq.EnableWindow(c);
    m_btnSetSamples.EnableWindow(c);
    m_btnPing.EnableWindow(c);
    m_btnApplyMode.EnableWindow(c);
    m_btnApplyData.EnableWindow(c);
    m_btnApplyInterval.EnableWindow(c);
    m_btnTrigger.EnableWindow(c);
    m_btnGetStatus.EnableWindow(c);
}

bool CommandPanel::IsPcRawMode() const
{
    if (!m_rdoPcRaw.GetSafeHwnd()) return true;
    return m_rdoPcRaw.GetCheck() == BST_CHECKED;
}

FftSettings CommandPanel::GetFftSettings() const
{
    FftSettings cfg;
    if (!m_cmbFftSize.GetSafeHwnd()) return cfg;

    static const int kSizes[] = { 512, 1024, 2048, 4096 };
    int sel = m_cmbFftSize.GetCurSel();
    if (sel >= 0 && sel < 4) cfg.size = kSizes[sel];

    sel = m_cmbFftWin.GetCurSel();
    if (sel >= 0 && sel <= 3)
        cfg.window = static_cast<FftWindow>(sel);

    cfg.logScale = (m_chkFftLog.GetCheck() == BST_CHECKED);
    return cfg;
}

void CommandPanel::OnPcModeChanged()
{
    bool rawMode = IsPcRawMode();
    // Enable/disable FFT settings controls (and their labels) based on mode.
    m_lblFftSize.EnableWindow(rawMode);
    m_cmbFftSize.EnableWindow(rawMode);
    m_lblFftWin .EnableWindow(rawMode);
    m_cmbFftWin .EnableWindow(rawMode);
    m_chkFftLog .EnableWindow(rawMode);
    m_btnApplyFft.EnableWindow(rawMode);
    // Notify parent (skip if no parent yet during OnCreate initialisation).
    CWnd* pParent = GetParent();
    if (pParent && ::IsWindow(pParent->GetSafeHwnd()))
        pParent->PostMessage(WM_APP_ACQ_MODE, rawMode ? 0 : 1, 0);
}

void CommandPanel::OnApplyFft()
{
    FftSettings* p = new FftSettings(GetFftSettings());
    CWnd* pParent = GetParent();
    if (pParent) { if (!pParent->PostMessage(WM_APP_FFT_SETTINGS, 0, reinterpret_cast<LPARAM>(p))) delete p; }
    else delete p;
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

uint16_t CommandPanel::GetFreqHz() const
{
    CString s;
    const_cast<CEdit&>(m_edtFreq).GetWindowText(s);
    int v = _ttoi(s);
    if (v < 1)      v = 1;
    if (v > 0xFFFF) v = 0xFFFF;
    return static_cast<uint16_t>(v);
}

uint32_t CommandPanel::GetSamples() const
{
    CString s;
    const_cast<CEdit&>(m_edtSamples).GetWindowText(s);
    int v = _ttoi(s);
    if (v < 0) v = 0;
    return static_cast<uint32_t>(v);
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
void CommandPanel::OnStart()      { PostToParent(this, WM_APP_CMD_START); }
void CommandPanel::OnStop()       { PostToParent(this, WM_APP_CMD_STOP); }
void CommandPanel::OnSetFreq()    { PostToParent(this, WM_APP_CMD_SET_FREQ, GetFreqHz()); }
void CommandPanel::OnSetSamples() { PostToParent(this, WM_APP_CMD_SET_SAMPLES, GetSamples()); }
void CommandPanel::OnPing()       { PostToParent(this, WM_APP_CMD_PING); }
void CommandPanel::OnSaveFrame()  { PostToParent(this, WM_APP_CMD_SAVE_FRAME); }
void CommandPanel::OnClear()      { PostToParent(this, WM_APP_CMD_CLEAR); }
void CommandPanel::OnApplyMode()     { PostToParent(this, WM_APP_CMD_SET_MODE,      GetModeSel()); }
void CommandPanel::OnApplyData()     { PostToParent(this, WM_APP_CMD_SET_DATA_MASK, GetDataMask()); }
void CommandPanel::OnApplyInterval() { PostToParent(this, WM_APP_CMD_SET_INTERVAL,  GetIntervalMs()); }
void CommandPanel::OnTrigger()       { PostToParent(this, WM_APP_CMD_TRIGGER); }
void CommandPanel::OnGetStatus()     { PostToParent(this, WM_APP_CMD_GET_STATUS); }

uint16_t CommandPanel::GetModeSel() const
{
    int sel = const_cast<CComboBox&>(m_cmbMode).GetCurSel();
    if (sel < 0) sel = 0;
    if (sel > 2) sel = 2;
    return static_cast<uint16_t>(sel);
}

uint8_t CommandPanel::GetDataMask() const
{
    uint8_t m = 0;
    if (const_cast<CButton&>(m_chkRaw).GetCheck() == BST_CHECKED) m |= 0x01;
    if (const_cast<CButton&>(m_chkFft).GetCheck() == BST_CHECKED) m |= 0x02;
    return m;
}

uint16_t CommandPanel::GetIntervalMs() const
{
    CString s;
    const_cast<CEdit&>(m_edtInterval).GetWindowText(s);
    int v = _ttoi(s);
    if (v < 5)     v = 5;
    if (v > 10000) v = 10000;
    return static_cast<uint16_t>(v);
}
