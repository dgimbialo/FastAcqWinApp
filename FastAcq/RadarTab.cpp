#include "pch.h"
#include "RadarTab.h"
#include "AppMessages.h"
#include "Dpi.h"
#include "Theme.h"
#include "resource.h"

BEGIN_MESSAGE_MAP(RadarTab, CWnd)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
    ON_WM_CTLCOLOR()
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_WM_MOUSEMOVE()
    ON_WM_SETCURSOR()
    ON_CBN_SELCHANGE(IDC_CMB_TRACE,      &RadarTab::OnFooterChanged)
    ON_CBN_SELCHANGE(IDC_CMB_WF_PALETTE, &RadarTab::OnFooterChanged)
    ON_BN_CLICKED(IDC_CHK_SHOW_UP,       &RadarTab::OnFooterChanged)
    ON_BN_CLICKED(IDC_CHK_SHOW_DN,       &RadarTab::OnFooterChanged)
    ON_BN_CLICKED(IDC_CHK_SHOW_THR,      &RadarTab::OnFooterChanged)
    ON_BN_CLICKED(IDC_CHK_SHOW_NOISE,    &RadarTab::OnFooterChanged)
    ON_BN_CLICKED(IDC_CHK_SHOW_RD,       &RadarTab::OnFooterChanged)
    ON_BN_CLICKED(IDC_BTN_RESET_AVG,     &RadarTab::OnResetAvg)
    ON_BN_CLICKED(IDC_BTN_AUTOSCALE,     &RadarTab::OnAutoscale)
    ON_BN_CLICKED(IDC_BTN_CLEAR_WF,      &RadarTab::OnClearWf)
    ON_MESSAGE(WM_APP_XRANGE_CHANGED,    &RadarTab::OnXRangeChanged)
    ON_MESSAGE(WM_APP_LEARN_SPURS,       &RadarTab::OnLearnSpurs)
END_MESSAGE_MAP()

BOOL RadarTab::CreateTab(CWnd* parent, UINT id)
{
    LPCTSTR cls = AfxRegisterWndClass(0, ::LoadCursor(nullptr, IDC_ARROW), nullptr, nullptr);
    return Create(cls, nullptr, WS_CHILD | WS_CLIPCHILDREN, CRect(0, 0, 10, 10), parent, id);
}

