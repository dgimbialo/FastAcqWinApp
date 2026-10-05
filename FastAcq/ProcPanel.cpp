#include "pch.h"
#include "ProcPanel.h"
#include "AppMessages.h"
#include "Dpi.h"
#include "Theme.h"
#include "resource.h"

BEGIN_MESSAGE_MAP(ProcPanel, CWnd)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_ERASEBKGND()
    ON_WM_CTLCOLOR()
    ON_WM_VSCROLL()
    ON_WM_MOUSEWHEEL()
    ON_BN_CLICKED(IDC_BTN_DEFAULTS,    &ProcPanel::OnDefaults)
    ON_BN_CLICKED(IDC_CHK_FW_GEOM,     &ProcPanel::OnAutoApply)
    ON_BN_CLICKED(IDC_CHK_TONE,        &ProcPanel::OnAutoApply)
    ON_CONTROL_RANGE(CBN_SELCHANGE, IDC_EDT_GUARD, IDC_EDT_VREF, &ProcPanel::OnAutoApplyRange)
    ON_CONTROL_RANGE(BN_CLICKED,    IDC_EDT_GUARD, IDC_EDT_VREF, &ProcPanel::OnAutoApplyRange)
    ON_CONTROL_RANGE(EN_KILLFOCUS,  IDC_EDT_GUARD, IDC_EDT_VREF, &ProcPanel::OnEditKillFocus)
END_MESSAGE_MAP()

void ProcPanel::OnAutoApplyRange(UINT) { OnAutoApply(); }
void ProcPanel::OnEditKillFocus(UINT)  { NotifyChanged(); }
void ProcPanel::OnAutoApply()          { NotifyChanged(); }

BOOL ProcPanel::CreatePanel(CWnd* parent, UINT id)
{
    LPCTSTR cls = AfxRegisterWndClass(0, ::LoadCursor(nullptr, IDC_ARROW), nullptr, nullptr);
    return Create(cls, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_VSCROLL, CRect(0, 0, 10, 10), parent, id);
}

