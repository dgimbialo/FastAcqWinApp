#include "pch.h"
#include "Lang.h"
#include "SettingsTab.h"
#include "AppMessages.h"
#include "Dpi.h"
#include "ProtocolDefs.h"
#include "Theme.h"
#include "resource.h"

BEGIN_MESSAGE_MAP(SettingsTab, CWnd)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_ERASEBKGND()
    ON_WM_CTLCOLOR()
    ON_BN_CLICKED(IDC_BTN_APPLY_MODE,  &SettingsTab::OnApplyMode)
    ON_BN_CLICKED(IDC_BTN_SET_FREQ,    &SettingsTab::OnSetFreq)
    ON_BN_CLICKED(IDC_BTN_SET_SAMPLES, &SettingsTab::OnSetSamples)
    ON_BN_CLICKED(IDC_BTN_APPLY_INT,   &SettingsTab::OnApplyInterval)
    ON_BN_CLICKED(IDC_BTN_APPLY_DATA,  &SettingsTab::OnApplyData)
    ON_BN_CLICKED(IDC_BTN_SET_AMP,     &SettingsTab::OnSetAmplitude)
    ON_BN_CLICKED(IDC_BTN_SET_OFFSET,  &SettingsTab::OnSetOffset)
    ON_CONTROL_RANGE(EN_CHANGE, IDC_EDT_OFFSET, IDC_EDT_OFFSET, &SettingsTab::OnPreviewInput)
    ON_BN_CLICKED(IDC_BTN_SET_BURST,   &SettingsTab::OnSetBurst)
    ON_BN_CLICKED(IDC_BTN_SET_RAMP,    &SettingsTab::OnSetRamp)
    ON_BN_CLICKED(IDC_BTN_APPLY_PPM,   &SettingsTab::OnApplyPpm)
    ON_EN_CHANGE(IDC_EDT_FREQ,         &SettingsTab::OnFreqChanged)
    ON_EN_CHANGE(IDC_EDT_RISE,         &SettingsTab::OnRampChanged)
    ON_EN_CHANGE(IDC_EDT_FALL,         &SettingsTab::OnRampChanged)
    ON_CBN_SELCHANGE(IDC_CMB_MODE,     &SettingsTab::OnModeSelChanged)
    ON_CBN_SELCHANGE(IDC_CMB_LANG,     &SettingsTab::OnLangChanged)
    ON_BN_CLICKED(IDC_CHK_VCO,         &SettingsTab::OnAutoApply)
    ON_CONTROL_RANGE(EN_KILLFOCUS, IDC_EDT_VTUNE_LO, IDC_EDT_VCO_CURVE, &SettingsTab::OnEditKillFocus)
    ON_CONTROL_RANGE(EN_CHANGE, IDC_EDT_SAMPLES, IDC_EDT_BURST, &SettingsTab::OnPreviewInput)
    ON_BN_CLICKED(IDC_BTN_PING,        &SettingsTab::OnPing)
    ON_BN_CLICKED(IDC_BTN_GET_STATUS,  &SettingsTab::OnGetStatus)
    ON_BN_CLICKED(IDC_BTN_SEND_ALL,    &SettingsTab::OnSendAll)
    ON_CONTROL_RANGE(CBN_SELCHANGE, IDC_EDT_F0, IDC_EDT_PAIR_V, &SettingsTab::OnAutoApplyRange)
    ON_CONTROL_RANGE(BN_CLICKED,    IDC_EDT_F0, IDC_EDT_PAIR_V, &SettingsTab::OnAutoApplyRange)
    ON_CONTROL_RANGE(BN_CLICKED,    IDC_CHK_DARK, IDC_CHK_AUTOCONNECT, &SettingsTab::OnAutoApplyRange)
    ON_CONTROL_RANGE(EN_KILLFOCUS,  IDC_EDT_FREQ, IDC_EDT_PAIR_V, &SettingsTab::OnEditKillFocus)
    ON_CONTROL_RANGE(EN_KILLFOCUS,  IDC_EDT_FSCAL, IDC_EDT_FSCAL, &SettingsTab::OnEditKillFocus)
END_MESSAGE_MAP()

// ON_CONTROL_RANGE handlers take the control id.
void SettingsTab::OnAutoApplyRange(UINT) { OnAutoApply(); }

BOOL SettingsTab::CreateTab(CWnd* parent, UINT id)
{
    LPCTSTR cls = AfxRegisterWndClass(0, ::LoadCursor(nullptr, IDC_ARROW), nullptr, nullptr);
    return Create(cls, nullptr, WS_CHILD | WS_CLIPCHILDREN, CRect(0, 0, 10, 10), parent, id);
}