int RadarTab::OnCreate(LPCREATESTRUCT lpcs)
{
    if (CWnd::OnCreate(lpcs) == -1) return -1;
    m_bgBrush.CreateSolidBrush(Theme::Get().bg);
    Dpi::MakeFont(m_font, m_hWnd, 9);

    m_profile.CreatePlot(this, IDC_RP_VIEW);
    m_waterfall.CreatePlot(this, IDC_WF_VIEW);
    m_rd.CreatePlot(this, IDC_RD_VIEW);
    m_rd.ShowWindow(SW_HIDE);
    m_targets.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT | LVS_SINGLESEL | LVS_NOSORTHEADER,
                     CRect(0, 0, 10, 10), this, IDC_TARGET_LIST);
    m_targets.SetFont(&m_font);
    m_targets.Init();
    m_proc.CreatePanel(this, IDC_PROC_PANEL);

    const DWORD ss  = WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE;
    const DWORD cs  = WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL;
    const DWORD chk = WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX;
    const DWORD bs  = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON;
    CRect rc(0, 0, 10, 10);

    m_lblTrace.Create(_T("Trace:"), ss, rc, this);
    m_cmbTrace.Create(cs, rc, this, IDC_CMB_TRACE);
    for (int i = 0; i < static_cast<int>(dsp::TraceMode::Count); ++i)
        m_cmbTrace.AddString(CString(dsp::TraceModeName(static_cast<dsp::TraceMode>(i))));
    m_cmbTrace.SetCurSel(0);
    m_lblPalette.Create(_T("Palette:"), ss, rc, this);
    m_cmbPalette.Create(cs, rc, this, IDC_CMB_WF_PALETTE);
    for (int i = 0; i < static_cast<int>(Palette::Count); ++i)
        m_cmbPalette.AddString(ColorMap::Name(static_cast<Palette>(i)));
    m_cmbPalette.SetCurSel(1);
    m_chkUp.Create(_T("UP"), chk, rc, this, IDC_CHK_SHOW_UP);
    m_chkDn.Create(_T("DOWN"), chk, rc, this, IDC_CHK_SHOW_DN);
    m_chkThr.Create(_T("Threshold"), chk, rc, this, IDC_CHK_SHOW_THR);
    m_chkNoise.Create(_T("Noise"), chk, rc, this, IDC_CHK_SHOW_NOISE);
    m_chkRd.Create(_T("Range-Doppler"), chk, rc, this, IDC_CHK_SHOW_RD);
    m_btnResetAvg.Create(_T("Reset avg"), bs, rc, this, IDC_BTN_RESET_AVG);
    m_btnAutoscale.Create(_T("Autoscale"), bs, rc, this, IDC_BTN_AUTOSCALE);
    m_btnClearWf.Create(_T("Clear WF"), bs, rc, this, IDC_BTN_CLEAR_WF);
    m_lblPhase.Create(_T(""), WS_CHILD | WS_VISIBLE | SS_RIGHT | SS_CENTERIMAGE, rc, this, IDC_LBL_PHASE);

    CWnd* kids[] = { &m_lblTrace, &m_cmbTrace, &m_lblPalette, &m_cmbPalette, &m_chkUp, &m_chkDn, &m_chkThr,
                     &m_chkNoise, &m_chkRd, &m_btnResetAvg, &m_btnAutoscale, &m_btnClearWf, &m_lblPhase };
    for (auto* k : kids) k->SetFont(&m_font);

    m_profile.SetTitle(_T("Range profile"));
    m_waterfall.SetTitle(_T("Waterfall (range - time)"));
    return 0;
}

void RadarTab::ApplyTheme()
{
    m_proc.ApplyTheme();
    if (m_bgBrush.GetSafeHandle()) m_bgBrush.DeleteObject();
    m_bgBrush.CreateSolidBrush(Theme::Get().bg);
    m_targets.ApplyTheme();
    Invalidate();
    m_profile.Invalidate(FALSE);
    m_waterfall.Invalidate(FALSE);
    m_rd.Invalidate(FALSE);
}

void RadarTab::ApplySettings(const AppSettings& s)
{
    m_suppress = true;
    m_disp = s.display;
    m_showRd = s.display.showRangeDoppler;
    m_profile.SetDisplay(s.display);
    m_maxRangeM = s.dsp.maxRangeM;
    m_profile.SetMaxRangeM(s.dsp.maxRangeM);
    m_proc.ApplySettings(s);
    m_waterfall.SetDisplay(s.display);
    m_waterfall.SetMaxRangeM(s.dsp.maxRangeM);
    m_rd.SetDisplay(s.display);
    m_cmbTrace.SetCurSel(static_cast<int>(s.dsp.trace));
    m_cmbPalette.SetCurSel(s.display.palette);
    m_chkUp.SetCheck(s.display.showUp ? BST_CHECKED : BST_UNCHECKED);
    m_chkDn.SetCheck(s.display.showDown ? BST_CHECKED : BST_UNCHECKED);
    m_chkThr.SetCheck(s.display.showThreshold ? BST_CHECKED : BST_UNCHECKED);
    m_chkNoise.SetCheck(s.display.showNoise ? BST_CHECKED : BST_UNCHECKED);
    m_chkRd.SetCheck(s.display.showRangeDoppler ? BST_CHECKED : BST_UNCHECKED);
    m_suppress = false;
    Relayout();
}

