#include "pch.h"
#include "SettingsTab.h"
#include "AppMessages.h"
#include "resource.h"
#include <algorithm>
#include <shlobj.h>   // SHGetFolderPath (calibration ini in %APPDATA%)

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
    ON_BN_CLICKED(IDC_BTN_SET_RAMP,    &SettingsTabWnd::OnSetRamp)
    ON_BN_CLICKED(IDC_BTN_PING,        &SettingsTabWnd::OnPing)
    ON_BN_CLICKED(IDC_BTN_GET_STATUS,  &SettingsTabWnd::OnGetStatus)
    ON_BN_CLICKED(IDC_RDO_PC_RAW,      &SettingsTabWnd::OnPcModeChanged)
    ON_BN_CLICKED(IDC_RDO_PC_FFT,      &SettingsTabWnd::OnPcModeChanged)
    ON_BN_CLICKED(IDC_BTN_APPLY_PPM,   &SettingsTabWnd::OnApplyPpm)
    ON_EN_CHANGE(IDC_EDT_FREQ,         &SettingsTabWnd::OnFreqChanged)
    ON_EN_CHANGE(IDC_EDT_SAMPLES,      &SettingsTabWnd::OnParamChanged)
    ON_EN_CHANGE(IDC_EDT_INTERVAL,     &SettingsTabWnd::OnParamChanged)
    ON_EN_CHANGE(IDC_EDT_AMPLITUDE,    &SettingsTabWnd::OnParamChanged)
    ON_EN_CHANGE(IDC_EDT_BURST,        &SettingsTabWnd::OnParamChanged)
    ON_EN_CHANGE(IDC_EDT_RISE,         &SettingsTabWnd::OnRampChanged)
    ON_EN_CHANGE(IDC_EDT_FALL,         &SettingsTabWnd::OnRampChanged)
    ON_CBN_SELCHANGE(IDC_CMB_MODE,     &SettingsTabWnd::OnParamChanged)
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

    m_lblRamp.Create(_T("Ramp rise / fall, us (0/0 = from freq):"), ss, rc, this);
    m_edtRise.Create(es, rc, this, IDC_EDT_RISE);
    m_edtRise.SetWindowText(_T("1092"));
    m_edtFall.Create(es, rc, this, IDC_EDT_FALL);
    m_edtFall.SetWindowText(_T("1092"));
    m_btnSetRamp.Create(_T("Set ramp"), bs, rc, this, IDC_BTN_SET_RAMP);

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

    m_lblPpm.Create(_T("ADC clock, ppm:"), ss, rc, this);
    // Signed decimal value: no ES_NUMBER (it rejects '.' and '-').
    m_edtPpm.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, rc, this, IDC_EDT_PPM);
    m_btnApplyPpm.Create(_T("Apply"), bs, rc, this, IDC_BTN_APPLY_PPM);
    m_lblPpmHint.Create(_T("+ = ADC clock fast (measured f too low). "
                          "ppm = (f_true / f_measured - 1) * 1e6"), ss, rc, this);

    CWnd* kids[] = { &m_lblMode, &m_cmbMode, &m_btnApplyMode,
                     &m_lblFreq, &m_edtFreq, &m_btnSetFreq,
                     &m_lblSamples, &m_edtSamples, &m_btnSetSamples,
                     &m_lblInterval, &m_edtInterval, &m_btnApplyInterval,
                     &m_lblAmplitude, &m_edtAmplitude, &m_btnSetAmp,
                     &m_lblBurst, &m_edtBurst, &m_btnSetBurst,
                     &m_lblRamp, &m_edtRise, &m_edtFall, &m_btnSetRamp,
                     &m_lblData, &m_chkRaw, &m_chkFft, &m_btnApplyData,
                     &m_btnPing, &m_btnGetStatus,
                     &m_lblPcMode, &m_rdoPcRaw, &m_rdoPcFft,
                     &m_lblPpm, &m_edtPpm, &m_btnApplyPpm, &m_lblPpmHint };
    for (auto* c : kids) c->SetFont(&m_font);

    m_hdrMcu.SetFont(&m_hdrFont);
    m_hdrData.SetFont(&m_hdrFont);
    m_hdrPc.SetFont(&m_hdrFont);

    m_preview.CreateCtrl(this, IDC_CHIRP_PREVIEW);
    UpdatePreview();

    // Restore the stored calibration and hand it to the main frame.
    LoadFsPpm();
    PostToMain(WM_APP_FS_PPM);

    SetConnected(false);
    return 0;
}



bool SettingsTabWnd::ControlsReady() const
{
    return m_edtFreq.GetSafeHwnd() && m_edtRise.GetSafeHwnd() && m_edtFall.GetSafeHwnd() &&
           m_preview.GetSafeHwnd();
}

void SettingsTabWnd::OnParamChanged() { if (ControlsReady()) UpdatePreview(); }