int SettingsTab::OnCreate(LPCREATESTRUCT lpcs)
{
    if (CWnd::OnCreate(lpcs) == -1) return -1;
    m_bgBrush.CreateSolidBrush(Theme::Get().bg);
    Dpi::MakeFont(m_font, m_hWnd, 9);
    Dpi::MakeFont(m_hdrFont, m_hWnd, 10, true);

    const DWORD bs  = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON;
    const DWORD es  = WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_NUMBER;
    const DWORD esf = WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL;     // allows '.', '-', 'e'
    const DWORD ss  = WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE;
    const DWORD chk = WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX;
    const DWORD cs  = WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL;
    CRect rc(0, 0, 100, 22);

    // --- MCU acquisition
    m_hdrMcu.Create(TR("MCU acquisition"), ss, rc, this);
    m_lblMode.Create(TR("Mode"), ss, rc, this);
    m_cmbMode.Create(cs, rc, this, IDC_CMB_MODE);
    m_cmbMode.AddString(TR("Idle")); m_cmbMode.AddString(TR("Continuous")); m_cmbMode.AddString(TR("Single"));
    m_cmbMode.SetCurSel(1);
    m_btnApplyMode.Create(TR("Apply"), bs, rc, this, IDC_BTN_APPLY_MODE);
    m_lblFreq.Create(TR("Chirp freq, Hz (100..24000)"), ss, rc, this);
    m_edtFreq.Create(es, rc, this, IDC_EDT_FREQ);
    m_btnSetFreq.Create(TR("Set"), bs, rc, this, IDC_BTN_SET_FREQ);
    m_lblSamples.Create(TR("Samples (0 = auto)"), ss, rc, this);
    m_edtSamples.Create(es, rc, this, IDC_EDT_SAMPLES);
    m_btnSetSamples.Create(TR("Set"), bs, rc, this, IDC_BTN_SET_SAMPLES);
    m_lblInterval.Create(TR("Interval, ms"), ss, rc, this);
    m_edtInterval.Create(es, rc, this, IDC_EDT_INTERVAL);
    m_btnApplyInterval.Create(TR("Set"), bs, rc, this, IDC_BTN_APPLY_INT);
    m_lblAmplitude.Create(TR("Chirp amplitude, DAC"), ss, rc, this);
    m_edtAmplitude.Create(es, rc, this, IDC_EDT_AMPLITUDE);
    m_btnSetAmp.Create(TR("Set"), bs, rc, this, IDC_BTN_SET_AMP);
    m_lblOffset.Create(TR("Chirp offset (base), DAC"), ss, rc, this);
    m_edtOffset.Create(es, rc, this, IDC_EDT_OFFSET);
    m_btnSetOffset.Create(TR("Set"), bs, rc, this, IDC_BTN_SET_OFFSET);
    m_lblBurst.Create(TR("Chirps per capture"), ss, rc, this);
    m_edtBurst.Create(es, rc, this, IDC_EDT_BURST);
    m_btnSetBurst.Create(TR("Set"), bs, rc, this, IDC_BTN_SET_BURST);
    m_lblRamp.Create(TR("Ramp rise / fall, us"), ss, rc, this);
    m_edtRise.Create(es, rc, this, IDC_EDT_RISE);
    m_edtFall.Create(es, rc, this, IDC_EDT_FALL);
    m_btnSetRamp.Create(TR("Set ramp"), bs, rc, this, IDC_BTN_SET_RAMP);
    m_lblData.Create(TR("Frame content"), ss, rc, this);
    m_chkRaw.Create(TR("Raw"), chk, rc, this, IDC_CHK_RAW);
    m_chkFft.Create(TR("FFT"), chk, rc, this, IDC_CHK_FFT);
    m_btnApplyData.Create(TR("Set"), bs, rc, this, IDC_BTN_APPLY_DATA);
    m_btnPing.Create(TR("Ping"), bs, rc, this, IDC_BTN_PING);
    m_btnGetStatus.Create(TR("Get status"), bs, rc, this, IDC_BTN_GET_STATUS);
    m_btnSendAll.Create(TR("Send all"), bs, rc, this, IDC_BTN_SEND_ALL);

    // --- Radar
    m_hdrRadar.Create(TR("Radar geometry"), ss, rc, this);
    m_lblVco.Create(TR("VCO (HMC431)"), ss, rc, this);
    m_chkVco.Create(TR("f0 / B from tuning curve"), chk, rc, this, IDC_CHK_VCO);
    m_lblVtune.Create(TR("Vtune at DAC 0 / 4095, V"), ss, rc, this);
    m_edtVtuneLo.Create(esf, rc, this, IDC_EDT_VTUNE_LO);
    m_edtVtuneHi.Create(esf, rc, this, IDC_EDT_VTUNE_HI);
    m_lblCurve.Create(TR("Tuning curve V:GHz,..."), ss, rc, this);
    m_edtCurve.Create(esf, rc, this, IDC_EDT_VCO_CURVE);
    m_lblF0.Create(TR("Carrier f0 (sweep centre), GHz"), ss, rc, this);          m_edtF0.Create(esf, rc, this, IDC_EDT_F0);
    m_lblBw.Create(TR("Sweep bandwidth B, MHz"), ss, rc, this);   m_edtBw.Create(esf, rc, this, IDC_EDT_BW);
    m_lblTramp.Create(TR("Ramp time, ms (0 = auto)"), ss, rc, this); m_edtTramp.Create(esf, rc, this, IDC_EDT_TRAMP);
    m_lblRoff.Create(TR("Range offset, m"), ss, rc, this);        m_edtRoff.Create(esf, rc, this, IDC_EDT_ROFFSET);
    m_lblShape.Create(TR("Modulation"), ss, rc, this);
    m_cmbShape.Create(cs, rc, this, IDC_CMB_SHAPE);
    for (int i = 0; i < static_cast<int>(dsp::RampShape::Count); ++i)
        m_cmbShape.AddString(Lang::Tr(CString(dsp::RampShapeName(static_cast<dsp::RampShape>(i)))));
    m_lblChirps.Create(TR("Chirps per frame"), ss, rc, this);
    m_chkChirpsAuto.Create(TR("= burst"), chk, rc, this, IDC_CHK_CHIRPS_AUTO);
    m_edtChirps.Create(es, rc, this, IDC_EDT_CHIRPS);
    m_lblPairV.Create(TR("UP/DOWN pair gate, m/s"), ss, rc, this); m_edtPairV.Create(esf, rc, this, IDC_EDT_PAIR_V);

    // --- Application
    m_hdrDisplay.Create(TR("Application"), ss, rc, this);
    m_lblDark.Create(TR("Theme"), ss, rc, this);                  m_chkDark.Create(TR("Dark"), chk, rc, this, IDC_CHK_DARK);
    m_lblLang.Create(TR("Language"), ss, rc, this);
    m_cmbLang.Create(cs, rc, this, IDC_CMB_LANG);
    for (int i = 0; i < static_cast<int>(Lang::Id::Count); ++i) m_cmbLang.AddString(Lang::Name(static_cast<Lang::Id>(i)));
    m_lblFsCal.Create(TR("Fallback Fs, Hz"), ss, rc, this);       m_edtFsCal.Create(es, rc, this, IDC_EDT_FSCAL);
    m_lblPpm.Create(TR("ADC clock corr., ppm"), ss, rc, this);     m_edtPpm.Create(esf, rc, this, IDC_EDT_PPM);
    m_btnApplyPpm.Create(TR("Apply"), bs, rc, this, IDC_BTN_APPLY_PPM);
    m_lblVerbose.Create(TR("Log"), ss, rc, this);                 m_chkVerbose.Create(TR("Per-frame RX lines"), chk, rc, this, IDC_CHK_VERBOSE);
    m_lblAutoConn.Create(TR("Start-up"), ss, rc, this);           m_chkAutoConnect.Create(TR("Auto-connect"), chk, rc, this, IDC_CHK_AUTOCONNECT);

    m_hdrDerived.Create(TR("Derived values"), ss, rc, this);
    m_lblDerived.Create(_T(""), WS_CHILD | WS_VISIBLE | SS_LEFT, rc, this, IDC_LBL_DERIVED);
    m_preview.CreateCtrl(this, IDC_CHIRP_PREVIEW);

    // Fonts.
    CWnd* pw = GetWindow(GW_CHILD);
    while (pw) { pw->SetFont(&m_font); pw = pw->GetWindow(GW_HWNDNEXT); }
    for (CStatic* h : { &m_hdrMcu, &m_hdrRadar, &m_hdrDisplay, &m_hdrDerived })
        h->SetFont(&m_hdrFont);

    AppSettings defaults;
    ApplySettings(defaults);
    SetConnected(false);
    m_tips.Attach(this);
    return 0;
}

