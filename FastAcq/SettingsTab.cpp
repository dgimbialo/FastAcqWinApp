#include "pch.h"
#include "SettingsTab.h"
#include "AppMessages.h"
#include "resource.h"

BEGIN_MESSAGE_MAP(SettingsTabWnd, CWnd)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_BN_CLICKED(IDC_BTN_APPLY_MODE,  &SettingsTabWnd::OnApplyMode)
    ON_BN_CLICKED(IDC_BTN_SET_FREQ,    &SettingsTabWnd::OnSetFreq)
    ON_BN_CLICKED(IDC_BTN_SET_SAMPLES, &SettingsTabWnd::OnSetSamples)
    ON_BN_CLICKED(IDC_BTN_APPLY_INT,   &SettingsTabWnd::OnApplyInterval)
    ON_BN_CLICKED(IDC_BTN_APPLY_DATA,  &SettingsTabWnd::OnApplyData)
    ON_BN_CLICKED(IDC_BTN_SET_AMP,     &SettingsTabWnd::OnSetAmplitude)
    ON_BN_CLICKED(IDC_BTN_SET_BURST,   &SettingsTabWnd::OnSetBurst)
    ON_BN_CLICKED(IDC_BTN_PING,        &SettingsTabWnd::OnPing)
    ON_BN_CLICKED(IDC_BTN_GET_STATUS,  &SettingsTabWnd::OnGetStatus)
    ON_BN_CLICKED(IDC_RDO_PC_RAW,      &SettingsTabWnd::OnPcModeChanged)
    ON_BN_CLICKED(IDC_RDO_PC_FFT,      &SettingsTabWnd::OnPcModeChanged)
END_MESSAGE_MAP()

BOOL SettingsTabWnd::CreateTab(CWnd* parent, UINT id)
{
    LPCTSTR cls = AfxRegisterWndClass(0, ::LoadCursor(nullptr, IDC_ARROW),
                                      reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1),
                                      nullptr);
    return Create(cls, nullptr, WS_CHILD, CRect(0, 0, 10, 10), parent, id);
}

int SettingsTabWnd::OnCreate(LPCREATESTRUCT lpcs)
{
    if (CWnd::OnCreate(lpcs) == -1) return -1;

    m_font.CreatePointFont(90, _T("Segoe UI"));
    m_hdrFont.CreateFont(-14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                         DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                         CLEARTYPE_QUALITY, VARIABLE_PITCH | FF_SWISS, _T("Segoe UI"));

    const DWORD bs  = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON;
    const DWORD es  = WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_NUMBER;
    const DWORD ss  = WS_CHILD | WS_VISIBLE | SS_LEFT;
    const DWORD chk = WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX;
    const DWORD cs2 = WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL;

    CRect rc(0, 0, 100, 22);

    m_hdrMcu.Create(_T("MCU acquisition"), ss, rc, this);

    m_lblMode.Create(_T("Mode:"), ss, rc, this);
    m_cmbMode.Create(cs2, rc, this, IDC_CMB_MODE);
    m_cmbMode.AddString(_T("Idle"));
    m_cmbMode.AddString(_T("Continuous"));
    m_cmbMode.AddString(_T("Single"));
    m_cmbMode.SetCurSel(1);
    m_btnApplyMode.Create(_T("Apply"), bs, rc, this, IDC_BTN_APPLY_MODE);

    m_lblFreq.Create(_T("Chirp freq, Hz (100..24000):"), ss, rc, this);
    m_edtFreq.Create(es, rc, this, IDC_EDT_FREQ);
    m_edtFreq.SetWindowText(_T("458"));
    m_btnSetFreq.Create(_T("Set"), bs, rc, this, IDC_BTN_SET_FREQ);

    m_lblSamples.Create(_T("Samples (0 = auto, max 650000):"), ss, rc, this);
    m_edtSamples.Create(es, rc, this, IDC_EDT_SAMPLES);
    m_edtSamples.SetWindowText(_T("0"));
    m_btnSetSamples.Create(_T("Set"), bs, rc, this, IDC_BTN_SET_SAMPLES);

    m_lblInterval.Create(_T("Interval between cycles, ms:"), ss, rc, this);
    m_edtInterval.Create(es, rc, this, IDC_EDT_INTERVAL);
    m_edtInterval.SetWindowText(_T("30"));
    m_btnApplyInterval.Create(_T("Set"), bs, rc, this, IDC_BTN_APPLY_INT);

    m_lblAmplitude.Create(_T("Chirp amplitude, DAC (1..4095):"), ss, rc, this);
    m_edtAmplitude.Create(es, rc, this, IDC_EDT_AMPLITUDE);
    m_edtAmplitude.SetWindowText(_T("4095"));
    m_btnSetAmp.Create(_T("Set"), bs, rc, this, IDC_BTN_SET_AMP);

    m_lblBurst.Create(_T("Chirps per capture (1..1024):"), ss, rc, this);
    m_edtBurst.Create(es, rc, this, IDC_EDT_BURST);
    m_edtBurst.SetWindowText(_T("1"));
    m_btnSetBurst.Create(_T("Set"), bs, rc, this, IDC_BTN_SET_BURST);

    m_hdrData.Create(_T("Data / diagnostics"), ss, rc, this);

    m_lblData.Create(_T("Frame content:"), ss, rc, this);
    m_chkRaw.Create(_T("Raw"), chk, rc, this, IDC_CHK_RAW);
    m_chkRaw.SetCheck(BST_CHECKED);
    m_chkFft.Create(_T("FFT"), chk, rc, this, IDC_CHK_FFT);
    m_chkFft.SetCheck(BST_CHECKED);
    m_btnApplyData.Create(_T("Apply mask"), bs, rc, this, IDC_BTN_APPLY_DATA);

    m_btnPing.Create(_T("Ping"), bs, rc, this, IDC_BTN_PING);
    m_btnGetStatus.Create(_T("Get Status"), bs, rc, this, IDC_BTN_GET_STATUS);

    m_hdrPc.Create(_T("PC processing"), ss, rc, this);

    m_lblPcMode.Create(_T("Source:"), ss, rc, this);
    m_rdoPcRaw.Create(_T("RAW (local FFT)"),
                      WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
                      rc, this, IDC_RDO_PC_RAW);
    m_rdoPcFft.Create(_T("FFT (from MCU)"),
                      WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
                      rc, this, IDC_RDO_PC_FFT);
    m_rdoPcRaw.SetCheck(BST_CHECKED);

    CWnd* kids[] = { &m_lblMode, &m_cmbMode, &m_btnApplyMode,
                     &m_lblFreq, &m_edtFreq, &m_btnSetFreq,
                     &m_lblSamples, &m_edtSamples, &m_btnSetSamples,
                     &m_lblInterval, &m_edtInterval, &m_btnApplyInterval,
                     &m_lblAmplitude, &m_edtAmplitude, &m_btnSetAmp,
                     &m_lblBurst, &m_edtBurst, &m_btnSetBurst,
                     &m_lblData, &m_chkRaw, &m_chkFft, &m_btnApplyData,
                     &m_btnPing, &m_btnGetStatus,
                     &m_lblPcMode, &m_rdoPcRaw, &m_rdoPcFft };
    for (auto* c : kids) c->SetFont(&m_font);

    m_hdrMcu.SetFont(&m_hdrFont);
    m_hdrData.SetFont(&m_hdrFont);
    m_hdrPc.SetFont(&m_hdrFont);

    SetConnected(false);
    return 0;
}