// Frequency edited: mirror a symmetric ramp into rise/fall so the two ways of
// describing the chirp never disagree, and preview in frequency mode (this is
// also what CMD_START_CHIRP does on the device: it clears the ramp override).
void SettingsTabWnd::OnFreqChanged()
{
    // EN_CHANGE also fires from SetWindowText() inside OnCreate, before the
    // ramp edits exist; writing to them then throws and would leave
    // m_syncing stuck at true, silently disabling every later update.
    if (m_syncing || !ControlsReady()) return;
    m_rampMode = false;
    CString s; m_edtFreq.GetWindowText(s);
    int f = _ttoi(s);
    if (f >= 100 && f <= 24000) {
        int half = static_cast<int>(500000.0 / f + 0.5);   // us
        CString t; t.Format(_T("%d"), half);
        m_syncing = true;
        m_edtRise.SetWindowText(t);
        m_edtFall.SetWindowText(t);
        m_syncing = false;
    }
    UpdatePreview();
}

// Rise / fall edited: the resulting frequency is shown in the freq box and
// the preview switches to ramp mode (what CMD_SET_RAMP applies).
void SettingsTabWnd::OnRampChanged()
{
    if (m_syncing || !ControlsReady()) return;
    m_rampMode = true;
    uint16_t r = 0, f = 0; GetRampUs(r, f);
    if (r && f) {
        CString t; t.Format(_T("%d"), static_cast<int>(1e6 / (r + f) + 0.5));
        m_syncing = true;
        m_edtFreq.SetWindowText(t);
        m_syncing = false;
    }
    UpdatePreview();
}