void RadarTab::ReadFooter(AppSettings& s) const
{
    int t = m_cmbTrace.GetCurSel();
    if (t >= 0 && t < static_cast<int>(dsp::TraceMode::Count)) s.dsp.trace = static_cast<dsp::TraceMode>(t);
    int p = m_cmbPalette.GetCurSel();
    if (p >= 0 && p < static_cast<int>(Palette::Count)) s.display.palette = p;
    s.display.showUp           = m_chkUp.GetCheck() == BST_CHECKED;
    s.display.showDown         = m_chkDn.GetCheck() == BST_CHECKED;
    s.display.showThreshold    = m_chkThr.GetCheck() == BST_CHECKED;
    s.display.showNoise        = m_chkNoise.GetCheck() == BST_CHECKED;
    s.display.showRangeDoppler = m_chkRd.GetCheck() == BST_CHECKED;
    if (m_maxRangeM > 0.0) s.dsp.maxRangeM = m_maxRangeM;
}

// Zooming out past what the current decimation computed asks the DSP for a
// larger range of interest (lower decimation); zooming in hands the range
// back so processing stays fast. The zoom itself is left untouched, so the
// view does not jump when the next frame arrives with the new grid.
void RadarTab::FollowZoomWithRangeOfInterest()
{
    if (!m_profile.HasZoom() || !m_res || !m_res->valid || m_res->rangePerHz <= 0.0) return;
    double f0 = 0.0, f1 = 0.0;
    m_profile.GetXRange(f0, f1);
    if (f1 <= f0) return;
    const double wanted = (std::max)(0.5, f1 * m_res->rangePerHz);
    if (std::fabs(wanted - m_maxRangeM) <= 0.05 * m_maxRangeM) return;   // same decimation anyway
    m_maxRangeM = wanted;
    m_profile.SetMaxRangeM(wanted);
    m_waterfall.SetMaxRangeM(wanted);
    PostSettingsChanged();
}

void RadarTab::PostSettingsChanged()
{
    if (m_suppress) return;
    CWnd* main = AfxGetMainWnd();
    if (main && ::IsWindow(main->GetSafeHwnd()))
        main->PostMessage(WM_APP_SETTINGS_CHANGED, SETTINGS_FROM_RADAR, 0);
}

void RadarTab::OnFooterChanged() { PostSettingsChanged(); }

void RadarTab::OnResetAvg()
{
    CWnd* main = AfxGetMainWnd();
    if (main && ::IsWindow(main->GetSafeHwnd())) main->PostMessage(WM_APP_RESET_AVERAGES, 0, 0);
}

void RadarTab::OnAutoscale()
{
    m_profile.AutoscaleDb();
    m_waterfall.AutoscaleDb();
}

void RadarTab::OnClearWf() { m_waterfall.Clear(); }

void RadarTab::ClearHistory()
{
    m_waterfall.Clear();
    m_res.reset();
    m_profile.SetResult(nullptr);
    m_rd.SetResult(nullptr);
    m_targets.SetTargets({}, false, false);
    m_lblPhase.SetWindowText(_T(""));
    Relayout();
}

void RadarTab::ResetZoom()     { m_profile.ResetZoom(); m_waterfall.ResetZoom(); }
void RadarTab::ClearMarkers()  { m_profile.ClearMarkers(); }

void RadarTab::SetSplits(float a, float b)
{
    if (a > 0.1f && a < 0.9f) m_split1 = a;
    if (b > m_split1 + 0.05f && b < 0.95f) m_split2 = b;
    Relayout();
}

void RadarTab::ShowResult(std::shared_ptr<const dsp::FrameResult> r, bool pushWaterfall)
{
    m_res = std::move(r);
    m_profile.SetResult(m_res);
    m_rd.SetResult(m_res);
    if (m_res) {
        if (pushWaterfall) m_waterfall.PushResult(*m_res);
        m_targets.SetTargets(m_res->targets, m_res->rangePerHz > 0.0,
                             m_res->shape == dsp::RampShape::Triangle && m_res->down.valid);
        CString ph;
        if (m_res->phase.valid)
            ph.Format(_T("Phase @ %s: %+.3f mm"), RangeProfileView::FormatFreq(m_res->phase.freqHz).GetString(),
                      m_res->phase.displacementMm);
        m_lblPhase.SetWindowText(ph);
    }
    const bool rdNow = m_showRd && m_rd.HasData();
    if (rdNow != m_rdVisible) Relayout();
}