void SettingsTabWnd::OnSize(UINT t, int cx, int cy)
{
    CWnd::OnSize(t, cx, cy);
    Relayout();
}

void SettingsTabWnd::Relayout()
{
    if (!m_lblMode.GetSafeHwnd()) return;

    const int pad   = 10;
    const int h     = 24;   // control height
    const int lh    = 18;   // label height
    const int rowH  = h + 8;
    const int lblW  = 210;  // settings label column
    const int ctlW  = 110;  // edit / combo column
    const int btnW  = 80;   // apply button column

    int x0 = 16, y = 12;

    auto header = [&](CStatic& s) {
        s.MoveWindow(x0, y, 300, 20);
        y += 28;
    };
    auto row = [&](CStatic& lbl, CWnd& ctl, CWnd& btn, int ctlH = 0) {
        lbl.MoveWindow(x0, y + (h - lh) / 2, lblW, lh);
        ctl.MoveWindow(x0 + lblW + pad, y, ctlW, ctlH ? ctlH : h);
        btn.MoveWindow(x0 + lblW + pad + ctlW + pad, y, btnW, h);
        y += rowH;
    };

    header(m_hdrMcu);
    row(m_lblMode,      m_cmbMode,      m_btnApplyMode, 200);
    row(m_lblFreq,      m_edtFreq,      m_btnSetFreq);
    row(m_lblSamples,   m_edtSamples,   m_btnSetSamples);
    row(m_lblInterval,  m_edtInterval,  m_btnApplyInterval);
    row(m_lblAmplitude, m_edtAmplitude, m_btnSetAmp);
    row(m_lblBurst,     m_edtBurst,     m_btnSetBurst);

    y += 8;
    header(m_hdrData);
    // Frame content: label + Raw + FFT + Apply mask
    m_lblData.MoveWindow(x0, y + (h - lh) / 2, lblW, lh);
    m_chkRaw.MoveWindow(x0 + lblW + pad,            y, 55, h);
    m_chkFft.MoveWindow(x0 + lblW + pad + 60,       y, 55, h);
    m_btnApplyData.MoveWindow(x0 + lblW + pad + 120, y, 100, h);
    y += rowH;
    m_btnPing.MoveWindow(x0 + lblW + pad, y, 70, h);
    m_btnGetStatus.MoveWindow(x0 + lblW + pad + 75, y, 90, h);
    y += rowH + 8;

    header(m_hdrPc);
    m_lblPcMode.MoveWindow(x0, y + (h - lh) / 2, lblW, lh);
    m_rdoPcRaw.MoveWindow(x0 + lblW + pad,        y, 140, h);
    m_rdoPcFft.MoveWindow(x0 + lblW + pad + 145,  y, 140, h);
}