void SettingsTabWnd::UpdatePreview()
{
    if (!m_preview.GetSafeHwnd() || !m_edtFreq.GetSafeHwnd()) return;
    ChirpParams p;
    CString s;
    m_edtFreq.GetWindowText(s);      p.freqHz     = static_cast<uint32_t>((std::max)(0, _ttoi(s)));
    m_edtSamples.GetWindowText(s);   p.samplesOvr = static_cast<uint32_t>((std::max)(0, _ttoi(s)));
    m_edtInterval.GetWindowText(s);  p.intervalMs = static_cast<uint32_t>((std::max)(0, _ttoi(s)));
    m_edtAmplitude.GetWindowText(s); p.amplitude  = static_cast<uint32_t>((std::min)(4095, (std::max)(1, _ttoi(s))));
    m_edtBurst.GetWindowText(s);     p.burst      = static_cast<uint32_t>((std::min)(1024, (std::max)(1, _ttoi(s))));
    uint16_t r = 0, f = 0; GetRampUs(r, f);
    p.riseUs = m_rampMode ? r : 0;   // freq mode ignores the ramp boxes
    p.fallUs = m_rampMode ? f : 0;
    p.mode = (std::min)(2, (std::max)(0, m_cmbMode.GetCurSel()));
    m_preview.SetParams(p);
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
    // Ramp: two narrow edits share the control column.
    m_lblRamp.MoveWindow(x0, y + (h - lh) / 2, lblW, lh);
    m_edtRise.MoveWindow(x0 + lblW + pad,                   y, ctlW / 2 - 3, h);
    m_edtFall.MoveWindow(x0 + lblW + pad + ctlW / 2 + 3,    y, ctlW / 2 - 3, h);
    m_btnSetRamp.MoveWindow(x0 + lblW + pad + ctlW + pad,   y, btnW, h);
    y += rowH;
    const int yLeftEnd = y;

    // Right column: data / diagnostics and PC processing next to the MCU block,
    // so the chirp preview below gets the vertical space.
    const int x1   = x0 + lblW + pad + ctlW + pad + btnW + 36;
    const int lbl2 = 110;
    y = 12;
    m_hdrData.MoveWindow(x1, y, 300, 20); y += 28;
    m_lblData.MoveWindow(x1, y + (h - lh) / 2, lbl2, lh);
    m_chkRaw.MoveWindow(x1 + lbl2 + pad,             y, 55, h);
    m_chkFft.MoveWindow(x1 + lbl2 + pad + 60,        y, 55, h);
    m_btnApplyData.MoveWindow(x1 + lbl2 + pad + 120, y, 100, h);
    y += rowH;
    m_btnPing.MoveWindow(x1 + lbl2 + pad,      y, 70, h);
    m_btnGetStatus.MoveWindow(x1 + lbl2 + pad + 75, y, 90, h);
    y += rowH + 8;
    m_hdrPc.MoveWindow(x1, y, 300, 20); y += 28;
    m_lblPcMode.MoveWindow(x1, y + (h - lh) / 2, lbl2, lh);
    m_rdoPcRaw.MoveWindow(x1 + lbl2 + pad,       y, 140, h);
    m_rdoPcFft.MoveWindow(x1 + lbl2 + pad + 145, y, 140, h);
    y += rowH;
    m_lblPpm.MoveWindow(x1, y + (h - lh) / 2, lbl2, lh);
    m_edtPpm.MoveWindow(x1 + lbl2 + pad, y, 90, h);
    m_btnApplyPpm.MoveWindow(x1 + lbl2 + pad + 95, y, 70, h);
    y += rowH;
    m_lblPpmHint.MoveWindow(x1, y, 460, lh);
    y += lh + 6;
    y = (std::max)(y, yLeftEnd) + 6;

    // Live chirp preview fills the remaining space.
    CRect rc; GetClientRect(&rc);
    int ph = rc.Height() - y - 12;
    if (ph < 210) ph = 210;
    if (m_preview.GetSafeHwnd())
        m_preview.MoveWindow(x0, y, (std::max)(200, rc.Width() - 2 * x0), ph);
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
    m_btnSetRamp.EnableWindow(c);
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

void SettingsTabWnd::GetRampUs(uint16_t& riseUs, uint16_t& fallUs) const
{
    CString s;
    const_cast<CEdit&>(m_edtRise).GetWindowText(s); int r = _ttoi(s);
    const_cast<CEdit&>(m_edtFall).GetWindowText(s); int f = _ttoi(s);
    if (r < 0) r = 0; if (r > 65535) r = 65535;
    if (f < 0) f = 0; if (f > 65535) f = 65535;
    if (r == 0 || f == 0) { r = 0; f = 0; }   // both zero = symmetric from freq
    riseUs = static_cast<uint16_t>(r);
    fallUs = static_cast<uint16_t>(f);
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
void SettingsTabWnd::OnSetFreq()       { m_rampMode = false; UpdatePreview(); PostToMain(WM_APP_CMD_SET_FREQ, GetFreqHz()); }
void SettingsTabWnd::OnSetSamples()    { PostToMain(WM_APP_CMD_SET_SAMPLES,   GetSamples()); }
void SettingsTabWnd::OnApplyInterval() { PostToMain(WM_APP_CMD_SET_INTERVAL,  GetIntervalMs()); }
void SettingsTabWnd::OnApplyData()     { PostToMain(WM_APP_CMD_SET_DATA_MASK, GetDataMask()); }
void SettingsTabWnd::OnSetAmplitude()  { PostToMain(WM_APP_CMD_SET_AMPLITUDE, GetAmplitude()); }
void SettingsTabWnd::OnSetBurst()      { PostToMain(WM_APP_CMD_SET_BURST,     GetBurst()); }
void SettingsTabWnd::OnSetRamp()
{
    uint16_t r = 0, f = 0;
    GetRampUs(r, f);
    m_rampMode = (r != 0 && f != 0);
    UpdatePreview();
    PostToMain(WM_APP_CMD_SET_RAMP, static_cast<WPARAM>(r) | (static_cast<WPARAM>(f) << 16));
}
void SettingsTabWnd::OnPing()          { PostToMain(WM_APP_CMD_PING); }
void SettingsTabWnd::OnGetStatus()     { PostToMain(WM_APP_CMD_GET_STATUS); }

void SettingsTabWnd::OnPcModeChanged()
{
    PostToMain(WM_APP_ACQ_MODE, IsPcRawMode() ? 0 : 1, 0);
}

/* ---- ADC clock calibration ---- */

CString SettingsTabWnd::IniPath()
{
    TCHAR base[MAX_PATH] = {};
    if (FAILED(::SHGetFolderPath(nullptr, CSIDL_APPDATA, nullptr, 0, base)))
        return CString();
    CString dir = CString(base) + _T("\\FastAcq");
    ::CreateDirectory(dir, nullptr);   // no-op if it already exists
    return dir + _T("\\FastAcq.ini");
}

void SettingsTabWnd::LoadFsPpm()
{
    m_fsPpm = 0.0;
    const CString ini = IniPath();
    if (!ini.IsEmpty()) {
        TCHAR buf[64] = {};
        ::GetPrivateProfileString(_T("Calibration"), _T("FsPpm"), _T("0"), buf, 64, ini);
        m_fsPpm = _tstof(buf);
        if (!(m_fsPpm > -1000.0 && m_fsPpm < 1000.0)) m_fsPpm = 0.0;
    }
    CString t; t.Format(_T("%.3f"), m_fsPpm);
    m_edtPpm.SetWindowText(t);
}

void SettingsTabWnd::SaveFsPpm() const
{
    const CString ini = IniPath();
    if (ini.IsEmpty()) return;
    CString t; t.Format(_T("%.3f"), m_fsPpm);
    ::WritePrivateProfileString(_T("Calibration"), _T("FsPpm"), t, ini);
}

void SettingsTabWnd::OnApplyPpm()
{
    CString s; m_edtPpm.GetWindowText(s);
    s.Replace(_T(','), _T('.'));   // accept a decimal comma
    double v = _tstof(s);
    if (v < -1000.0) v = -1000.0;  // a crystal is never off by more than this
    if (v >  1000.0) v =  1000.0;
    m_fsPpm = v;
    CString t; t.Format(_T("%.3f"), m_fsPpm);
    m_edtPpm.SetWindowText(t);
    SaveFsPpm();
    PostToMain(WM_APP_FS_PPM);
}