BOOL SettingsTab::PreTranslateMessage(MSG* pMsg)
{
    m_tips.Relay(pMsg);
    return CWnd::PreTranslateMessage(pMsg);
}

void SettingsTab::OnLangChanged()
{
    if (m_suppress) return;
    const int sel = m_cmbLang.GetCurSel();
    if (sel >= 0 && sel != m_last.language)
        AfxMessageBox(TR("The language changes after the application is restarted."), MB_ICONINFORMATION);
    NotifyChanged();
}

void SettingsTab::ApplyTheme()
{
    if (m_bgBrush.GetSafeHandle()) m_bgBrush.DeleteObject();
    m_bgBrush.CreateSolidBrush(Theme::Get().bg);
    m_preview.ApplyTheme();
    Invalidate();
    CWnd* pw = GetWindow(GW_CHILD);
    while (pw) { pw->Invalidate(); pw = pw->GetWindow(GW_HWNDNEXT); }
}

// ---------------------------------------------------------------------------
int SettingsTab::GetInt(const CEdit& e, int def)
{
    CString s; const_cast<CEdit&>(e).GetWindowText(s); s.Trim();
    return s.IsEmpty() ? def : _ttoi(s);
}

double SettingsTab::GetDouble(const CEdit& e, double def)
{
    CString s; const_cast<CEdit&>(e).GetWindowText(s); s.Trim();
    if (s.IsEmpty()) return def;
    s.Replace(_T(','), _T('.'));
    return _tcstod(s, nullptr);
}

void SettingsTab::SetInt(CEdit& e, long long v) { CString s; s.Format(_T("%lld"), v); e.SetWindowText(s); }
void SettingsTab::SetDouble(CEdit& e, double v, LPCTSTR fmt) { CString s; s.Format(fmt, v); e.SetWindowText(s); }

void SettingsTab::ApplySettings(const AppSettings& s)
{
    m_last = s;
    m_suppress = true;
    m_cmbMode.SetCurSel(s.acq.mode >= 0 && s.acq.mode <= 2 ? s.acq.mode : 1);
    SetInt(m_edtFreq, s.acq.chirpFreqHz);
    SetInt(m_edtSamples, s.acq.samples);
    SetInt(m_edtInterval, s.acq.intervalMs);
    SetInt(m_edtAmplitude, s.acq.amplitude);
    SetInt(m_edtOffset, s.acq.offset);
    SetInt(m_edtBurst, s.acq.burst);
    m_rampMode = (s.acq.riseUs > 0 && s.acq.fallUs > 0);
    if (m_rampMode) {
        SetInt(m_edtRise, s.acq.riseUs);
        SetInt(m_edtFall, s.acq.fallUs);
    } else {
        const int half = s.acq.chirpFreqHz > 0 ? static_cast<int>(500000.0 / s.acq.chirpFreqHz + 0.5) : 0;
        SetInt(m_edtRise, half);
        SetInt(m_edtFall, half);
    }
    m_chkRaw.SetCheck(s.acq.sendRaw ? BST_CHECKED : BST_UNCHECKED);
    m_chkFft.SetCheck(s.acq.sendFft ? BST_CHECKED : BST_UNCHECKED);

    m_chkVco.SetCheck(s.vco.useCurve ? BST_CHECKED : BST_UNCHECKED);
    SetDouble(m_edtVtuneLo, s.vco.vtuneAtDac0V, _T("%.3f"));
    SetDouble(m_edtVtuneHi, s.vco.vtuneAtDacFullV, _T("%.3f"));
    m_edtCurve.SetWindowText(CString(s.vco.Curve().Format().c_str()));
    m_edtF0.SetReadOnly(s.vco.useCurve ? TRUE : FALSE);
    m_edtBw.SetReadOnly(s.vco.useCurve ? TRUE : FALSE);
    SetDouble(m_edtF0, s.radar.f0Hz / 1e9, _T("%.4f"));
    SetDouble(m_edtBw, s.radar.bandwidthHz / 1e6, _T("%.3f"));
    SetDouble(m_edtTramp, s.radar.rampSec * 1e3, _T("%.4f"));
    SetDouble(m_edtRoff, s.radar.rangeOffsetM, _T("%.3f"));
    m_cmbShape.SetCurSel(static_cast<int>(s.dsp.shape));
    m_chkChirpsAuto.SetCheck(s.chirpsFromBurst ? BST_CHECKED : BST_UNCHECKED);
    SetInt(m_edtChirps, s.dsp.chirpsInFrame);
    m_edtChirps.EnableWindow(!s.chirpsFromBurst);
    SetDouble(m_edtPairV, s.radar.pairMaxVelocityMps, _T("%.1f"));

    m_chkDark.SetCheck(s.display.darkTheme ? BST_CHECKED : BST_UNCHECKED);
    m_cmbLang.SetCurSel(s.language);
    SetInt(m_edtFsCal, s.sampleRateCalHz);
    SetDouble(m_edtPpm, s.fsPpm, _T("%.3f"));
    m_chkVerbose.SetCheck(s.verboseLog ? BST_CHECKED : BST_UNCHECKED);
    m_chkAutoConnect.SetCheck(s.autoConnect ? BST_CHECKED : BST_UNCHECKED);

    m_suppress = false;
    RefreshDerived();
}

