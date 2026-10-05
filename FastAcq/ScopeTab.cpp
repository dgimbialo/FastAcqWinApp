#include "pch.h"
#include "ScopeTab.h"
#include "AppMessages.h"
#include "Dpi.h"
#include "Theme.h"
#include "resource.h"

BEGIN_MESSAGE_MAP(ScopeTab, CWnd)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
    ON_WM_CTLCOLOR()
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_WM_MOUSEMOVE()
    ON_WM_SETCURSOR()
    ON_BN_CLICKED(IDC_CHK_DOTS,    &ScopeTab::OnFooterChanged)
    ON_BN_CLICKED(IDC_RDO_VOLTS,   &ScopeTab::OnFooterChanged)
    ON_BN_CLICKED(IDC_RDO_CODES,   &ScopeTab::OnFooterChanged)
    ON_CBN_SELCHANGE(IDC_CMB_RAMP_SEL, &ScopeTab::OnRampSelChanged)
END_MESSAGE_MAP()

BOOL ScopeTab::CreateTab(CWnd* parent, UINT id)
{
    LPCTSTR cls = AfxRegisterWndClass(0, ::LoadCursor(nullptr, IDC_ARROW), nullptr, nullptr);
    return Create(cls, nullptr, WS_CHILD | WS_CLIPCHILDREN, CRect(0, 0, 10, 10), parent, id);
}

int ScopeTab::OnCreate(LPCREATESTRUCT lpcs)
{
    if (CWnd::OnCreate(lpcs) == -1) return -1;
    m_bgBrush.CreateSolidBrush(Theme::Get().bg);
    Dpi::MakeFont(m_font, m_hWnd, 9);

    m_frame.CreateView(this, IDC_WAVE_FRAME);
    m_ramp.CreateView(this, IDC_WAVE_RAMP);
    m_frame.SetTitle(_T("Frame (all chirps)"));
    m_ramp.SetTitle(_T("Ramp detail"));

    CRect rc(0, 0, 10, 10);
    m_chkDots.Create(_T("Dots"), WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, rc, this, IDC_CHK_DOTS);
    m_rdoVolts.Create(_T("Volts"), WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP, rc, this, IDC_RDO_VOLTS);
    m_rdoCodes.Create(_T("ADC codes"), WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON, rc, this, IDC_RDO_CODES);
    m_rdoVolts.SetCheck(BST_CHECKED);
    m_lblRamp.Create(_T("Ramp:"), WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE, rc, this);
    m_cmbRamp.Create(WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, rc, this, IDC_CMB_RAMP_SEL);
    m_lblInfo.Create(_T(""), WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE | SS_ENDELLIPSIS, rc, this, IDC_LBL_SCOPE_INFO);
    CWnd* kids[] = { &m_chkDots, &m_rdoVolts, &m_rdoCodes, &m_lblRamp, &m_cmbRamp, &m_lblInfo };
    for (auto* k : kids) k->SetFont(&m_font);
    return 0;
}

void ScopeTab::ApplyTheme()
{
    if (m_bgBrush.GetSafeHandle()) m_bgBrush.DeleteObject();
    m_bgBrush.CreateSolidBrush(Theme::Get().bg);
    Invalidate();
    m_frame.Invalidate(FALSE);
    m_ramp.Invalidate(FALSE);
}

void ScopeTab::ApplySettings(const AppSettings& s)
{
    m_suppress = true;
    m_chkDots.SetCheck(s.display.dots ? BST_CHECKED : BST_UNCHECKED);
    m_rdoVolts.SetCheck(s.display.showVolts ? BST_CHECKED : BST_UNCHECKED);
    m_rdoCodes.SetCheck(s.display.showVolts ? BST_UNCHECKED : BST_CHECKED);
    m_suppress = false;
    for (WaveformView* v : { &m_frame, &m_ramp }) {
        v->SetDotsMode(s.display.dots);
        v->SetAdcConfig(s.display.adcBits, s.display.vRef, s.display.showVolts);
        v->SetSampleRate(static_cast<double>(s.sampleRateCalHz));
    }
}

void ScopeTab::ReadFooter(AppSettings& s) const
{
    s.display.dots      = m_chkDots.GetCheck() == BST_CHECKED;
    s.display.showVolts = m_rdoVolts.GetCheck() == BST_CHECKED;
}

void ScopeTab::OnFooterChanged()
{
    if (m_suppress) return;
    CWnd* main = AfxGetMainWnd();
    if (main && ::IsWindow(main->GetSafeHwnd()))
        main->PostMessage(WM_APP_SETTINGS_CHANGED, SETTINGS_FROM_SCOPE, 0);
}

void ScopeTab::ResetZoom()     { m_frame.ResetZoom(); m_ramp.ResetZoom(); }
void ScopeTab::ClearCursors()  { m_frame.ClearCursors(); m_ramp.ClearCursors(); }