void SettingsTabWnd::SetConnected(bool c)
{
    m_connected = c;
    m_btnApplyMode.EnableWindow(c);
    m_btnSetFreq.EnableWindow(c);
    m_btnSetSamples.EnableWindow(c);
    m_btnApplyInterval.EnableWindow(c);
    m_btnSetAmp.EnableWindow(c);
    m_btnSetBurst.EnableWindow(c);
    m_btnApplyData.EnableWindow(c);
    m_btnPing.EnableWindow(c);
    m_btnGetStatus.EnableWindow(c);
}

/* ---- getters (with firmware range clamps) ---- */

uint16_t SettingsTabWnd::GetFreqHz() const
{
    CString s; const_cast<CEdit&>(m_edtFreq).GetWindowText(s);
    int v = _ttoi(s);
    if (v < 100)   v = 100;
    if (v > 24000) v = 24000;
    return static_cast<uint16_t>(v);
}

uint32_t SettingsTabWnd::GetSamples() const
{
    CString s; const_cast<CEdit&>(m_edtSamples).GetWindowText(s);
    int v = _ttoi(s);
    if (v < 0)      v = 0;
    if (v > 650000) v = 650000;
    return static_cast<uint32_t>(v);
}

uint16_t SettingsTabWnd::GetModeSel() const
{
    int sel = const_cast<CComboBox&>(m_cmbMode).GetCurSel();
    if (sel < 0) sel = 0;
    if (sel > 2) sel = 2;
    return static_cast<uint16_t>(sel);
}

uint8_t SettingsTabWnd::GetDataMask() const
{
    uint8_t m = 0;
    if (const_cast<CButton&>(m_chkRaw).GetCheck() == BST_CHECKED) m |= 0x01;
    if (const_cast<CButton&>(m_chkFft).GetCheck() == BST_CHECKED) m |= 0x02;
    return m;
}

uint16_t SettingsTabWnd::GetIntervalMs() const
{
    CString s; const_cast<CEdit&>(m_edtInterval).GetWindowText(s);
    int v = _ttoi(s);
    if (v < 5)     v = 5;
    if (v > 10000) v = 10000;
    return static_cast<uint16_t>(v);
}

uint16_t SettingsTabWnd::GetAmplitude() const
{
    CString s; const_cast<CEdit&>(m_edtAmplitude).GetWindowText(s);
    int v = _ttoi(s);
    if (v < 1)    v = 1;
    if (v > 4095) v = 4095;
    return static_cast<uint16_t>(v);
}

uint16_t SettingsTabWnd::GetBurst() const
{
    CString s; const_cast<CEdit&>(m_edtBurst).GetWindowText(s);
    int v = _ttoi(s);
    if (v < 1)    v = 1;
    if (v > 1024) v = 1024;
    return static_cast<uint16_t>(v);
}

bool SettingsTabWnd::IsPcRawMode() const
{
    if (!m_rdoPcRaw.GetSafeHwnd()) return true;
    return m_rdoPcRaw.GetCheck() == BST_CHECKED;
}

/* ---- command routing (to the main frame, not the tab control parent) ---- */

void SettingsTabWnd::PostToMain(UINT msg, WPARAM wp, LPARAM lp)
{
    CWnd* pMain = AfxGetMainWnd();
    if (pMain && ::IsWindow(pMain->GetSafeHwnd()))
        pMain->PostMessage(msg, wp, lp);
}

void SettingsTabWnd::OnApplyMode()     { PostToMain(WM_APP_CMD_SET_MODE,      GetModeSel()); }
void SettingsTabWnd::OnSetFreq()       { PostToMain(WM_APP_CMD_SET_FREQ,      GetFreqHz()); }
void SettingsTabWnd::OnSetSamples()    { PostToMain(WM_APP_CMD_SET_SAMPLES,   GetSamples()); }
void SettingsTabWnd::OnApplyInterval() { PostToMain(WM_APP_CMD_SET_INTERVAL,  GetIntervalMs()); }
void SettingsTabWnd::OnApplyData()     { PostToMain(WM_APP_CMD_SET_DATA_MASK, GetDataMask()); }
void SettingsTabWnd::OnSetAmplitude()  { PostToMain(WM_APP_CMD_SET_AMPLITUDE, GetAmplitude()); }
void SettingsTabWnd::OnSetBurst()      { PostToMain(WM_APP_CMD_SET_BURST,     GetBurst()); }
void SettingsTabWnd::OnPing()          { PostToMain(WM_APP_CMD_PING); }
void SettingsTabWnd::OnGetStatus()     { PostToMain(WM_APP_CMD_GET_STATUS); }

void SettingsTabWnd::OnPcModeChanged()
{
    PostToMain(WM_APP_ACQ_MODE, IsPcRawMode() ? 0 : 1, 0);
}