void SettingsTab::ReadInto(AppSettings& s) const
{
    s.acq.mode        = m_cmbMode.GetCurSel() < 0 ? 1 : m_cmbMode.GetCurSel();
    s.acq.chirpFreqHz = GetFreqHz();
    s.acq.samples     = static_cast<int>(GetSamples());
    s.acq.intervalMs  = GetIntervalMs();
    s.acq.amplitude   = GetAmplitude();
    s.acq.offset      = GetOffset();
    s.acq.burst       = GetBurst();
    s.acq.riseUs      = GetRiseUs();
    s.acq.fallUs      = GetFallUs();
    s.acq.sendRaw     = m_chkRaw.GetCheck() == BST_CHECKED;
    s.acq.sendFft     = m_chkFft.GetCheck() == BST_CHECKED;

    s.radar.f0Hz        = GetDouble(m_edtF0, 5.5) * 1e9;
    s.radar.bandwidthHz = GetDouble(m_edtBw, 1000.0) * 1e6;
    s.vco.useCurve        = m_chkVco.GetCheck() == BST_CHECKED;
    s.vco.vtuneAtDac0V    = GetDouble(m_edtVtuneLo, 0.0);
    s.vco.vtuneAtDacFullV = GetDouble(m_edtVtuneHi, 10.0);
    if (!(s.vco.vtuneAtDacFullV > s.vco.vtuneAtDac0V)) s.vco.vtuneAtDacFullV = s.vco.vtuneAtDac0V + 10.0;
    {
        CString ct; const_cast<CEdit&>(m_edtCurve).GetWindowText(ct);
        core::VcoCurve c;
        s.vco.curveText = c.Parse(std::string(CStringA(ct))) ? c.Format() : std::string();
    }
    s.ApplyVcoToRadar();
    s.radar.rampSec     = GetDouble(m_edtTramp, 0.0) * 1e-3;
    s.radar.rangeOffsetM= GetDouble(m_edtRoff, 0.0);
    s.radar.pairMaxVelocityMps = GetDouble(m_edtPairV, 30.0);
    int shape = m_cmbShape.GetCurSel();
    if (shape >= 0 && shape < static_cast<int>(dsp::RampShape::Count)) s.dsp.shape = static_cast<dsp::RampShape>(shape);
    s.chirpsFromBurst = m_chkChirpsAuto.GetCheck() == BST_CHECKED;
    s.dsp.chirpsInFrame = s.chirpsFromBurst ? s.acq.burst : (std::max)(1, GetInt(m_edtChirps, 1));

    s.display.darkTheme = m_chkDark.GetCheck() == BST_CHECKED;
    if (m_cmbLang.GetCurSel() >= 0) s.language = m_cmbLang.GetCurSel();
    int fs = GetInt(m_edtFsCal, 60058600);
    s.sampleRateCalHz   = fs > 1000 ? static_cast<uint32_t>(fs) : 60058600u;
    s.fsPpm             = GetFsPpm();
    s.verboseLog        = m_chkVerbose.GetCheck() == BST_CHECKED;
    s.autoConnect       = m_chkAutoConnect.GetCheck() == BST_CHECKED;

}

void SettingsTab::SetObserved(double fsHz, size_t samplesPerFrame, size_t mcuFftSize)
{
    m_obsFs = fsHz; m_obsSamples = samplesPerFrame; m_obsMcuFft = mcuFftSize;
    RefreshDerived();
}

