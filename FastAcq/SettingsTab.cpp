#include "pch.h"
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
    ON_BN_CLICKED(IDC_BTN_SET_BURST,   &SettingsTab::OnSetBurst)
    ON_BN_CLICKED(IDC_BTN_SET_RAMP,    &SettingsTab::OnSetRamp)
    ON_BN_CLICKED(IDC_BTN_APPLY_PPM,   &SettingsTab::OnApplyPpm)
    ON_EN_CHANGE(IDC_EDT_FREQ,         &SettingsTab::OnFreqChanged)
    ON_EN_CHANGE(IDC_EDT_RISE,         &SettingsTab::OnRampChanged)
    ON_EN_CHANGE(IDC_EDT_FALL,         &SettingsTab::OnRampChanged)
    ON_CBN_SELCHANGE(IDC_CMB_MODE,     &SettingsTab::OnModeSelChanged)
    ON_BN_CLICKED(IDC_CHK_FW_GEOM,     &SettingsTab::OnAutoApply)
    ON_BN_CLICKED(IDC_CHK_TONE,        &SettingsTab::OnAutoApply)
    ON_BN_CLICKED(IDC_CHK_VCO,         &SettingsTab::OnAutoApply)
    ON_CONTROL_RANGE(EN_KILLFOCUS, IDC_EDT_VTUNE_LO, IDC_EDT_VCO_CURVE, &SettingsTab::OnEditKillFocus)
    ON_CONTROL_RANGE(EN_CHANGE, IDC_EDT_SAMPLES, IDC_EDT_BURST, &SettingsTab::OnPreviewInput)
    ON_BN_CLICKED(IDC_BTN_PING,        &SettingsTab::OnPing)
    ON_BN_CLICKED(IDC_BTN_GET_STATUS,  &SettingsTab::OnGetStatus)
    ON_BN_CLICKED(IDC_BTN_SEND_ALL,    &SettingsTab::OnSendAll)
    ON_BN_CLICKED(IDC_BTN_APPLY_PROC,  &SettingsTab::OnApplyProc)
    ON_BN_CLICKED(IDC_BTN_DEFAULTS,    &SettingsTab::OnDefaults)
    ON_CONTROL_RANGE(CBN_SELCHANGE, IDC_EDT_F0, IDC_BTN_DEFAULTS, &SettingsTab::OnAutoApplyRange)
    ON_CONTROL_RANGE(BN_CLICKED,    IDC_EDT_F0, IDC_BTN_APPLY_PROC - 1, &SettingsTab::OnAutoApplyRange)
    ON_CONTROL_RANGE(EN_KILLFOCUS,  IDC_EDT_FREQ, IDC_BTN_DEFAULTS, &SettingsTab::OnEditKillFocus)
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
    m_hdrMcu.Create(_T("MCU acquisition"), ss, rc, this);
    m_lblMode.Create(_T("Mode"), ss, rc, this);
    m_cmbMode.Create(cs, rc, this, IDC_CMB_MODE);
    m_cmbMode.AddString(_T("Idle")); m_cmbMode.AddString(_T("Continuous")); m_cmbMode.AddString(_T("Single"));
    m_cmbMode.SetCurSel(1);
    m_btnApplyMode.Create(_T("Apply"), bs, rc, this, IDC_BTN_APPLY_MODE);
    m_lblFreq.Create(_T("Chirp freq, Hz (100..24000)"), ss, rc, this);
    m_edtFreq.Create(es, rc, this, IDC_EDT_FREQ);
    m_btnSetFreq.Create(_T("Set"), bs, rc, this, IDC_BTN_SET_FREQ);
    m_lblSamples.Create(_T("Samples (0 = auto)"), ss, rc, this);
    m_edtSamples.Create(es, rc, this, IDC_EDT_SAMPLES);
    m_btnSetSamples.Create(_T("Set"), bs, rc, this, IDC_BTN_SET_SAMPLES);
    m_lblInterval.Create(_T("Interval, ms"), ss, rc, this);
    m_edtInterval.Create(es, rc, this, IDC_EDT_INTERVAL);
    m_btnApplyInterval.Create(_T("Set"), bs, rc, this, IDC_BTN_APPLY_INT);
    m_lblAmplitude.Create(_T("Chirp amplitude, DAC"), ss, rc, this);
    m_edtAmplitude.Create(es, rc, this, IDC_EDT_AMPLITUDE);
    m_btnSetAmp.Create(_T("Set"), bs, rc, this, IDC_BTN_SET_AMP);
    m_lblBurst.Create(_T("Chirps per capture"), ss, rc, this);
    m_edtBurst.Create(es, rc, this, IDC_EDT_BURST);
    m_btnSetBurst.Create(_T("Set"), bs, rc, this, IDC_BTN_SET_BURST);
    m_lblRamp.Create(_T("Ramp rise / fall, us"), ss, rc, this);
    m_edtRise.Create(es, rc, this, IDC_EDT_RISE);
    m_edtFall.Create(es, rc, this, IDC_EDT_FALL);
    m_btnSetRamp.Create(_T("Set ramp"), bs, rc, this, IDC_BTN_SET_RAMP);
    m_lblData.Create(_T("Frame content"), ss, rc, this);
    m_chkRaw.Create(_T("Raw"), chk, rc, this, IDC_CHK_RAW);
    m_chkFft.Create(_T("FFT"), chk, rc, this, IDC_CHK_FFT);
    m_btnApplyData.Create(_T("Set"), bs, rc, this, IDC_BTN_APPLY_DATA);
    m_btnPing.Create(_T("Ping"), bs, rc, this, IDC_BTN_PING);
    m_btnGetStatus.Create(_T("Get status"), bs, rc, this, IDC_BTN_GET_STATUS);
    m_btnSendAll.Create(_T("Send all"), bs, rc, this, IDC_BTN_SEND_ALL);
    m_hdrSource.Create(_T("Spectrum source"), ss, rc, this);
    m_lblSource.Create(_T("Process"), ss, rc, this);
    m_rdoSrcRaw.Create(_T("RAW on PC"), WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP, rc, this, IDC_RDO_SRC_RAW);
    m_rdoSrcMcu.Create(_T("MCU FFT"), WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON, rc, this, IDC_RDO_SRC_MCU);
    m_rdoSrcRaw.SetCheck(BST_CHECKED);

    // --- Radar
    m_hdrRadar.Create(_T("Radar geometry"), ss, rc, this);
    m_lblVco.Create(_T("VCO (HMC431)"), ss, rc, this);
    m_chkVco.Create(_T("f0 / B from tuning curve"), chk, rc, this, IDC_CHK_VCO);
    m_lblVtune.Create(_T("Vtune at DAC 0 / 4095, V"), ss, rc, this);
    m_edtVtuneLo.Create(esf, rc, this, IDC_EDT_VTUNE_LO);
    m_edtVtuneHi.Create(esf, rc, this, IDC_EDT_VTUNE_HI);
    m_lblCurve.Create(_T("Tuning curve V:GHz,..."), ss, rc, this);
    m_edtCurve.Create(esf, rc, this, IDC_EDT_VCO_CURVE);
    m_lblF0.Create(_T("Carrier f0 (sweep centre), GHz"), ss, rc, this);          m_edtF0.Create(esf, rc, this, IDC_EDT_F0);
    m_lblBw.Create(_T("Sweep bandwidth B, MHz"), ss, rc, this);   m_edtBw.Create(esf, rc, this, IDC_EDT_BW);
    m_lblTramp.Create(_T("Ramp time, ms (0 = auto)"), ss, rc, this); m_edtTramp.Create(esf, rc, this, IDC_EDT_TRAMP);
    m_lblRoff.Create(_T("Range offset, m"), ss, rc, this);        m_edtRoff.Create(esf, rc, this, IDC_EDT_ROFFSET);
    m_lblShape.Create(_T("Modulation"), ss, rc, this);
    m_cmbShape.Create(cs, rc, this, IDC_CMB_SHAPE);
    for (int i = 0; i < static_cast<int>(dsp::RampShape::Count); ++i)
        m_cmbShape.AddString(CString(dsp::RampShapeName(static_cast<dsp::RampShape>(i))));
    m_lblChirps.Create(_T("Chirps per frame"), ss, rc, this);
    m_chkChirpsAuto.Create(_T("= burst"), chk, rc, this, IDC_CHK_CHIRPS_AUTO);
    m_edtChirps.Create(es, rc, this, IDC_EDT_CHIRPS);
    m_lblPairV.Create(_T("UP/DOWN pair gate, m/s"), ss, rc, this); m_edtPairV.Create(esf, rc, this, IDC_EDT_PAIR_V);

    // --- Display
    m_hdrDisplay.Create(_T("Display"), ss, rc, this);
    m_lblDbTop.Create(_T("dB axis top"), ss, rc, this);           m_edtDbTop.Create(esf, rc, this, IDC_EDT_DBTOP);
    m_lblDbBottom.Create(_T("dB axis bottom"), ss, rc, this);     m_edtDbBottom.Create(esf, rc, this, IDC_EDT_DBBOTTOM);
    m_lblPalette.Create(_T("Waterfall palette"), ss, rc, this);
    m_cmbPalette.Create(cs, rc, this, IDC_CMB_PALETTE);
    for (int i = 0; i < static_cast<int>(Palette::Count); ++i) m_cmbPalette.AddString(ColorMap::Name(static_cast<Palette>(i)));
    m_lblWfRows.Create(_T("Waterfall rows"), ss, rc, this);       m_edtWfRows.Create(es, rc, this, IDC_EDT_WF_ROWS);
    m_lblDark.Create(_T("Theme"), ss, rc, this);                  m_chkDark.Create(_T("Dark"), chk, rc, this, IDC_CHK_DARK);
    m_lblAdcBits.Create(_T("ADC bits"), ss, rc, this);            m_edtAdcBits.Create(es, rc, this, IDC_EDT_ADCBITS);
    m_lblVref.Create(_T("ADC full scale, V"), ss, rc, this);      m_edtVref.Create(esf, rc, this, IDC_EDT_VREF);
    m_lblFsCal.Create(_T("Fallback Fs, Hz"), ss, rc, this);       m_edtFsCal.Create(es, rc, this, IDC_EDT_FSCAL);
    m_lblPpm.Create(_T("ADC clock corr., ppm"), ss, rc, this);     m_edtPpm.Create(esf, rc, this, IDC_EDT_PPM);
    m_btnApplyPpm.Create(_T("Apply"), bs, rc, this, IDC_BTN_APPLY_PPM);
    m_lblVerbose.Create(_T("Log"), ss, rc, this);                 m_chkVerbose.Create(_T("Per-frame RX lines"), chk, rc, this, IDC_CHK_VERBOSE);
    m_lblAutoConn.Create(_T("Start-up"), ss, rc, this);           m_chkAutoConnect.Create(_T("Auto-connect"), chk, rc, this, IDC_CHK_AUTOCONNECT);

    // --- Processing
    m_hdrProc.Create(_T("Processing"), ss, rc, this);
    m_lblGuard.Create(_T("Ramp guard, % each end"), ss, rc, this); m_edtGuard.Create(esf, rc, this, IDC_EDT_GUARD);
    m_lblDetrend.Create(_T("Detrend"), ss, rc, this);             m_chkDetrend.Create(_T("Remove linear trend"), chk, rc, this, IDC_CHK_DETREND);
    m_lblDecim.Create(_T("Decimation"), ss, rc, this);
    m_cmbDecim.Create(cs, rc, this, IDC_CMB_DECIM);
    m_cmbDecim.AddString(_T("Auto"));
    for (int d = 1; d <= 512; d *= 2) { CString s; s.Format(_T("x%d"), d); m_cmbDecim.AddString(s); }
    m_lblMaxRange.Create(_T("Range of interest, m"), ss, rc, this); m_edtMaxRange.Create(esf, rc, this, IDC_EDT_MAXRANGE);
    m_lblWindow.Create(_T("Window"), ss, rc, this);
    m_cmbWindow.Create(cs, rc, this, IDC_CMB_WINDOW);
    for (int i = 0; i < static_cast<int>(dsp::WindowType::Count); ++i)
        m_cmbWindow.AddString(CString(dsp::WindowName(static_cast<dsp::WindowType>(i))));
    m_lblKaiser.Create(_T("Kaiser beta"), ss, rc, this);          m_edtKaiser.Create(esf, rc, this, IDC_EDT_KAISER);
    m_lblZeroPad.Create(_T("Zero padding"), ss, rc, this);
    m_cmbZeroPad.Create(cs, rc, this, IDC_CMB_ZEROPAD);
    for (int z = 1; z <= 8; z *= 2) { CString s; s.Format(_T("x%d"), z); m_cmbZeroPad.AddString(s); }
    m_lblRangeGain.Create(_T("Range gain"), ss, rc, this);
    m_cmbRangeGain.Create(cs, rc, this, IDC_CMB_RANGEGAIN);
    m_cmbRangeGain.AddString(_T("Off")); m_cmbRangeGain.AddString(_T("20 dB/dec (R^2)")); m_cmbRangeGain.AddString(_T("40 dB/dec (R^4)"));
    m_lblDetector.Create(_T("Detector"), ss, rc, this);
    m_cmbDetector.Create(cs, rc, this, IDC_CMB_DETECTOR);
    for (int i = 0; i < static_cast<int>(dsp::DetectorType::Count); ++i)
        m_cmbDetector.AddString(CString(dsp::DetectorName(static_cast<dsp::DetectorType>(i))));
    m_lblThresh.Create(_T("Threshold above noise, dB"), ss, rc, this); m_edtThresh.Create(esf, rc, this, IDC_EDT_THRESH);
    m_lblPfa.Create(_T("CFAR Pfa"), ss, rc, this);
    m_cmbPfa.Create(cs, rc, this, IDC_CMB_PFA);
    m_cmbPfa.AddString(_T("1e-2")); m_cmbPfa.AddString(_T("1e-3")); m_cmbPfa.AddString(_T("1e-4"));
    m_cmbPfa.AddString(_T("1e-5")); m_cmbPfa.AddString(_T("1e-6"));
    m_lblCfarGuard.Create(_T("CFAR guard cells"), ss, rc, this);  m_edtCfarGuard.Create(es, rc, this, IDC_EDT_CFAR_GUARD);
    m_lblCfarTrain.Create(_T("CFAR training cells"), ss, rc, this); m_edtCfarTrain.Create(es, rc, this, IDC_EDT_CFAR_TRAIN);
    m_lblInterp.Create(_T("Peak interpolation"), ss, rc, this);
    m_cmbInterp.Create(cs, rc, this, IDC_CMB_INTERP);
    for (int i = 0; i < static_cast<int>(dsp::PeakInterp::Count); ++i)
        m_cmbInterp.AddString(CString(dsp::PeakInterpName(static_cast<dsp::PeakInterp>(i))));
    m_lblMaxPeaks.Create(_T("Max targets"), ss, rc, this);        m_edtMaxPeaks.Create(es, rc, this, IDC_EDT_MAXPEAKS);
    m_lblMti.Create(_T("Doppler"), ss, rc, this);                 m_chkMti.Create(_T("MTI (subtract mean)"), chk, rc, this, IDC_CHK_MTI);
    m_lblTrack.Create(_T("Tracking"), ss, rc, this);              m_chkTrack.Create(_T("Stable target IDs"), chk, rc, this, IDC_CHK_TRACK);
    m_lblFwGeom.Create(_T("Chirp split"), ss, rc, this);          m_chkFwGeom.Create(_T("From frame header (v2 exact)"), chk, rc, this, IDC_CHK_FW_GEOM);
    m_lblTone.Create(_T("Tone estimate"), ss, rc, this);          m_chkTone.Create(_T("Precise (double FFT)"), chk, rc, this, IDC_CHK_TONE);
    m_btnApplyProc.Create(_T("Apply"), bs, rc, this, IDC_BTN_APPLY_PROC);
    m_btnDefaults.Create(_T("Defaults"), bs, rc, this, IDC_BTN_DEFAULTS);
    m_hdrDerived.Create(_T("Derived values"), ss, rc, this);
    m_lblDerived.Create(_T(""), WS_CHILD | WS_VISIBLE | SS_LEFT, rc, this, IDC_LBL_DERIVED);
    m_preview.CreateCtrl(this, IDC_CHIRP_PREVIEW);

    // Fonts.
    CWnd* pw = GetWindow(GW_CHILD);
    while (pw) { pw->SetFont(&m_font); pw = pw->GetWindow(GW_HWNDNEXT); }
    for (CStatic* h : { &m_hdrMcu, &m_hdrSource, &m_hdrRadar, &m_hdrDisplay, &m_hdrProc, &m_hdrDerived })
        h->SetFont(&m_hdrFont);

    AppSettings defaults;
    ApplySettings(defaults);
    SetConnected(false);
    return 0;
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
    m_suppress = true;
    m_cmbMode.SetCurSel(s.acq.mode >= 0 && s.acq.mode <= 2 ? s.acq.mode : 1);
    SetInt(m_edtFreq, s.acq.chirpFreqHz);
    SetInt(m_edtSamples, s.acq.samples);
    SetInt(m_edtInterval, s.acq.intervalMs);
    SetInt(m_edtAmplitude, s.acq.amplitude);
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
    m_rdoSrcRaw.SetCheck(s.dsp.useMcuFft ? BST_UNCHECKED : BST_CHECKED);
    m_rdoSrcMcu.SetCheck(s.dsp.useMcuFft ? BST_CHECKED : BST_UNCHECKED);

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

    SetDouble(m_edtDbTop, s.display.dbTop, _T("%.0f"));
    SetDouble(m_edtDbBottom, s.display.dbBottom, _T("%.0f"));
    m_cmbPalette.SetCurSel(s.display.palette);
    SetInt(m_edtWfRows, s.display.waterfallRows);
    m_chkDark.SetCheck(s.display.darkTheme ? BST_CHECKED : BST_UNCHECKED);
    SetInt(m_edtAdcBits, s.display.adcBits);
    SetDouble(m_edtVref, s.display.vRef, _T("%.3f"));
    SetInt(m_edtFsCal, s.sampleRateCalHz);
    SetDouble(m_edtPpm, s.fsPpm, _T("%.3f"));
    m_chkVerbose.SetCheck(s.verboseLog ? BST_CHECKED : BST_UNCHECKED);
    m_chkAutoConnect.SetCheck(s.autoConnect ? BST_CHECKED : BST_UNCHECKED);

    SetDouble(m_edtGuard, s.dsp.guardPct, _T("%.1f"));
    m_chkDetrend.SetCheck(s.dsp.detrend ? BST_CHECKED : BST_UNCHECKED);
    int dsel = 0;
    if (s.dsp.decimation >= 1) { int d = 1; dsel = 1; while (d < s.dsp.decimation && d < 512) { d *= 2; ++dsel; } }
    m_cmbDecim.SetCurSel(dsel);
    SetDouble(m_edtMaxRange, s.dsp.maxRangeM, _T("%.1f"));
    m_cmbWindow.SetCurSel(static_cast<int>(s.dsp.window));
    SetDouble(m_edtKaiser, s.dsp.kaiserBeta, _T("%.1f"));
    int zsel = 0; { int z = 1; while (z < s.dsp.zeroPad && z < 8) { z *= 2; ++zsel; } }
    m_cmbZeroPad.SetCurSel(zsel);
    m_cmbRangeGain.SetCurSel(s.dsp.rangeGainDbPerDecade >= 40.0f ? 2 : s.dsp.rangeGainDbPerDecade >= 20.0f ? 1 : 0);
    m_cmbDetector.SetCurSel(static_cast<int>(s.dsp.detector.type));
    SetDouble(m_edtThresh, s.dsp.detector.thresholdDb, _T("%.1f"));
    int psel = 2; { double p = s.dsp.detector.pfa; psel = (p >= 5e-3) ? 0 : (p >= 5e-4) ? 1 : (p >= 5e-5) ? 2 : (p >= 5e-6) ? 3 : 4; }
    m_cmbPfa.SetCurSel(psel);
    SetInt(m_edtCfarGuard, s.dsp.detector.guardCells);
    SetInt(m_edtCfarTrain, s.dsp.detector.trainCells);
    m_cmbInterp.SetCurSel(static_cast<int>(s.dsp.interp));
    SetInt(m_edtMaxPeaks, s.dsp.maxPeaks);
    m_chkMti.SetCheck(s.dsp.mti ? BST_CHECKED : BST_UNCHECKED);
    m_chkTrack.SetCheck(s.dsp.trackTargets ? BST_CHECKED : BST_UNCHECKED);
    m_chkFwGeom.SetCheck(s.dsp.firmwareGeometry ? BST_CHECKED : BST_UNCHECKED);
    m_chkTone.SetCheck(s.dsp.toneEstimate ? BST_CHECKED : BST_UNCHECKED);
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
    s.acq.burst       = GetBurst();
    s.acq.riseUs      = GetRiseUs();
    s.acq.fallUs      = GetFallUs();
    s.acq.sendRaw     = m_chkRaw.GetCheck() == BST_CHECKED;
    s.acq.sendFft     = m_chkFft.GetCheck() == BST_CHECKED;
    s.dsp.useMcuFft   = m_rdoSrcMcu.GetCheck() == BST_CHECKED;

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

    s.display.dbTop    = static_cast<float>(GetDouble(m_edtDbTop, 0.0));
    s.display.dbBottom = static_cast<float>(GetDouble(m_edtDbBottom, -120.0));
    if (s.display.dbTop <= s.display.dbBottom) { s.display.dbTop = 0.0f; s.display.dbBottom = -120.0f; }
    int pal = m_cmbPalette.GetCurSel();
    if (pal >= 0 && pal < static_cast<int>(Palette::Count)) s.display.palette = pal;
    s.display.waterfallRows = (std::max)(64, (std::min)(4096, GetInt(m_edtWfRows, 512)));
    s.display.darkTheme = m_chkDark.GetCheck() == BST_CHECKED;
    s.display.adcBits   = (std::max)(8, (std::min)(16, GetInt(m_edtAdcBits, 12)));
    s.display.vRef      = static_cast<float>(GetDouble(m_edtVref, 3.3));
    int fs = GetInt(m_edtFsCal, 60058600);
    s.sampleRateCalHz   = fs > 1000 ? static_cast<uint32_t>(fs) : 60058600u;
    s.fsPpm             = GetFsPpm();
    s.verboseLog        = m_chkVerbose.GetCheck() == BST_CHECKED;
    s.autoConnect       = m_chkAutoConnect.GetCheck() == BST_CHECKED;

    s.dsp.guardPct   = static_cast<float>((std::max)(0.0, (std::min)(45.0, GetDouble(m_edtGuard, 5.0))));
    s.dsp.detrend    = m_chkDetrend.GetCheck() == BST_CHECKED;
    int dsel = m_cmbDecim.GetCurSel();
    s.dsp.decimation = (dsel <= 0) ? 0 : (1 << (dsel - 1));
    s.dsp.maxRangeM  = (std::max)(0.1, GetDouble(m_edtMaxRange, 100.0));
    int win = m_cmbWindow.GetCurSel();
    if (win >= 0 && win < static_cast<int>(dsp::WindowType::Count)) s.dsp.window = static_cast<dsp::WindowType>(win);
    s.dsp.kaiserBeta = static_cast<float>((std::max)(0.0, GetDouble(m_edtKaiser, 9.0)));
    int zsel = m_cmbZeroPad.GetCurSel();
    s.dsp.zeroPad = (zsel < 0) ? 2 : (1 << zsel);
    int rg = m_cmbRangeGain.GetCurSel();
    s.dsp.rangeGainDbPerDecade = (rg == 1) ? 20.0f : (rg == 2) ? 40.0f : 0.0f;
    int det = m_cmbDetector.GetCurSel();
    if (det >= 0 && det < static_cast<int>(dsp::DetectorType::Count)) s.dsp.detector.type = static_cast<dsp::DetectorType>(det);
    s.dsp.detector.thresholdDb = static_cast<float>(GetDouble(m_edtThresh, 12.0));
    static const float kPfa[] = { 1e-2f, 1e-3f, 1e-4f, 1e-5f, 1e-6f };
    int psel = m_cmbPfa.GetCurSel();
    s.dsp.detector.pfa = (psel >= 0 && psel < 5) ? kPfa[psel] : 1e-4f;
    s.dsp.detector.guardCells = (std::max)(0, GetInt(m_edtCfarGuard, 2));
    s.dsp.detector.trainCells = (std::max)(2, GetInt(m_edtCfarTrain, 16));
    int ip = m_cmbInterp.GetCurSel();
    if (ip >= 0 && ip < static_cast<int>(dsp::PeakInterp::Count)) s.dsp.interp = static_cast<dsp::PeakInterp>(ip);
    s.dsp.maxPeaks = (std::max)(1, (std::min)(64, GetInt(m_edtMaxPeaks, 10)));
    s.dsp.mti          = m_chkMti.GetCheck() == BST_CHECKED;
    s.dsp.trackTargets = m_chkTrack.GetCheck() == BST_CHECKED;
    s.dsp.firmwareGeometry = m_chkFwGeom.GetCheck() == BST_CHECKED;
    s.dsp.toneEstimate     = m_chkTone.GetCheck() == BST_CHECKED;
}