void ScopeTab::RebuildRampCombo()
{
    const int oldSel = m_cmbRamp.GetCurSel();
    const int oldCount = m_cmbRamp.GetCount();
    std::vector<size_t> idx;
    if (m_res)
        for (size_t i = 0; i < m_res->segments.size(); ++i)
            if (m_res->segments[i].kind != dsp::Segment::Guard) idx.push_back(i);
    if (idx.size() == m_rampSegIdx.size() && static_cast<int>(idx.size()) == oldCount) { m_rampSegIdx = idx; return; }
    m_rampSegIdx = idx;
    m_cmbRamp.ResetContent();
    for (size_t i : m_rampSegIdx) {
        const dsp::Segment& sg = m_res->segments[i];
        CString s;
        s.Format(_T("%s #%d"), sg.kind == dsp::Segment::Up ? _T("UP") : _T("DOWN"), sg.chirp);
        m_cmbRamp.AddString(s);
    }
    if (m_cmbRamp.GetCount() > 0)
        m_cmbRamp.SetCurSel((oldSel >= 0 && oldSel < m_cmbRamp.GetCount()) ? oldSel : 0);
}

void ScopeTab::UpdateRampView()
{
    if (!m_f || !m_res) { m_ramp.SetSamples(nullptr, 0, 0); return; }
    const int sel = m_cmbRamp.GetCurSel();
    if (sel < 0 || static_cast<size_t>(sel) >= m_rampSegIdx.size()) {
        // No segmentation: show the whole frame.
        m_ramp.SetSamples(m_f->raw.data(), m_f->raw.size(), 0);
        m_ramp.SetSegments(m_res->segments);
        return;
    }
    const dsp::Segment& sg = m_res->segments[m_rampSegIdx[static_cast<size_t>(sel)]];
    size_t start = sg.start, end = sg.start + sg.length;
    // Include the adjacent guard intervals so the transients are visible.
    for (const auto& g : m_res->segments) {
        if (g.kind != dsp::Segment::Guard) continue;
        if (g.start + g.length == sg.start) start = g.start;
        if (g.start == sg.start + sg.length) end = g.start + g.length;
    }
    if (end > m_f->raw.size()) end = m_f->raw.size();
    if (start >= end) { m_ramp.SetSamples(nullptr, 0, 0); return; }
    m_ramp.SetSamples(m_f->raw.data() + start, end - start, start);
    m_ramp.SetSegments(m_res->segments);
    CString t;
    t.Format(_T("Ramp detail: %s #%d  (%zu samples)"), sg.kind == dsp::Segment::Up ? _T("UP") : _T("DOWN"), sg.chirp, end - start);
    m_ramp.SetTitle(t);
}

void ScopeTab::OnRampSelChanged() { UpdateRampView(); }

void ScopeTab::ShowFrame(ChirpFramePtr f, std::shared_ptr<const dsp::FrameResult> r)
{
    m_f   = std::move(f);
    m_res = std::move(r);
    if (!m_f) return;
    const double fs = (m_res && m_res->fsHz > 0.0) ? m_res->fsHz
                    : (m_f->header.sample_rate_hz > 0 ? m_f->header.sample_rate_hz : 0.0);
    if (fs > 0.0) { m_frame.SetSampleRate(fs); m_ramp.SetSampleRate(fs); }
    m_frame.SetSamples(m_f->raw.data(), m_f->raw.size(), 0);
    if (m_res) m_frame.SetSegments(m_res->segments); else m_frame.SetSegments({});
    CString title;
    title.Format(_T("Frame %u  (%zu samples, %d chirps)"), m_f->header.frame_id, m_f->raw.size(), m_res ? m_res->chirps : 1);
    m_frame.SetTitle(title);

    CString info;
    if (m_res && m_res->valid) {
        if (m_res->up.valid && !m_res->up.peaks.empty()) {
            CString s; s.Format(_T("UP f = %s"), WaveformView::FormatFreq(m_res->up.peaks[0].freqHz).GetString()); info += s;
        }
        if (m_res->down.valid && !m_res->down.peaks.empty()) {
            CString s; s.Format(_T("   DOWN f = %s"), WaveformView::FormatFreq(m_res->down.peaks[0].freqHz).GetString()); info += s;
        }
        if (!m_res->targets.empty() && m_res->rangePerHz > 0.0) {
            CString s; s.Format(_T("   R = %s"), WaveformView::FormatRange(m_res->targets[0].rangeM).GetString()); info += s;
            if (m_res->targets[0].paired) { CString v; v.Format(_T("  v = %+.2f m/s"), m_res->targets[0].velocityMps); info += v; }
        }
        if (m_res->up.tone.valid) {
            CString s; s.Format(_T("   tone UP %.1f Hz"), m_res->up.tone.freqHz); info += s;
            if (m_res->down.tone.valid) { CString t; t.Format(_T(" / DOWN %.1f Hz"), m_res->down.tone.freqHz); info += t; }
        }
        if (m_res->mcuPeakHz > 0.0) { CString s; s.Format(_T("   MCU peak %.1f Hz"), m_res->mcuPeakHz); info += s; }
        CString d; d.Format(_T("   decim x%d, fs_eff %s, ramp %s%s"), m_res->decimation,
                            WaveformView::FormatFreq(m_res->up.fsEffHz).GetString(), WaveformView::FormatTime(m_res->rampSec).GetString(),
                            m_res->geometryFromHeader ? _T(" (hdr)") : _T(""));
        info += d;
    }
    m_frame.SetInfo(info);
    m_lblInfo.SetWindowText(info);
    RebuildRampCombo();
    UpdateRampView();
}