void SettingsTab::RefreshDerived()
{
    if (!m_lblDerived.GetSafeHwnd()) return;
    AppSettings s = m_last;
    ReadInto(s);
    const double fs = (m_obsFs > 0.0) ? m_obsFs : static_cast<double>(s.sampleRateCalHz);
    size_t samples = m_obsSamples;
    if (samples == 0) {
        const core::ChirpGeometry& g = m_preview.Geometry();
        if (s.acq.samples > 0) samples = static_cast<size_t>(s.acq.samples);
        else if (g.valid) samples = g.samplesPerBurst;
        else if (s.acq.chirpFreqHz > 0) samples = static_cast<size_t>(fs / s.acq.chirpFreqHz + 0.5) * static_cast<size_t>((std::max)(1, s.acq.burst));
    }
    dsp::DerivedValues d = dsp::ComputeDerived(s.radar, s.dsp, fs, s.acq.chirpFreqHz, samples,
                                               s.acq.intervalMs, s.acq.sendRaw, s.acq.sendFft, m_obsMcuFft);
    CString t, line;
    UpdateVcoDerived(s);
    // One block per topic: a short caption line, then indented detail lines.
    const CString ind = _T("      ");
    {
        const core::VcoSweep sw = s.vco.SweepFor(s.acq.offset, s.acq.amplitude);
        t += TR("VCO sweep") + _T("\r\n");
        if (sw.valid) {
            line.Format(TR("DAC %d..%d = Vtune %.2f..%.2f V = %.4f..%.4f GHz"), s.acq.offset,
                        (std::min)(4095, s.acq.offset + s.acq.amplitude), sw.vLowV, sw.vHighV, sw.fStartHz / 1e9, sw.fStopHz / 1e9);
            t += ind + line + _T("\r\n");
            line.Format(TR("B = %.1f MHz, slope %.0f..%.0f MHz/V, sweep nonlinearity %.1f%%"),
                        sw.bandwidthHz / 1e6, sw.sensMinHzPerV / 1e6, sw.sensMaxHzPerV / 1e6, sw.nonlinearityPct);
            t += ind + line;
            if (sw.outOfTable) t += TR(" [Vtune outside the curve table!]");
            t += _T("\r\n");
        } else {
            t += ind + TR("tuning curve invalid (need >= 2 points V:GHz)") + _T("\r\n");
        }
    }
    t += _T("\r\n") + TR("Chirp") + _T("\r\n");
    line.Format(TR("ramp %.4f ms, period %.4f ms"), d.rampSec * 1e3, d.periodSec * 1e3); t += ind + line + _T("\r\n");
    line.Format(TR("%zu samples per ramp, %zu of them go into the FFT"), d.samplesPerRamp, d.samplesUsed); t += ind + line + _T("\r\n");

    t += _T("\r\n") + TR("Spectrum") + _T("\r\n");
    line.Format(TR("decimation x%d: effective Fs %.1f kHz, FFT %zu points"), d.decimation, d.fsEffHz / 1e3, d.fftSize); t += ind + line + _T("\r\n");
    line.Format(TR("one bin = %.1f Hz"), d.binHz); t += ind + line;
    if (d.rangeBinM > 0.0) { line.Format(TR(" = %.3f m"), d.rangeBinM); t += line; }
    t += _T("\r\n");

    t += _T("\r\n") + TR("Range") + _T("\r\n");
    if (d.rangeResM > 0.0) {
        line.Format(TR("resolution c/2B = %.3f m (effective %.3f m)"), d.rangeResM, d.rangeResEffM); t += ind + line + _T("\r\n");
        line.Format(TR("maximum %.1f m, beat frequency %.1f Hz per metre"), d.rangeMaxM, d.beatPerMeterHz); t += ind + line + _T("\r\n");
    } else {
        t += ind + TR("set bandwidth B > 0 for a range axis") + _T("\r\n");
    }
    if (d.velResMps > 0.0) {
        t += _T("\r\n") + TR("Velocity") + _T("\r\n");
        line.Format(TR("resolution %.3f m/s (burst %d), unambiguous +/- %.2f m/s"), d.velResMps, (std::max)(1, s.dsp.chirpsInFrame), d.velMaxMps);
        t += ind + line + _T("\r\n");
    }
    t += _T("\r\n") + TR("USB link") + _T("\r\n");
    line.Format(TR("%.2f MB/s at %d ms interval (full-speed CDC limit ~0.8 MB/s)"), d.usbMBps, s.acq.intervalMs); t += ind + line;
    if (s.fsPpm != 0.0) {
        line.Format(TR("\r\n%sADC clock %+.3f ppm: %.6f -> %.6f MS/s"), ind.GetString(), s.fsPpm, fs / 1e6, fs * (1.0 + s.fsPpm * 1e-6) / 1e6);
        t += line;
    }
    CString prev; m_lblDerived.GetWindowText(prev);
    int nPrev = 0, nNew = 0;
    for (int i = 0; i < prev.GetLength(); ++i) if (prev[i] == _T('\n')) ++nPrev;
    for (int i = 0; i < t.GetLength(); ++i) if (t[i] == _T('\n')) ++nNew;
    m_lblDerived.SetWindowText(t);
    if (nPrev != nNew) Relayout();   // the read-out height follows its line count
}

void SettingsTab::UpdateVcoDerived(AppSettings& s)
{
    if (!s.vco.useCurve) return;
    const bool was = m_suppress; m_suppress = true;
    SetDouble(m_edtF0, s.radar.f0Hz / 1e9, _T("%.4f"));
    SetDouble(m_edtBw, s.radar.bandwidthHz / 1e6, _T("%.3f"));
    m_edtF0.SetReadOnly(TRUE);
    m_edtBw.SetReadOnly(TRUE);
    m_suppress = was;
}

void SettingsTab::UpdatePreview()
{
    if (!m_preview.GetSafeHwnd()) return;
    {
        AppSettings s = m_last; ReadInto(s);
        m_preview.SetVco(s.vco.vtuneAtDac0V, s.vco.vtuneAtDacFullV, s.vco.useCurve ? s.vco.Curve() : core::VcoCurve());
    }
    core::ChirpParams p;
    p.freqHz     = GetFreqHz();
    p.riseUs     = GetRiseUs();
    p.fallUs     = GetFallUs();
    p.amplitude  = GetAmplitude();
    p.offset     = GetOffset();
    p.burst      = GetBurst();
    p.intervalMs = GetIntervalMs();
    p.samplesOvr = GetSamples();
    p.mode       = GetModeSel();
    m_preview.SetParams(p);
}

// Frequency typed: the ramp fields mirror the symmetric chirp the MCU will
// generate from CMD_START_CHIRP (and the ramp mode is left).
void SettingsTab::OnFreqChanged()
{
    if (m_suppress || m_syncing) return;
    m_syncing = true;
    m_rampMode = false;
    const int f = GetFreqHz();
    const int half = f > 0 ? static_cast<int>(500000.0 / f + 0.5) : 0;
    SetInt(m_edtRise, half);
    SetInt(m_edtFall, half);
    m_syncing = false;
    UpdatePreview();
    RefreshDerived();
}

// Rise / fall typed: the frequency field shows the resulting repetition
// rate and "Set ramp" becomes the way to program the device.
void SettingsTab::OnRampChanged()
{
    if (m_suppress || m_syncing) return;
    m_syncing = true;
    const int rise = GetInt(m_edtRise, 0), fall = GetInt(m_edtFall, 0);
    m_rampMode = (rise > 0 && fall > 0);
    if (m_rampMode) {
        const double f = 1e6 / static_cast<double>(rise + fall);
        SetInt(m_edtFreq, static_cast<long long>(f + 0.5));
    }
    m_syncing = false;
    UpdatePreview();
    RefreshDerived();
}

void SettingsTab::OnPreviewInput(UINT)   { if (!m_suppress) { UpdatePreview(); RefreshDerived(); } }
void SettingsTab::OnModeSelChanged()     { if (!m_suppress) UpdatePreview(); }

void SettingsTab::SetConnected(bool c)
{
    m_connected = c;
    for (CButton* b : { &m_btnApplyMode, &m_btnSetFreq, &m_btnSetSamples, &m_btnApplyInterval, &m_btnSetAmp, &m_btnSetOffset,
                        &m_btnSetBurst, &m_btnSetRamp, &m_btnApplyData, &m_btnPing, &m_btnGetStatus, &m_btnSendAll })
        b->EnableWindow(c);
}