void SettingsTab::SetObserved(double fsHz, size_t samplesPerFrame, size_t mcuFftSize)
{
    m_obsFs = fsHz; m_obsSamples = samplesPerFrame; m_obsMcuFft = mcuFftSize;
    RefreshDerived();
}

void SettingsTab::RefreshDerived()
{
    if (!m_lblDerived.GetSafeHwnd()) return;
    AppSettings s;
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
    {
        const core::VcoSweep sw = s.vco.SweepFor(s.acq.amplitude);
        if (sw.valid) {
            line.Format(_T("VCO: Vtune %.2f..%.2f V -> %.4f..%.4f GHz, B %.1f MHz, %.0f..%.0f MHz/V, nonlin. %.1f%%%s\r\n"),
                        sw.vLowV, sw.vHighV, sw.fStartHz / 1e9, sw.fStopHz / 1e9, sw.bandwidthHz / 1e6,
                        sw.sensMinHzPerV / 1e6, sw.sensMaxHzPerV / 1e6, sw.nonlinearityPct,
                        sw.outOfTable ? _T(" [Vtune outside the curve table!]") : _T(""));
            t += line;
        } else {
            t += _T("VCO: tuning curve invalid (need >= 2 points V:GHz)\r\n");
        }
    }
    line.Format(_T("Ramp %.4f ms, period %.4f ms\r\n"), d.rampSec * 1e3, d.periodSec * 1e3); t += line;
    line.Format(_T("%zu samples/ramp, %zu used\r\n"), d.samplesPerRamp, d.samplesUsed); t += line;
    line.Format(_T("Decimation x%d: Fs_eff %.1f kHz, FFT %zu\r\n"), d.decimation, d.fsEffHz / 1e3, d.fftSize); t += line;
    line.Format(_T("Bin %.1f Hz"), d.binHz); t += line;
    if (d.rangeBinM > 0.0) { line.Format(_T(" = %.3f m"), d.rangeBinM); t += line; }
    t += _T("\r\n");
    if (d.rangeResM > 0.0) {
        line.Format(_T("Range res. c/2B %.3f m (eff. %.3f m), R_max %.1f m\r\n"), d.rangeResM, d.rangeResEffM, d.rangeMaxM); t += line;
        line.Format(_T("Beat %.1f Hz per metre\r\n"), d.beatPerMeterHz); t += line;
    } else {
        t += _T("Range axis: set bandwidth B > 0\r\n");
    }
    if (d.velResMps > 0.0) {
        line.Format(_T("Velocity res. %.3f m/s (burst %d), max +/- %.2f m/s\r\n"), d.velResMps, (std::max)(1, s.dsp.chirpsInFrame), d.velMaxMps); t += line;
    }
    line.Format(_T("USB %.2f MB/s at %d ms (FS CDC limit ~0.8 MB/s)"), d.usbMBps, s.acq.intervalMs); t += line;
    if (s.fsPpm != 0.0) {
        line.Format(_T("\r\nADC clock %+.3f ppm: %.6f -> %.6f MS/s"), s.fsPpm,
                    fs / 1e6, fs * (1.0 + s.fsPpm * 1e-6) / 1e6);
        t += line;
    }
    m_lblDerived.SetWindowText(t);
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
        AppSettings s; ReadInto(s);
        m_preview.SetVco(s.vco.vtuneAtDac0V, s.vco.vtuneAtDacFullV, s.vco.useCurve ? s.vco.Curve() : core::VcoCurve());
    }
    core::ChirpParams p;
    p.freqHz     = GetFreqHz();
    p.riseUs     = GetRiseUs();
    p.fallUs     = GetFallUs();
    p.amplitude  = GetAmplitude();
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
    for (CButton* b : { &m_btnApplyMode, &m_btnSetFreq, &m_btnSetSamples, &m_btnApplyInterval, &m_btnSetAmp,
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
    PostToMain(WM_APP_CMD_SET_BURST,     GetBurst());
    PostToMain(WM_APP_CMD_SET_DATA_MASK, GetDataMask());
    PostToMain(WM_APP_CMD_GET_STATUS);
    NotifyChanged();
}

void SettingsTab::OnApplyProc()  { NotifyChanged(); }
void SettingsTab::OnAutoApply()
{
    const bool vco = m_chkVco.GetCheck() == BST_CHECKED;
    m_edtF0.SetReadOnly(vco ? TRUE : FALSE);
    m_edtBw.SetReadOnly(vco ? TRUE : FALSE);
    NotifyChanged();
}
void SettingsTab::OnEditKillFocus(UINT) { NotifyChanged(); }

void SettingsTab::OnDefaults()
{
    AppSettings d;
    AppSettings cur; ReadInto(cur);
    d.acq = cur.acq;                       // keep MCU values; reset radar/processing/display
    d.sampleRateCalHz = cur.sampleRateCalHz;
    d.fsPpm = cur.fsPpm;
    d.autoConnect = cur.autoConnect;
    ApplySettings(d);
    NotifyChanged();
}

// ---------------------------------------------------------------------------
void SettingsTab::OnSize(UINT, int, int) { Relayout(); }

void SettingsTab::Relayout()
{
    if (!m_lblMode.GetSafeHwnd()) return;
    const int S = static_cast<int>(Dpi::Of(m_hWnd));
    auto sc = [&](int px) { return ::MulDiv(px, S, 96); };
    CRect rc; GetClientRect(&rc);

    const int lblW = sc(150), ctlW = sc(92), btnW = sc(60), pad = sc(6), h = sc(23), rowH = sc(28);
    const int colW = lblW + ctlW + btnW + 3 * pad;
    int colX[4] = { sc(12), sc(12) + colW + sc(16), sc(12) + 2 * (colW + sc(16)), sc(12) + 3 * (colW + sc(16)) };
    // Wide: MCU | Radar + derived | Processing | Display, chirp preview under
    // the first two columns. Medium: Display under MCU, preview under Radar.
    // Narrow: everything stacked in one column, preview at the bottom.
    const bool narrow = rc.Width() < colX[2] + colW;
    const bool wide   = rc.Width() >= colX[3] + colW;

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
    row(0, y0, m_lblBurst, m_edtBurst, &m_btnSetBurst);
    {
        const int x = colX[0];
        m_lblRamp.MoveWindow(x, y0, lblW, h);
        m_edtRise.MoveWindow(x + lblW + pad, y0, (ctlW - pad) / 2, h);
        m_edtFall.MoveWindow(x + lblW + pad + (ctlW - pad) / 2 + pad, y0, ctlW - (ctlW - pad) / 2 - pad, h);
        m_btnSetRamp.MoveWindow(x + lblW + pad + ctlW + pad, y0, btnW, h);
        y0 += rowH;
        m_lblData.MoveWindow(x, y0, lblW, h);
        m_chkRaw.MoveWindow(x + lblW + pad, y0, sc(44), h);
        m_chkFft.MoveWindow(x + lblW + pad + sc(46), y0, sc(44), h);
        m_btnApplyData.MoveWindow(x + lblW + pad + ctlW + pad, y0, btnW, h);
        y0 += rowH;
        m_btnPing.MoveWindow(x, y0, sc(70), h);
        m_btnGetStatus.MoveWindow(x + sc(76), y0, sc(90), h);
        m_btnSendAll.MoveWindow(x + sc(172), y0, sc(90), h);
        y0 += rowH + sc(6);
    }
    header(m_hdrSource, 0, y0);
    {
        const int x = colX[0];
        m_lblSource.MoveWindow(x, y0, lblW, h);
        m_rdoSrcRaw.MoveWindow(x + lblW + pad, y0, sc(84), h);
        m_rdoSrcMcu.MoveWindow(x + lblW + pad + sc(88), y0, sc(76), h);
        y0 += rowH + sc(6);
    }
    const int yMcuEnd = y0;

    // Display: its own column when wide, else under the MCU column.
    const int colD = wide ? 3 : 0;
    int yD = wide ? sc(10) : y0;
    header(m_hdrDisplay, colD, yD);
    row(colD, yD, m_lblDbTop, m_edtDbTop);
    row(colD, yD, m_lblDbBottom, m_edtDbBottom);
    row(colD, yD, m_lblPalette, m_cmbPalette, nullptr, sc(200));
    row(colD, yD, m_lblWfRows, m_edtWfRows);
    row(colD, yD, m_lblDark, m_chkDark, nullptr, 0, sc(150));
    row(colD, yD, m_lblAdcBits, m_edtAdcBits);
    row(colD, yD, m_lblVref, m_edtVref);
    row(colD, yD, m_lblFsCal, m_edtFsCal);
    row(colD, yD, m_lblPpm, m_edtPpm, &m_btnApplyPpm);
    row(colD, yD, m_lblVerbose, m_chkVerbose, nullptr, 0, sc(150));
    row(colD, yD, m_lblAutoConn, m_chkAutoConnect, nullptr, 0, sc(150));
    if (!wide) y0 = yD;

    // Column 1: Radar + Derived.
    int y1 = narrow ? y0 + sc(10) : sc(10);
    const int col1 = narrow ? 0 : 1;
    header(m_hdrRadar, col1, y1);
    row(col1, y1, m_lblVco, m_chkVco, nullptr, 0, ctlW + btnW + pad);
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
        m_chkChirpsAuto.MoveWindow(x + lblW + pad, y1, sc(70), h);
        m_edtChirps.MoveWindow(x + lblW + pad + sc(74), y1, sc(60), h);
        y1 += rowH;
    }
    row(col1, y1, m_lblPairV, m_edtPairV);
    y1 += sc(6);
    header(m_hdrDerived, col1, y1);
    const int derivedW = narrow ? rc.Width() - colX[col1] - sc(10) : colW + sc(10);
    m_lblDerived.MoveWindow(colX[col1], y1, derivedW, sc(150));
    y1 += sc(156);

    // Column 2: Processing.
    int y2 = narrow ? (std::max)(y0, y1 + sc(130)) : sc(10);
    const int col2 = narrow ? 0 : 2;
    header(m_hdrProc, col2, y2);
    row(col2, y2, m_lblGuard, m_edtGuard);
    row(col2, y2, m_lblDetrend, m_chkDetrend, nullptr, 0, sc(150));
    row(col2, y2, m_lblDecim, m_cmbDecim, nullptr, sc(240));
    row(col2, y2, m_lblMaxRange, m_edtMaxRange);
    row(col2, y2, m_lblWindow, m_cmbWindow, nullptr, sc(200), ctlW + btnW + pad);
    row(col2, y2, m_lblKaiser, m_edtKaiser);
    row(col2, y2, m_lblZeroPad, m_cmbZeroPad, nullptr, sc(120));
    row(col2, y2, m_lblRangeGain, m_cmbRangeGain, nullptr, sc(120), ctlW + btnW + pad);
    row(col2, y2, m_lblDetector, m_cmbDetector, nullptr, sc(120), ctlW + btnW + pad);
    row(col2, y2, m_lblThresh, m_edtThresh);
    row(col2, y2, m_lblPfa, m_cmbPfa, nullptr, sc(150));
    row(col2, y2, m_lblCfarGuard, m_edtCfarGuard);
    row(col2, y2, m_lblCfarTrain, m_edtCfarTrain);
    row(col2, y2, m_lblInterp, m_cmbInterp, nullptr, sc(150), ctlW + btnW + pad);
    row(col2, y2, m_lblMaxPeaks, m_edtMaxPeaks);
    row(col2, y2, m_lblMti, m_chkMti, nullptr, 0, sc(150));
    row(col2, y2, m_lblTrack, m_chkTrack, nullptr, 0, sc(150));
    row(col2, y2, m_lblFwGeom, m_chkFwGeom, nullptr, 0, sc(200));
    row(col2, y2, m_lblTone, m_chkTone, nullptr, 0, sc(200));
    m_btnApplyProc.MoveWindow(colX[col2] + lblW + pad, y2, sc(80), h);
    m_btnDefaults.MoveWindow(colX[col2] + lblW + pad + sc(86), y2, sc(80), h);
    y2 += rowH;

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
    pDC->SetTextColor(Theme::Get().text);
    pDC->SetBkColor(Theme::Get().bg);
    return static_cast<HBRUSH>(m_bgBrush.GetSafeHandle());
}