LRESULT RadarTab::OnXRangeChanged(WPARAM src, LPARAM)
{
    double f0, f1;
    if (src == IDC_RP_VIEW) { m_profile.GetXRange(f0, f1); m_waterfall.SetXRange(m_profile.HasZoom() ? f0 : 0.0, m_profile.HasZoom() ? f1 : 0.0); }
    else if (src == IDC_WF_VIEW) { m_waterfall.GetXRange(f0, f1); m_profile.SetXRange(f0, f1); }
    FollowZoomWithRangeOfInterest();
    return 0;
}

// "Learn spurs": every peak currently detected on the UP and DOWN spectra is
// added to the spur mask (use it with no target in front of the antenna, so
// that the fixed spurious lines are masked afterwards).
LRESULT RadarTab::OnLearnSpurs(WPARAM, LPARAM)
{
    if (!m_res || !m_res->valid) return 0;
    std::vector<double> hz;
    double halfWidth = 2000.0;
    for (const dsp::RampSpectrum* sp : { &m_res->up, &m_res->down }) {
        if (!sp->valid) continue;
        for (const auto& pk : sp->peaks) hz.push_back(pk.freqHz);
        if (sp->freqResHz > 0.0) halfWidth = (std::max)(halfWidth, 2.0 * sp->freqResHz);
    }
    m_proc.AddSpurs(hz, halfWidth);
    return 0;
}

// ---------------------------------------------------------------------------
void RadarTab::OnSize(UINT, int, int) { Relayout(); }

void RadarTab::Relayout()
{
    if (!m_profile.GetSafeHwnd()) return;
    CRect rc; GetClientRect(&rc);
    const int S = static_cast<int>(Dpi::Of(m_hWnd));
    auto sc = [&](int px) { return ::MulDiv(px, S, 96); };
    const int splitH = sc(kSplitH96), footerH = sc(kFooterH96), minPane = sc(kMinPane96);
    const int cxAll = rc.Width(), cy = rc.Height();
    // Settings column on the right; the plots take the rest.
    const int panelW = sc(ProcPanel::kWidth96);
    const int cx = (std::max)(50, cxAll - panelW - sc(4));
    if (m_proc.GetSafeHwnd()) m_proc.MoveWindow(cx + sc(4), 0, cxAll - cx - sc(4), cy);
    if (cx < 50 || cy < footerH + 3 * minPane) return;

    const int avail = cy - footerH - 2 * splitH;
    int h1 = static_cast<int>(avail * m_split1);
    int h2 = static_cast<int>(avail * m_split2) - h1;
    if (h1 < minPane) h1 = minPane;
    if (h2 < minPane) h2 = minPane;
    if (h1 + h2 > avail - minPane) h2 = avail - minPane - h1;
    if (h2 < minPane) { h2 = minPane; h1 = avail - minPane - h2; }
    const int h3 = avail - h1 - h2;

    int y = 0;
    m_profile.MoveWindow(0, y, cx, h1);
    y += h1;
    m_rcSplit1 = CRect(0, y, cx, y + splitH);
    y += splitH;

    m_rdVisible = m_showRd && m_rd.HasData();
    if (m_rdVisible) {
        const int wfW = cx * 60 / 100;
        m_waterfall.MoveWindow(0, y, wfW - sc(2), h2);
        m_rd.MoveWindow(wfW, y, cx - wfW, h2);
        m_rd.ShowWindow(SW_SHOW);
    } else {
        m_waterfall.MoveWindow(0, y, cx, h2);
        m_rd.ShowWindow(SW_HIDE);
    }
    y += h2;
    m_rcSplit2 = CRect(0, y, cx, y + splitH);
    y += splitH;
    m_targets.MoveWindow(0, y, cx, h3);
    y += h3;

    // Footer.
    const int fy = y + (footerH - sc(22)) / 2;
    const int h = sc(22);
    int x = sc(6);
    auto place = [&](CWnd& w, int wpx, int extraH = 0) {
        if (w.GetSafeHwnd()) w.MoveWindow(x, fy, sc(wpx), extraH ? extraH : h);
        x += sc(wpx) + sc(4);
    };
    place(m_lblTrace, 40);
    place(m_cmbTrace, 95, sc(200));
    x += sc(6);
    place(m_lblPalette, 48);
    place(m_cmbPalette, 80, sc(200));
    x += sc(6);
    place(m_chkUp, 44);
    place(m_chkDn, 62);
    place(m_chkThr, 80);
    place(m_chkNoise, 58);
    place(m_chkRd, 110);
    x += sc(6);
    place(m_btnResetAvg, 78);
    place(m_btnAutoscale, 78);
    place(m_btnClearWf, 72);
    if (m_lblPhase.GetSafeHwnd()) m_lblPhase.MoveWindow(x, fy, (std::max)(10, cx - x - sc(6)), h);
    Invalidate();
}