// ---------------------------------------------------------------------------
uint16_t SettingsTab::GetFreqHz() const
{
    int v = GetInt(m_edtFreq, 458);
    if (v < static_cast<int>(CHIRP_FREQ_MIN_HZ)) v = CHIRP_FREQ_MIN_HZ;
    if (v > static_cast<int>(CHIRP_FREQ_MAX_HZ)) v = CHIRP_FREQ_MAX_HZ;
    return static_cast<uint16_t>(v);
}

uint32_t SettingsTab::GetSamples() const
{
    int v = GetInt(m_edtSamples, 0);
    if (v < 0) v = 0;
    if (v > static_cast<int>(SAMPLES_MAX)) v = SAMPLES_MAX;
    return static_cast<uint32_t>(v);
}

uint16_t SettingsTab::GetModeSel() const
{
    int sel = m_cmbMode.GetCurSel();
    if (sel < 0) sel = 0;
    if (sel > 2) sel = 2;
    return static_cast<uint16_t>(sel);
}

uint8_t SettingsTab::GetDataMask() const
{
    uint8_t m = 0;
    if (m_chkRaw.GetCheck() == BST_CHECKED) m |= 0x01;
    if (m_chkFft.GetCheck() == BST_CHECKED) m |= 0x02;
    return m;
}

uint16_t SettingsTab::GetIntervalMs() const
{
    int v = GetInt(m_edtInterval, 30);
    if (v < static_cast<int>(INTERVAL_MIN_MS)) v = INTERVAL_MIN_MS;
    if (v > static_cast<int>(INTERVAL_MAX_MS)) v = INTERVAL_MAX_MS;
    return static_cast<uint16_t>(v);
}

uint16_t SettingsTab::GetAmplitude() const
{
    int v = GetInt(m_edtAmplitude, 4095);
    if (v < 1) v = 1;
    if (v > static_cast<int>(AMPLITUDE_MAX)) v = AMPLITUDE_MAX;
    return static_cast<uint16_t>(v);
}

uint16_t SettingsTab::GetOffset() const
{
    int v = GetInt(m_edtOffset, 0);
    if (v < 0) v = 0;
    if (v > static_cast<int>(AMPLITUDE_MAX) - 1) v = AMPLITUDE_MAX - 1;
    return static_cast<uint16_t>(v);
}

uint16_t SettingsTab::GetBurst() const
{
    int v = GetInt(m_edtBurst, 1);
    if (v < 1) v = 1;
    if (v > static_cast<int>(BURST_MAX)) v = BURST_MAX;
    return static_cast<uint16_t>(v);
}

uint16_t SettingsTab::GetRiseUs() const
{
    if (!m_rampMode) return 0;
    int v = GetInt(m_edtRise, 0);
    if (v < 0) v = 0;
    if (v > 65535) v = 65535;
    return static_cast<uint16_t>(v);
}

uint16_t SettingsTab::GetFallUs() const
{
    if (!m_rampMode) return 0;
    int v = GetInt(m_edtFall, 0);
    if (v < 0) v = 0;
    if (v > 65535) v = 65535;
    return static_cast<uint16_t>(v);
}

double SettingsTab::GetFsPpm() const
{
    double v = GetDouble(m_edtPpm, 0.0);
    if (!(v > -100000.0 && v < 100000.0)) v = 0.0;
    return v;
}

// ---------------------------------------------------------------------------
void SettingsTab::PostToMain(UINT msg, WPARAM wp, LPARAM lp)
{
    CWnd* pMain = AfxGetMainWnd();
    if (pMain && ::IsWindow(pMain->GetSafeHwnd())) pMain->PostMessage(msg, wp, lp);
}

void SettingsTab::NotifyChanged()
{
    if (m_suppress) return;
    m_edtChirps.EnableWindow(m_chkChirpsAuto.GetCheck() != BST_CHECKED);
    UpdatePreview();
    RefreshDerived();
    PostToMain(WM_APP_SETTINGS_CHANGED, SETTINGS_FROM_TAB);
}

void SettingsTab::OnApplyMode()     { PostToMain(WM_APP_CMD_SET_MODE,      GetModeSel()); NotifyChanged(); }
void SettingsTab::OnSetFreq()       { PostToMain(WM_APP_CMD_SET_FREQ,      GetFreqHz()); NotifyChanged(); }
void SettingsTab::OnSetSamples()    { PostToMain(WM_APP_CMD_SET_SAMPLES,   GetSamples()); NotifyChanged(); }
void SettingsTab::OnApplyInterval() { PostToMain(WM_APP_CMD_SET_INTERVAL,  GetIntervalMs()); NotifyChanged(); }
void SettingsTab::OnApplyData()     { PostToMain(WM_APP_CMD_SET_DATA_MASK, GetDataMask()); NotifyChanged(); }
void SettingsTab::OnSetAmplitude()  { PostToMain(WM_APP_CMD_SET_AMPLITUDE, GetAmplitude()); NotifyChanged(); }
void SettingsTab::OnSetOffset()     { PostToMain(WM_APP_CMD_SET_OFFSET,    GetOffset()); NotifyChanged(); }
void SettingsTab::OnSetBurst()      { PostToMain(WM_APP_CMD_SET_BURST,     GetBurst()); NotifyChanged(); }
void SettingsTab::OnApplyPpm()      { NotifyChanged(); }

void SettingsTab::OnSetRamp()
{
    // Explicit rise/fall: CMD_SET_RAMP. Both fields 0 (or ramp mode off) fall
    // back to the symmetric chirp defined by the frequency (CMD_START_CHIRP).
    const int rise = GetInt(m_edtRise, 0), fall = GetInt(m_edtFall, 0);
    m_rampMode = (rise > 0 && fall > 0);
    if (m_rampMode) {
        const WPARAM wp = static_cast<WPARAM>(GetRiseUs()) | (static_cast<WPARAM>(GetFallUs()) << 16);
        PostToMain(WM_APP_CMD_SET_RAMP, wp);
    } else {
        PostToMain(WM_APP_CMD_SET_FREQ, GetFreqHz());
    }
    NotifyChanged();
}
void SettingsTab::OnPing()          { PostToMain(WM_APP_CMD_PING); }
void SettingsTab::OnGetStatus()     { PostToMain(WM_APP_CMD_GET_STATUS); }