// ---------------------------------------------------------------------------
void ScopeTab::OnSize(UINT, int, int) { Relayout(); }

void ScopeTab::Relayout()
{
    if (!m_frame.GetSafeHwnd()) return;
    CRect rc; GetClientRect(&rc);
    const int S = static_cast<int>(Dpi::Of(m_hWnd));
    auto sc = [&](int px) { return ::MulDiv(px, S, 96); };
    const int splitH = sc(kSplitH96), footerH = sc(kFooterH96);
    const int cx = rc.Width(), cy = rc.Height();
    if (cx < 50 || cy < footerH + 100) return;
    const int avail = cy - footerH - splitH;
    int h1 = static_cast<int>(avail * m_split);
    if (h1 < sc(40)) h1 = sc(40);
    if (h1 > avail - sc(40)) h1 = avail - sc(40);
    m_frame.MoveWindow(0, 0, cx, h1);
    m_rcSplit = CRect(0, h1, cx, h1 + splitH);
    m_ramp.MoveWindow(0, h1 + splitH, cx, avail - h1);

    const int fy = cy - footerH + (footerH - sc(22)) / 2;
    const int h = sc(22);
    int x = sc(6);
    auto place = [&](CWnd& w, int wpx, int extraH = 0) {
        if (w.GetSafeHwnd()) w.MoveWindow(x, fy, sc(wpx), extraH ? extraH : h);
        x += sc(wpx) + sc(4);
    };
    place(m_chkDots, 50);
    x += sc(6);
    place(m_rdoVolts, 56);
    place(m_rdoCodes, 86);
    x += sc(6);
    place(m_lblRamp, 40);
    place(m_cmbRamp, 100, sc(200));
    x += sc(6);
    if (m_lblInfo.GetSafeHwnd()) m_lblInfo.MoveWindow(x, fy, (std::max)(10, cx - x - sc(6)), h);
    Invalidate();
}

void ScopeTab::OnPaint()
{
    CPaintDC dc(this);
    const Theme::Palette& th = Theme::Get();
    dc.FillSolidRect(m_rcSplit, th.bg);
    const int midY = m_rcSplit.top + m_rcSplit.Height() / 2;
    const int midX = m_rcSplit.left + m_rcSplit.Width() / 2;
    for (int i = -3; i <= 3; ++i)
        dc.FillSolidRect(midX + i * 6 - 1, midY - 1, 3, 3, th.border);
}

BOOL ScopeTab::OnEraseBkgnd(CDC* pDC)
{
    CRect rc; GetClientRect(&rc);
    pDC->FillSolidRect(rc, Theme::Get().bg);
    return TRUE;
}

HBRUSH ScopeTab::OnCtlColor(CDC* pDC, CWnd*, UINT nCtlColor)
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

bool ScopeTab::HitSplitter(CPoint pt) const { return m_rcSplit.PtInRect(pt) != FALSE; }

void ScopeTab::OnLButtonDown(UINT, CPoint pt)
{
    if (HitSplitter(pt)) { m_dragging = true; SetCapture(); ::SetCursor(::LoadCursor(nullptr, IDC_SIZENS)); }
}

void ScopeTab::OnLButtonUp(UINT, CPoint)
{
    if (m_dragging) { m_dragging = false; ReleaseCapture(); }
}

void ScopeTab::OnMouseMove(UINT, CPoint pt)
{
    if (!m_dragging) { if (HitSplitter(pt)) ::SetCursor(::LoadCursor(nullptr, IDC_SIZENS)); return; }
    CRect rc; GetClientRect(&rc);
    const int S = static_cast<int>(Dpi::Of(m_hWnd));
    const int avail = rc.Height() - ::MulDiv(kFooterH96, S, 96) - ::MulDiv(kSplitH96, S, 96);
    if (avail < 50) return;
    float f = static_cast<float>(pt.y) / static_cast<float>(avail);
    if (f < 0.1f) f = 0.1f;
    if (f > 0.9f) f = 0.9f;
    m_split = f;
    Relayout();
}

BOOL ScopeTab::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT msg)
{
    CPoint pt; ::GetCursorPos(&pt); ScreenToClient(&pt);
    if (HitSplitter(pt)) { ::SetCursor(::LoadCursor(nullptr, IDC_SIZENS)); return TRUE; }
    return CWnd::OnSetCursor(pWnd, nHitTest, msg);
}