void RadarTab::OnPaint()
{
    CPaintDC dc(this);
    const Theme::Palette& th = Theme::Get();
    for (const CRect* r : { &m_rcSplit1, &m_rcSplit2 }) {
        dc.FillSolidRect(*r, th.bg);
        const int midY = r->top + r->Height() / 2;
        const int midX = r->left + r->Width() / 2;
        for (int i = -3; i <= 3; ++i)
            dc.FillSolidRect(midX + i * 6 - 1, midY - 1, 3, 3, th.border);
    }
}

BOOL RadarTab::OnEraseBkgnd(CDC* pDC)
{
    CRect rc; GetClientRect(&rc);
    pDC->FillSolidRect(rc, Theme::Get().bg);
    return TRUE;
}

HBRUSH RadarTab::OnCtlColor(CDC* pDC, CWnd*, UINT nCtlColor)
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

int RadarTab::HitSplitter(CPoint pt) const
{
    if (m_rcSplit1.PtInRect(pt)) return 1;
    if (m_rcSplit2.PtInRect(pt)) return 2;
    return 0;
}

void RadarTab::OnLButtonDown(UINT, CPoint pt)
{
    m_dragging = HitSplitter(pt);
    if (m_dragging) { SetCapture(); ::SetCursor(::LoadCursor(nullptr, IDC_SIZENS)); }
}

void RadarTab::OnLButtonUp(UINT, CPoint)
{
    if (m_dragging) { m_dragging = 0; ReleaseCapture(); }
}

void RadarTab::OnMouseMove(UINT, CPoint pt)
{
    if (!m_dragging) { if (HitSplitter(pt)) ::SetCursor(::LoadCursor(nullptr, IDC_SIZENS)); return; }
    CRect rc; GetClientRect(&rc);
    const int S = static_cast<int>(Dpi::Of(m_hWnd));
    const int splitH = ::MulDiv(kSplitH96, S, 96), footerH = ::MulDiv(kFooterH96, S, 96);
    const int avail = rc.Height() - footerH - 2 * splitH;
    if (avail < 50) return;
    float f = static_cast<float>(pt.y) / static_cast<float>(avail);
    if (m_dragging == 1) { if (f < 0.1f) f = 0.1f; if (f > m_split2 - 0.08f) f = m_split2 - 0.08f; m_split1 = f; }
    else                 { if (f < m_split1 + 0.08f) f = m_split1 + 0.08f; if (f > 0.95f) f = 0.95f; m_split2 = f; }
    Relayout();
}

BOOL RadarTab::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT msg)
{
    CPoint pt; ::GetCursorPos(&pt); ScreenToClient(&pt);
    if (HitSplitter(pt)) { ::SetCursor(::LoadCursor(nullptr, IDC_SIZENS)); return TRUE; }
    return CWnd::OnSetCursor(pWnd, nHitTest, msg);
}