void SettingsTab::OnSendAll()
{
    if (m_rampMode)
        PostToMain(WM_APP_CMD_SET_RAMP, static_cast<WPARAM>(GetRiseUs()) | (static_cast<WPARAM>(GetFallUs()) << 16));
    else
        PostToMain(WM_APP_CMD_SET_FREQ, GetFreqHz());
    PostToMain(WM_APP_CMD_SET_SAMPLES,   GetSamples());
    PostToMain(WM_APP_CMD_SET_INTERVAL,  GetIntervalMs());
    PostToMain(WM_APP_CMD_SET_AMPLITUDE, GetAmplitude());
    PostToMain(WM_APP_CMD_SET_OFFSET,    GetOffset());
    PostToMain(WM_APP_CMD_SET_BURST,     GetBurst());
    PostToMain(WM_APP_CMD_SET_DATA_MASK, GetDataMask());
    PostToMain(WM_APP_CMD_GET_STATUS);
    NotifyChanged();
}

void SettingsTab::OnAutoApply()
{
    const bool vco = m_chkVco.GetCheck() == BST_CHECKED;
    m_edtF0.SetReadOnly(vco ? TRUE : FALSE);
    m_edtBw.SetReadOnly(vco ? TRUE : FALSE);
    NotifyChanged();
}
void SettingsTab::OnEditKillFocus(UINT) { NotifyChanged(); }

// ---------------------------------------------------------------------------
void SettingsTab::OnSize(UINT, int, int) { Relayout(); }