int ProcPanel::OnCreate(LPCREATESTRUCT lpcs)
{
    if (CWnd::OnCreate(lpcs) == -1) return -1;
    m_bgBrush.CreateSolidBrush(Theme::Get().bg);
    Dpi::MakeFont(m_font, m_hWnd, 9);
    Dpi::MakeFont(m_hdrFont, m_hWnd, 10, true);

    const DWORD es  = WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_NUMBER;
    const DWORD esf = WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL;
    const DWORD ss  = WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE;
    const DWORD chk = WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX;
    const DWORD cs  = WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL;
    CRect rc(0, 0, 100, 22);

    m_hdrProc.Create(_T("Processing"), ss, rc, this);
    m_lblSource.Create(_T("Spectrum source"), ss, rc, this);
    m_rdoSrcRaw.Create(_T("RAW on PC"), WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP, rc, this, IDC_RDO_SRC_RAW);
    m_rdoSrcMcu.Create(_T("MCU FFT"), WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON, rc, this, IDC_RDO_SRC_MCU);
    m_rdoSrcRaw.SetCheck(BST_CHECKED);
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
    m_lblFwGeom.Create(_T("Chirp split"), ss, rc, this);          m_chkFwGeom.Create(_T("From frame header"), chk, rc, this, IDC_CHK_FW_GEOM);
    m_lblTone.Create(_T("Tone estimate"), ss, rc, this);          m_chkTone.Create(_T("Precise (double FFT)"), chk, rc, this, IDC_CHK_TONE);

    m_hdrDetect.Create(_T("Detection"), ss, rc, this);
    m_lblDetector.Create(_T("Detector"), ss, rc, this);
    m_cmbDetector.Create(cs, rc, this, IDC_CMB_DETECTOR);
    for (int i = 0; i < static_cast<int>(dsp::DetectorType::Count); ++i)
        m_cmbDetector.AddString(CString(dsp::DetectorName(static_cast<dsp::DetectorType>(i))));
    m_lblThresh.Create(_T("Threshold above noise, dB"), ss, rc, this); m_edtThresh.Create(esf, rc, this, IDC_EDT_THRESH);
    m_lblPfa.Create(_T("CFAR false-alarm prob."), ss, rc, this);
    m_cmbPfa.Create(cs, rc, this, IDC_CMB_PFA);
    m_cmbPfa.AddString(_T("0.01  (1 in 100)"));
    m_cmbPfa.AddString(_T("0.001  (1 in 1000)"));
    m_cmbPfa.AddString(_T("0.0001  (1 in 10 000)"));
    m_cmbPfa.AddString(_T("0.00001  (1 in 100 000)"));
    m_cmbPfa.AddString(_T("0.000001  (1 in 1 000 000)"));
    m_lblCfarGuard.Create(_T("CFAR guard cells"), ss, rc, this);  m_edtCfarGuard.Create(es, rc, this, IDC_EDT_CFAR_GUARD);
    m_lblCfarTrain.Create(_T("CFAR training cells"), ss, rc, this); m_edtCfarTrain.Create(es, rc, this, IDC_EDT_CFAR_TRAIN);
    m_lblInterp.Create(_T("Peak interpolation"), ss, rc, this);
    m_cmbInterp.Create(cs, rc, this, IDC_CMB_INTERP);
    for (int i = 0; i < static_cast<int>(dsp::PeakInterp::Count); ++i)
        m_cmbInterp.AddString(CString(dsp::PeakInterpName(static_cast<dsp::PeakInterp>(i))));
    m_lblMaxPeaks.Create(_T("Max targets"), ss, rc, this);        m_edtMaxPeaks.Create(es, rc, this, IDC_EDT_MAXPEAKS);
    m_lblMti.Create(_T("Doppler"), ss, rc, this);                 m_chkMti.Create(_T("MTI (subtract mean)"), chk, rc, this, IDC_CHK_MTI);
    m_lblTrack.Create(_T("Tracking"), ss, rc, this);              m_chkTrack.Create(_T("Stable target IDs"), chk, rc, this, IDC_CHK_TRACK);

    m_hdrDisplay.Create(_T("Display"), ss, rc, this);
    m_lblDbTop.Create(_T("dB axis top"), ss, rc, this);           m_edtDbTop.Create(esf, rc, this, IDC_EDT_DBTOP);
    m_lblDbBottom.Create(_T("dB axis bottom"), ss, rc, this);     m_edtDbBottom.Create(esf, rc, this, IDC_EDT_DBBOTTOM);
    m_lblWfRows.Create(_T("Waterfall rows"), ss, rc, this);       m_edtWfRows.Create(es, rc, this, IDC_EDT_WF_ROWS);
    m_lblAdcBits.Create(_T("ADC bits"), ss, rc, this);            m_edtAdcBits.Create(es, rc, this, IDC_EDT_ADCBITS);
    m_lblVref.Create(_T("ADC full scale, V"), ss, rc, this);      m_edtVref.Create(esf, rc, this, IDC_EDT_VREF);
    m_btnDefaults.Create(_T("Defaults"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, rc, this, IDC_BTN_DEFAULTS);

    CWnd* pw = GetWindow(GW_CHILD);
    while (pw) { pw->SetFont(&m_font); pw = pw->GetWindow(GW_HWNDNEXT); }
    for (CStatic* h : { &m_hdrProc, &m_hdrDetect, &m_hdrDisplay }) h->SetFont(&m_hdrFont);

    AppSettings defaults;
    ApplySettings(defaults);
    return 0;
}

void ProcPanel::ApplyTheme()
{
    if (m_bgBrush.GetSafeHandle()) m_bgBrush.DeleteObject();
    m_bgBrush.CreateSolidBrush(Theme::Get().bg);
    Invalidate();
    CWnd* pw = GetWindow(GW_CHILD);
    while (pw) { pw->Invalidate(); pw = pw->GetWindow(GW_HWNDNEXT); }
}

// ---------------------------------------------------------------------------
int ProcPanel::GetInt(const CEdit& e, int def)
{
    CString s; const_cast<CEdit&>(e).GetWindowText(s); s.Trim();
    return s.IsEmpty() ? def : _ttoi(s);
}

double ProcPanel::GetDouble(const CEdit& e, double def)
{
    CString s; const_cast<CEdit&>(e).GetWindowText(s); s.Trim();
    if (s.IsEmpty()) return def;
    s.Replace(_T(','), _T('.'));
    return _tcstod(s, nullptr);
}

void ProcPanel::SetInt(CEdit& e, long long v) { CString s; s.Format(_T("%lld"), v); e.SetWindowText(s); }
void ProcPanel::SetDouble(CEdit& e, double v, LPCTSTR fmt) { CString s; s.Format(fmt, v); e.SetWindowText(s); }

void ProcPanel::ApplySettings(const AppSettings& s)
{
    m_last = s;
    m_suppress = true;
    m_rdoSrcRaw.SetCheck(s.dsp.useMcuFft ? BST_UNCHECKED : BST_CHECKED);
    m_rdoSrcMcu.SetCheck(s.dsp.useMcuFft ? BST_CHECKED : BST_UNCHECKED);
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
    m_chkFwGeom.SetCheck(s.dsp.firmwareGeometry ? BST_CHECKED : BST_UNCHECKED);
    m_chkTone.SetCheck(s.dsp.toneEstimate ? BST_CHECKED : BST_UNCHECKED);

    m_cmbDetector.SetCurSel(static_cast<int>(s.dsp.detector.type));
    SetDouble(m_edtThresh, s.dsp.detector.thresholdDb, _T("%.1f"));
    { double p = s.dsp.detector.pfa; m_cmbPfa.SetCurSel((p >= 5e-3) ? 0 : (p >= 5e-4) ? 1 : (p >= 5e-5) ? 2 : (p >= 5e-6) ? 3 : 4); }
    SetInt(m_edtCfarGuard, s.dsp.detector.guardCells);
    SetInt(m_edtCfarTrain, s.dsp.detector.trainCells);
    m_cmbInterp.SetCurSel(static_cast<int>(s.dsp.interp));
    SetInt(m_edtMaxPeaks, s.dsp.maxPeaks);
    m_chkMti.SetCheck(s.dsp.mti ? BST_CHECKED : BST_UNCHECKED);
    m_chkTrack.SetCheck(s.dsp.trackTargets ? BST_CHECKED : BST_UNCHECKED);

    SetDouble(m_edtDbTop, s.display.dbTop, _T("%.0f"));
    SetDouble(m_edtDbBottom, s.display.dbBottom, _T("%.0f"));
    SetInt(m_edtWfRows, s.display.waterfallRows);
    SetInt(m_edtAdcBits, s.display.adcBits);
    SetDouble(m_edtVref, s.display.vRef, _T("%.3f"));
    m_suppress = false;
}

void ProcPanel::ReadInto(AppSettings& s) const
{
    s.dsp.useMcuFft  = m_rdoSrcMcu.GetCheck() == BST_CHECKED;
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
    s.dsp.firmwareGeometry = m_chkFwGeom.GetCheck() == BST_CHECKED;
    s.dsp.toneEstimate     = m_chkTone.GetCheck() == BST_CHECKED;

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
    s.dsp.maxPeaks     = (std::max)(1, (std::min)(64, GetInt(m_edtMaxPeaks, 10)));
    s.dsp.mti          = m_chkMti.GetCheck() == BST_CHECKED;
    s.dsp.trackTargets = m_chkTrack.GetCheck() == BST_CHECKED;

    s.display.dbTop    = static_cast<float>(GetDouble(m_edtDbTop, 0.0));
    s.display.dbBottom = static_cast<float>(GetDouble(m_edtDbBottom, -120.0));
    if (s.display.dbTop <= s.display.dbBottom) { s.display.dbTop = 0.0f; s.display.dbBottom = -120.0f; }
    s.display.waterfallRows = (std::max)(64, (std::min)(4096, GetInt(m_edtWfRows, 512)));
    s.display.adcBits   = (std::max)(8, (std::min)(16, GetInt(m_edtAdcBits, 12)));
    s.display.vRef      = static_cast<float>(GetDouble(m_edtVref, 3.3));
}

void ProcPanel::NotifyChanged()
{
    if (m_suppress) return;
    CWnd* pMain = AfxGetMainWnd();
    if (pMain && ::IsWindow(pMain->GetSafeHwnd())) pMain->PostMessage(WM_APP_SETTINGS_CHANGED, SETTINGS_FROM_PROC, 0);
}

void ProcPanel::OnDefaults()
{
    AppSettings d = m_last;       // keep everything that is not owned by this panel
    AppSettings fresh;
    d.dsp = fresh.dsp;
    d.dsp.chirpsInFrame = m_last.dsp.chirpsInFrame;
    d.dsp.shape         = m_last.dsp.shape;
    d.display.dbTop = fresh.display.dbTop; d.display.dbBottom = fresh.display.dbBottom;
    d.display.waterfallRows = fresh.display.waterfallRows;
    d.display.adcBits = fresh.display.adcBits; d.display.vRef = fresh.display.vRef;
    ApplySettings(d);
    NotifyChanged();
}

// ---------------------------------------------------------------------------
void ProcPanel::OnSize(UINT, int, int) { Relayout(); }

void ProcPanel::Relayout()
{
    if (!m_lblGuard.GetSafeHwnd()) return;
    const int S = static_cast<int>(Dpi::Of(m_hWnd));
    auto sc = [&](int px) { return ::MulDiv(px, S, 96); };
    CRect rc; GetClientRect(&rc);

    const int lblW = sc(138), pad = sc(6), h = sc(22), rowH = sc(26);
    const int x = sc(8);
    const int ctlW = (std::max)(sc(60), rc.Width() - x - lblW - pad - sc(8));
    int y = sc(6) - m_scroll;

    auto header = [&](CStatic& hdr) { hdr.MoveWindow(x, y, lblW + pad + ctlW, sc(20)); y += sc(24); };
    auto row = [&](CStatic& lbl, CWnd& ctl, int ctlH = 0, int w = 0) {
        lbl.MoveWindow(x, y, lblW, h);
        ctl.MoveWindow(x + lblW + pad, y, w ? w : ctlW, ctlH ? ctlH : h);
        y += rowH;
    };

    header(m_hdrProc);
    m_lblSource.MoveWindow(x, y, lblW, h);
    m_rdoSrcRaw.MoveWindow(x + lblW + pad, y, sc(84), h);
    m_rdoSrcMcu.MoveWindow(x + lblW + pad + sc(86), y, (std::max)(sc(40), ctlW - sc(86)), h);
    y += rowH;
    row(m_lblGuard, m_edtGuard);
    row(m_lblDetrend, m_chkDetrend);
    row(m_lblDecim, m_cmbDecim, sc(240));
    row(m_lblMaxRange, m_edtMaxRange);
    row(m_lblWindow, m_cmbWindow, sc(200));
    row(m_lblKaiser, m_edtKaiser);
    row(m_lblZeroPad, m_cmbZeroPad, sc(120));
    row(m_lblRangeGain, m_cmbRangeGain, sc(120));
    row(m_lblFwGeom, m_chkFwGeom);
    row(m_lblTone, m_chkTone);
    y += sc(4);
    header(m_hdrDetect);
    row(m_lblDetector, m_cmbDetector, sc(120));
    row(m_lblThresh, m_edtThresh);
    row(m_lblPfa, m_cmbPfa, sc(150));
    row(m_lblCfarGuard, m_edtCfarGuard);
    row(m_lblCfarTrain, m_edtCfarTrain);
    row(m_lblInterp, m_cmbInterp, sc(150));
    row(m_lblMaxPeaks, m_edtMaxPeaks);
    row(m_lblMti, m_chkMti);
    row(m_lblTrack, m_chkTrack);
    y += sc(4);
    header(m_hdrDisplay);
    row(m_lblDbTop, m_edtDbTop);
    row(m_lblDbBottom, m_edtDbBottom);
    row(m_lblWfRows, m_edtWfRows);
    row(m_lblAdcBits, m_edtAdcBits);
    row(m_lblVref, m_edtVref);
    m_btnDefaults.MoveWindow(x + lblW + pad, y, sc(80), h);
    y += rowH + sc(6);

    m_contentH = y + m_scroll;
    SCROLLINFO si{}; si.cbSize = sizeof(si); si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    si.nMin = 0; si.nMax = (std::max)(0, m_contentH - 1); si.nPage = static_cast<UINT>(rc.Height()); si.nPos = m_scroll;
    SetScrollInfo(SB_VERT, &si, TRUE);
    const int maxScroll = (std::max)(0, m_contentH - rc.Height());
    if (m_scroll > maxScroll) { m_scroll = maxScroll; Relayout(); return; }
    Invalidate();
}

void ProcPanel::SetScrollPos(int pos)
{
    CRect rc; GetClientRect(&rc);
    const int maxScroll = (std::max)(0, m_contentH - rc.Height());
    pos = (std::max)(0, (std::min)(maxScroll, pos));
    if (pos == m_scroll) return;
    m_scroll = pos;
    Relayout();
}

void ProcPanel::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar*)
{
    CRect rc; GetClientRect(&rc);
    const int step = Dpi::Scale(m_hWnd, 26);
    switch (nSBCode) {
    case SB_LINEUP:        SetScrollPos(m_scroll - step); break;
    case SB_LINEDOWN:      SetScrollPos(m_scroll + step); break;
    case SB_PAGEUP:        SetScrollPos(m_scroll - rc.Height()); break;
    case SB_PAGEDOWN:      SetScrollPos(m_scroll + rc.Height()); break;
    case SB_THUMBTRACK:
    case SB_THUMBPOSITION: SetScrollPos(static_cast<int>(nPos)); break;
    case SB_TOP:           SetScrollPos(0); break;
    case SB_BOTTOM:        SetScrollPos(m_contentH); break;
    default: break;
    }
}

BOOL ProcPanel::OnMouseWheel(UINT, short zDelta, CPoint)
{
    SetScrollPos(m_scroll - (zDelta / WHEEL_DELTA) * Dpi::Scale(m_hWnd, 3 * 26));
    return TRUE;
}

BOOL ProcPanel::OnEraseBkgnd(CDC* pDC)
{
    CRect rc; GetClientRect(&rc);
    pDC->FillSolidRect(rc, Theme::Get().bg);
    return TRUE;
}

HBRUSH ProcPanel::OnCtlColor(CDC* pDC, CWnd*, UINT nCtlColor)
{
    if (nCtlColor == CTLCOLOR_STATIC || nCtlColor == CTLCOLOR_BTN) {
        pDC->SetBkMode(TRANSPARENT);
        pDC->SetTextColor(Theme::Get().text);
        return static_cast<HBRUSH>(m_bgBrush.GetSafeHandle());
    }
    pDC->SetTextColor(Theme::Get().text);
    pDC->SetBkColor(Theme::Get().plot);
    return Theme::FieldBrush();
}