void SettingsTab::Relayout()
{
    if (!m_lblMode.GetSafeHwnd()) return;
    const int S = static_cast<int>(Dpi::Of(m_hWnd));
    auto sc = [&](int px) { return ::MulDiv(px, S, 96); };
    CRect rc; GetClientRect(&rc);

    // Label column and button width follow the longest caption (translations
    // are longer than the English source strings).
    int lblW = sc(150), btnW = sc(60);
    for (CWnd* c = GetWindow(GW_CHILD); c; c = c->GetWindow(GW_HWNDNEXT)) {
        TCHAR cls[32]{}; ::GetClassName(c->GetSafeHwnd(), cls, 31);
        if (_tcsicmp(cls, _T("Static")) == 0) {
            if (c == &m_lblDerived || c == &m_hdrMcu || c == &m_hdrRadar || c == &m_hdrDisplay || c == &m_hdrDerived) continue;
            lblW = (std::max)(lblW, Dpi::FitWidth(*c, 0));
        } else if (_tcsicmp(cls, _T("Button")) == 0 && (::GetWindowLong(c->GetSafeHwnd(), GWL_STYLE) & BS_TYPEMASK) == BS_PUSHBUTTON) {
            if (c == &m_btnPing || c == &m_btnGetStatus || c == &m_btnSendAll) continue;
            btnW = (std::max)(btnW, Dpi::FitWidth(*c, 0));
        }
    }
    lblW = (std::min)(lblW, sc(230));
    const int ctlW = sc(92), pad = sc(6), h = sc(23), rowH = sc(28);
    const int colW = lblW + ctlW + btnW + 3 * pad;
    int colX[4] = { sc(12), sc(12) + colW + sc(16), sc(12) + 2 * (colW + sc(16)), sc(12) + 3 * (colW + sc(16)) };
    // Wide: MCU | Radar + derived | Application, chirp preview under the
    // first two columns. Medium: Application under MCU, preview under Radar.
    // Narrow: everything stacked in one column, preview at the bottom.
    const bool narrow = rc.Width() < colX[2] + colW;
    const bool wide   = rc.Width() >= colX[2] + colW;

    auto header = [&](CStatic& hdr, int col, int& y) {
        hdr.MoveWindow(colX[col], y, colW, sc(20)); y += sc(26);
    };
    auto row = [&](int col, int& y, CStatic& lbl, CWnd& ctl, CWnd* btn = nullptr, int ctlH = 0, int ctlWOverride = 0) {
        const int x = colX[col];
        lbl.MoveWindow(x, y, lblW, h);
        const int cw = ctlWOverride ? ctlWOverride : ctlW;
        ctl.MoveWindow(x + lblW + pad, y, cw, ctlH ? ctlH : h);
        if (btn) btn->MoveWindow(x + lblW + pad + cw + pad, y, btnW, h);
        y += rowH;
    };

    // Column 0: MCU + source.
    int y0 = sc(10);
    header(m_hdrMcu, 0, y0);
    row(0, y0, m_lblMode, m_cmbMode, &m_btnApplyMode, sc(200));
    row(0, y0, m_lblFreq, m_edtFreq, &m_btnSetFreq);
    row(0, y0, m_lblSamples, m_edtSamples, &m_btnSetSamples);
    row(0, y0, m_lblInterval, m_edtInterval, &m_btnApplyInterval);
    row(0, y0, m_lblAmplitude, m_edtAmplitude, &m_btnSetAmp);
    row(0, y0, m_lblOffset, m_edtOffset, &m_btnSetOffset);
    row(0, y0, m_lblBurst, m_edtBurst, &m_btnSetBurst);
    {
        const int x = colX[0];
        m_lblRamp.MoveWindow(x, y0, lblW, h);
        m_edtRise.MoveWindow(x + lblW + pad, y0, (ctlW - pad) / 2, h);
        m_edtFall.MoveWindow(x + lblW + pad + (ctlW - pad) / 2 + pad, y0, ctlW - (ctlW - pad) / 2 - pad, h);
        m_btnSetRamp.MoveWindow(x + lblW + pad + ctlW + pad, y0, btnW, h);
        y0 += rowH;
        m_lblData.MoveWindow(x, y0, lblW, h);
        // Check boxes take the width their captions need; the button moves
        // right when the two of them are wider than a normal edit field.
        const int wRaw = Dpi::FitWidth(m_chkRaw, sc(44)), wFft = Dpi::FitWidth(m_chkFft, sc(44));
        m_chkRaw.MoveWindow(x + lblW + pad, y0, wRaw, h);
        m_chkFft.MoveWindow(x + lblW + pad + wRaw + pad, y0, wFft, h);
        m_btnApplyData.MoveWindow(x + lblW + pad + (std::max)(ctlW, wRaw + pad + wFft) + pad, y0, btnW, h);
        y0 += rowH;
        const int wPing = Dpi::FitWidth(m_btnPing, sc(70)), wStat = Dpi::FitWidth(m_btnGetStatus, sc(90)),
                  wAll = Dpi::FitWidth(m_btnSendAll, sc(90));
        m_btnPing.MoveWindow(x, y0, wPing, h);
        m_btnGetStatus.MoveWindow(x + wPing + pad, y0, wStat, h);
        m_btnSendAll.MoveWindow(x + wPing + pad + wStat + pad, y0, wAll, h);
        y0 += rowH + sc(6);
    }
    const int yMcuEnd = y0;

    // Application: its own column when wide, else under the MCU column.
    const int colD = wide ? 2 : 0;
    int yD = wide ? sc(10) : y0;
    header(m_hdrDisplay, colD, yD);
    row(colD, yD, m_lblDark, m_chkDark, nullptr, 0, Dpi::FitWidth(m_chkDark, sc(150)));
    row(colD, yD, m_lblLang, m_cmbLang, nullptr, sc(120), ctlW + btnW + pad);
    row(colD, yD, m_lblFsCal, m_edtFsCal);
    row(colD, yD, m_lblPpm, m_edtPpm, &m_btnApplyPpm);
    row(colD, yD, m_lblVerbose, m_chkVerbose, nullptr, 0, Dpi::FitWidth(m_chkVerbose, sc(150)));
    row(colD, yD, m_lblAutoConn, m_chkAutoConnect, nullptr, 0, Dpi::FitWidth(m_chkAutoConnect, sc(150)));
    if (!wide) y0 = yD;

    // Column 1: Radar + Derived.
    int y1 = narrow ? y0 + sc(10) : sc(10);
    const int col1 = narrow ? 0 : 1;
    header(m_hdrRadar, col1, y1);
    row(col1, y1, m_lblVco, m_chkVco, nullptr, 0, Dpi::FitWidth(m_chkVco, ctlW + btnW + pad));
    {
        const int x = colX[col1];
        m_lblVtune.MoveWindow(x, y1, lblW, h);
        m_edtVtuneLo.MoveWindow(x + lblW + pad, y1, (ctlW - pad) / 2, h);
        m_edtVtuneHi.MoveWindow(x + lblW + pad + (ctlW - pad) / 2 + pad, y1, ctlW - (ctlW - pad) / 2 - pad, h);
        y1 += rowH;
    }
    row(col1, y1, m_lblCurve, m_edtCurve, nullptr, 0, ctlW + btnW + pad);
    row(col1, y1, m_lblF0, m_edtF0);
    row(col1, y1, m_lblBw, m_edtBw);
    row(col1, y1, m_lblTramp, m_edtTramp);
    row(col1, y1, m_lblRoff, m_edtRoff);
    row(col1, y1, m_lblShape, m_cmbShape, nullptr, sc(200), ctlW + btnW + pad);
    {
        const int x = colX[col1];
        m_lblChirps.MoveWindow(x, y1, lblW, h);
        const int wAuto = Dpi::FitWidth(m_chkChirpsAuto, sc(70));
        m_chkChirpsAuto.MoveWindow(x + lblW + pad, y1, wAuto, h);
        m_edtChirps.MoveWindow(x + lblW + pad + wAuto + pad, y1, sc(60), h);
        y1 += rowH;
    }
    row(col1, y1, m_lblPairV, m_edtPairV);
    y1 += sc(6);
    header(m_hdrDerived, col1, y1);
    const int derivedW = narrow ? rc.Width() - colX[col1] - sc(10) : colW + sc(10);
    // The read-out grows with its text so the chirp preview below never
    // covers the last block.
    int derivedLines = 1;
    {
        CString t; m_lblDerived.GetWindowText(t);
        for (int i = 0; i < t.GetLength(); ++i) if (t[i] == _T('\n')) ++derivedLines;
    }
    const int derivedH = derivedLines * Dpi::LineHeight(m_lblDerived) + sc(8);
    m_lblDerived.MoveWindow(colX[col1], y1, derivedW, derivedH);
    y1 += derivedH + sc(6);

    const int y2 = narrow ? (std::max)(y0, yD) : sc(10);
    (void)y2;

    if (m_preview.GetSafeHwnd()) {
        CRect pr;
        if (narrow) {
            const int top = (std::max)(y0, y2) + sc(6);
            pr.SetRect(colX[0], top, rc.Width() - sc(10), (std::max)(top + sc(260), rc.Height() - sc(10)));
        } else if (wide) {
            const int top = (std::max)(yMcuEnd, y1) + sc(4);
            pr.SetRect(colX[0], top, colX[1] + colW + sc(10), (std::max)(top + sc(300), rc.Height() - sc(10)));
        } else {
            pr.SetRect(colX[col1], y1, colX[col1] + derivedW, (std::max)(y1 + sc(260), rc.Height() - sc(10)));
        }
        m_preview.MoveWindow(pr);
    }
}

BOOL SettingsTab::OnEraseBkgnd(CDC* pDC)
{
    CRect rc; GetClientRect(&rc);
    pDC->FillSolidRect(rc, Theme::Get().bg);
    return TRUE;
}

HBRUSH SettingsTab::OnCtlColor(CDC* pDC, CWnd*, UINT nCtlColor)
{
    if (nCtlColor == CTLCOLOR_STATIC || nCtlColor == CTLCOLOR_BTN) {
        pDC->SetBkMode(TRANSPARENT);
        pDC->SetTextColor(Theme::Get().text);
        return static_cast<HBRUSH>(m_bgBrush.GetSafeHandle());
    }
    // Edit fields, list boxes and combo drop-downs: keep text readable in the dark theme.
    // Editable fields: white (light theme) so they stand out from read-only
    // read-outs, which arrive as CTLCOLOR_STATIC and keep the grey background.
    pDC->SetTextColor(Theme::Get().text);
    pDC->SetBkColor(Theme::Get().plot);
    return Theme::FieldBrush();
}
