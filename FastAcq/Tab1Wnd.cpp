#include "pch.h"
#include "Tab1Wnd.h"
#include "AppMessages.h"
#include "Theme.h"

// ----- Tab1 -----
BEGIN_MESSAGE_MAP(Tab1Wnd, CWnd)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
    ON_WM_CTLCOLOR()
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_WM_MOUSEMOVE()
    ON_WM_SETCURSOR()
    ON_BN_CLICKED(2010, &Tab1Wnd::OnDotsChanged)
END_MESSAGE_MAP()

BOOL Tab1Wnd::CreateTab(CWnd* parent, UINT id)
{
    LPCTSTR cls = AfxRegisterWndClass(0, ::LoadCursor(nullptr, IDC_ARROW),
                                      reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1),
                                      nullptr);
    return Create(cls, nullptr, WS_CHILD, CRect(0, 0, 10, 10), parent, id);
}

int Tab1Wnd::OnCreate(LPCREATESTRUCT lpcs)
{
    if (CWnd::OnCreate(lpcs) == -1) return -1;
    m_bgBrush.CreateSolidBrush(::GetSysColor(COLOR_BTNFACE));
    m_up.CreateView(this, 2001);
    m_dn.CreateView(this, 2002);
    m_up.SetTitle(_T("UP ramp"));
    m_dn.SetTitle(_T("DOWN ramp"));

    m_footerFont.CreateFont(-11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                            CLEARTYPE_QUALITY, VARIABLE_PITCH | FF_SWISS, _T("Segoe UI"));

    m_chkDots.Create(_T("Dots"), WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX,
                     CRect(0,0,60,20), this, 2010);
    m_chkDots.SetFont(&m_footerFont);
    return 0;
}

void Tab1Wnd::OnSize(UINT t, int cx, int cy)
{
    CWnd::OnSize(t, cx, cy);
    m_lastCy = cy;
    ApplySplit(cy);
}

BOOL Tab1Wnd::OnEraseBkgnd(CDC* pDC)
{
    CRect rc; GetClientRect(&rc);
    pDC->FillSolidRect(rc, ::GetSysColor(COLOR_BTNFACE));
    return TRUE;
}

HBRUSH Tab1Wnd::OnCtlColor(CDC* pDC, CWnd*, UINT nCtlColor)
{
    if (nCtlColor == CTLCOLOR_STATIC || nCtlColor == CTLCOLOR_BTN) {
        pDC->SetBkMode(TRANSPARENT);
        pDC->SetTextColor(::GetSysColor(COLOR_WINDOWTEXT));
        return static_cast<HBRUSH>(m_bgBrush.GetSafeHandle());
    }
    return static_cast<HBRUSH>(m_bgBrush.GetSafeHandle());
}

void Tab1Wnd::ApplySplit(int cy)
{
    if (cy <= kSplitH + kFooterH) return;
    CRect rc; GetClientRect(&rc);
    int cx = rc.Width();

    int availH = cy - kSplitH - kFooterH;
    int upH = static_cast<int>(availH * m_splitRatio);
    if (upH < 20) upH = 20;
    if (upH > availH - 20) upH = availH - 20;

    int splitterY = upH;
    int dnY       = splitterY + kSplitH;
    int dnH       = availH - upH;

    if (m_up.GetSafeHwnd()) m_up.MoveWindow(0, 0, cx, upH);
    if (m_dn.GetSafeHwnd()) m_dn.MoveWindow(0, dnY, cx, dnH);

    // Footer at bottom
    if (m_chkDots.GetSafeHwnd())
        m_chkDots.MoveWindow(10, cy - kFooterH + 4, 60, 20);

    // Repaint splitter bar area.
    CRect splitterRc(0, splitterY, cx, splitterY + kSplitH);
    InvalidateRect(splitterRc, FALSE);
}

void Tab1Wnd::OnPaint()
{
    CPaintDC dc(this);
    CRect rc; GetClientRect(&rc);

    // Draw splitter bar between the two views.
    int upH = static_cast<int>((m_lastCy - kSplitH) * m_splitRatio);
    if (upH < 20) upH = 20;
    if (upH > m_lastCy - kSplitH - 20) upH = m_lastCy - kSplitH - 20;

    CRect bar(rc.left, upH, rc.right, upH + kSplitH);
    dc.FillSolidRect(bar, ::GetSysColor(COLOR_BTNFACE));

    // Grip dots in the centre of the bar.
    int midY = bar.top + kSplitH / 2;
    int midX = bar.left + bar.Width() / 2;
    for (int i = -3; i <= 3; ++i) {
        CRect dot(midX + i * 6 - 1, midY - 1, midX + i * 6 + 2, midY + 2);
        dc.FillSolidRect(dot, ::GetSysColor(COLOR_BTNSHADOW));
    }
}

bool Tab1Wnd::HitSplitter(CPoint pt) const
{
    int upH = static_cast<int>((m_lastCy - kSplitH) * m_splitRatio);
    if (upH < 20) upH = 20;
    if (upH > m_lastCy - kSplitH - 20) upH = m_lastCy - kSplitH - 20;
    return (pt.y >= upH && pt.y < upH + kSplitH);
}

void Tab1Wnd::OnLButtonDown(UINT, CPoint pt)
{
    if (HitSplitter(pt)) {
        m_dragging = true;
        SetCapture();
        ::SetCursor(::LoadCursor(nullptr, IDC_SIZENS));
    }
}

void Tab1Wnd::OnLButtonUp(UINT, CPoint)
{
    if (m_dragging) {
        m_dragging = false;
        ReleaseCapture();
    }
}

void Tab1Wnd::OnMouseMove(UINT, CPoint pt)
{
    if (m_dragging && m_lastCy > kSplitH + 40) {
        float ratio = static_cast<float>(pt.y) / static_cast<float>(m_lastCy - kSplitH);
        if (ratio < 0.05f) ratio = 0.05f;
        if (ratio > 0.95f) ratio = 0.95f;
        m_splitRatio = ratio;
        ApplySplit(m_lastCy);
    } else if (HitSplitter(pt)) {
        ::SetCursor(::LoadCursor(nullptr, IDC_SIZENS));
    }
}

BOOL Tab1Wnd::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT msg)
{
    CPoint pt; ::GetCursorPos(&pt); ScreenToClient(&pt);
    if (HitSplitter(pt)) {
        ::SetCursor(::LoadCursor(nullptr, IDC_SIZENS));
        return TRUE;
    }
    return CWnd::OnSetCursor(pWnd, nHitTest, msg);
}

void Tab1Wnd::SetAcqMode(bool rawMode)
{
    m_rawMode = rawMode;
    m_up.ShowWindow(rawMode ? SW_SHOW : SW_HIDE);
    m_dn.ShowWindow(rawMode ? SW_SHOW : SW_HIDE);
    Invalidate(FALSE);
}

void Tab1Wnd::SetDotsMode(bool dots)
{
    m_up.SetDotsMode(dots);
    m_dn.SetDotsMode(dots);
    if (m_chkDots.GetSafeHwnd())
        m_chkDots.SetCheck(dots ? BST_CHECKED : BST_UNCHECKED);
}

void Tab1Wnd::OnDotsChanged()
{
    bool dots = (m_chkDots.GetCheck() == BST_CHECKED);
    SetDotsMode(dots);
}

void Tab1Wnd::ShowFrame(const ChirpFrame& f, bool rawMode,
                        const FftSettings& cfg, uint32_t sampleRateHz)
{
    if (rawMode != m_rawMode) SetAcqMode(rawMode);
    if (!rawMode) return;   // FFT mode: no waveform display
    if (f.raw.empty()) return;

    size_t n    = f.raw.size();
    size_t half = n / 2;

    m_up.SetSamples(f.raw.data(),        half);
    m_dn.SetSamples(f.raw.data() + half, n - half);
    m_up.SetSampleRate(sampleRateHz);
    m_dn.SetSampleRate(sampleRateHz);

    // Compute dominant frequency for each ramp via local FFT.
    if (sampleRateHz > 0) {
        float freqRes = 0.0f;
        {
            auto mag = LocalFft::Compute(f.raw.data(), half, sampleRateHz, cfg, freqRes);
            float freqUp = LocalFft::PeakFrequencyHz(mag, freqRes);
            m_up.SetFrequency(freqUp);
            TRACE(_T("UP freq: %.1f Hz (res=%.1f Hz/bin, bins=%zu)\n"), freqUp, freqRes, mag.size());
        }
        {
            auto mag = LocalFft::Compute(f.raw.data() + half, n - half, sampleRateHz, cfg, freqRes);
            float freqDn = LocalFft::PeakFrequencyHz(mag, freqRes);
            m_dn.SetFrequency(freqDn);
            TRACE(_T("DN freq: %.1f Hz (res=%.1f Hz/bin)\n"), freqDn, freqRes);
        }
    }
}

// ----- Tab2 -----
BEGIN_MESSAGE_MAP(Tab2Wnd, CWnd)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_ERASEBKGND()
    ON_WM_CTLCOLOR()
    ON_BN_CLICKED(2020, &Tab2Wnd::OnApplyFft)
END_MESSAGE_MAP()

BOOL Tab2Wnd::CreateTab(CWnd* parent, UINT id)
{
    LPCTSTR cls = AfxRegisterWndClass(0, ::LoadCursor(nullptr, IDC_ARROW),
                                      reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1),
                                      nullptr);
    return Create(cls, nullptr, WS_CHILD, CRect(0, 0, 10, 10), parent, id);
}

int Tab2Wnd::OnCreate(LPCREATESTRUCT lpcs)
{
    if (CWnd::OnCreate(lpcs) == -1) return -1;
    m_bgBrush.CreateSolidBrush(::GetSysColor(COLOR_BTNFACE));
    m_waterfall.CreateView(this, 2101);
    m_specUp.CreateView(this, 2102);
    m_specDn.CreateView(this, 2103);
    m_specUp.SetTitle(_T("Spectrum UP"));
    m_specDn.SetTitle(_T("Spectrum DOWN"));

    m_footerFont.CreateFont(-11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                            CLEARTYPE_QUALITY, VARIABLE_PITCH | FF_SWISS, _T("Segoe UI"));

    m_lblFftSize.Create(_T("FFT:"), WS_CHILD|WS_VISIBLE, CRect(0,0,30,20), this);
    m_cmbFftSize.Create(WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST, CRect(0,0,80,200), this, 2021);
    m_lblFftWin.Create(_T("Win:"), WS_CHILD|WS_VISIBLE, CRect(0,0,30,20), this);
    m_cmbFftWin.Create(WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST, CRect(0,0,100,200), this, 2022);
    m_btnApplyFft.Create(_T("Apply FFT"), WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, CRect(0,0,80,20), this, 2020);

    m_lblFftSize.SetFont(&m_footerFont);
    m_cmbFftSize.SetFont(&m_footerFont);
    m_lblFftWin.SetFont(&m_footerFont);
    m_cmbFftWin.SetFont(&m_footerFont);
    m_btnApplyFft.SetFont(&m_footerFont);

    m_cmbFftSize.AddString(_T("512"));
    m_cmbFftSize.AddString(_T("1024"));
    m_cmbFftSize.AddString(_T("2048"));
    m_cmbFftSize.AddString(_T("4096"));
    m_cmbFftSize.AddString(_T("8192"));
    m_cmbFftSize.AddString(_T("16384"));
    m_cmbFftSize.AddString(_T("32768"));
    m_cmbFftSize.SetCurSel(5);

    m_cmbFftWin.AddString(_T("Rectangular"));
    m_cmbFftWin.AddString(_T("Hann"));
    m_cmbFftWin.AddString(_T("Hamming"));
    m_cmbFftWin.AddString(_T("Blackman"));
    m_cmbFftWin.SetCurSel(1);

    return 0;
}

void Tab2Wnd::OnSize(UINT t, int cx, int cy)
{
    CWnd::OnSize(t, cx, cy);
    if (cy < kFooterH) return;

    int availH = cy - kFooterH;
    int waterH = availH * 6 / 10;
    int specH  = availH - waterH;

    if (m_waterfall.GetSafeHwnd()) m_waterfall.MoveWindow(0, 0, cx, waterH);
    int halfW = cx / 2;
    if (m_specUp.GetSafeHwnd()) m_specUp.MoveWindow(0,     waterH, halfW,      specH);
    if (m_specDn.GetSafeHwnd()) m_specDn.MoveWindow(halfW, waterH, cx - halfW, specH);

    // Footer at bottom
    int x = 10;
    int footerY = cy - kFooterH + 4;
    if (m_lblFftSize.GetSafeHwnd()) m_lblFftSize.MoveWindow(x, footerY, 30, 20);
    x += 34;
    if (m_cmbFftSize.GetSafeHwnd()) m_cmbFftSize.MoveWindow(x, footerY, 80, 200);
    x += 84;
    if (m_lblFftWin.GetSafeHwnd()) m_lblFftWin.MoveWindow(x, footerY, 30, 20);
    x += 34;
    if (m_cmbFftWin.GetSafeHwnd()) m_cmbFftWin.MoveWindow(x, footerY, 100, 200);
    x += 104;
    if (m_btnApplyFft.GetSafeHwnd()) m_btnApplyFft.MoveWindow(x, footerY, 80, 20);
}

BOOL Tab2Wnd::OnEraseBkgnd(CDC* pDC)
{
    CRect rc; GetClientRect(&rc);
    pDC->FillSolidRect(rc, ::GetSysColor(COLOR_BTNFACE));
    return TRUE;
}

HBRUSH Tab2Wnd::OnCtlColor(CDC* pDC, CWnd*, UINT nCtlColor)
{
    if (nCtlColor == CTLCOLOR_STATIC || nCtlColor == CTLCOLOR_BTN) {
        pDC->SetBkMode(TRANSPARENT);
        pDC->SetTextColor(::GetSysColor(COLOR_WINDOWTEXT));
        return static_cast<HBRUSH>(m_bgBrush.GetSafeHandle());
    }
    return static_cast<HBRUSH>(m_bgBrush.GetSafeHandle());
}

void Tab2Wnd::SetAcqMode(bool rawMode)
{
    m_rawMode = rawMode;
}

void Tab2Wnd::ShowFrame(const ChirpFrame& f, bool rawMode,
                        const FftSettings& cfg, uint32_t sampleRateHz)
{
    m_rawMode = rawMode;
    m_fftCfg  = cfg;

    if (rawMode) {
        // RAW mode: compute FFT locally from the full raw capture.
        if (f.raw.empty()) return;
        float freqRes = 0.0f;
        auto  mag     = LocalFft::Compute(f.raw.data(), f.raw.size(),
                                          sampleRateHz, cfg, freqRes);
        if (mag.empty()) return;
        m_waterfall.PushFft(mag.data(), mag.size());
        m_specUp.SetFft(mag.data(), mag.size(), freqRes);
        m_specDn.SetFft(mag.data(), mag.size(), freqRes);
    } else {
        // FFT mode: use data already computed by the firmware.
        if (f.fft.empty()) return;
        float res = f.header.fft_freq_res_hz;
        m_waterfall.PushFft(f.fft.data(), f.fft.size());
        m_specUp.SetFft(f.fft.data(), f.fft.size(), res);
        m_specDn.SetFft(f.fft.data(), f.fft.size(), res);
    }
}

void Tab2Wnd::ClearHistory()
{
    m_waterfall.Clear();
}

void Tab2Wnd::OnApplyFft()
{
    int sizeIdx = m_cmbFftSize.GetCurSel();
    int winIdx  = m_cmbFftWin.GetCurSel();

    if (sizeIdx < 0) sizeIdx = 5;
    if (winIdx < 0)  winIdx = 1;

    const int sizes[] = { 512, 1024, 2048, 4096, 8192, 16384, 32768 };
    m_fftCfg.size = (sizeIdx < 7) ? sizes[sizeIdx] : 16384;
    m_fftCfg.window = static_cast<FftWindow>(winIdx);

    // Notify parent MainFrame
    CWnd* parent = GetParent();
    if (parent && parent->GetParent()) {
        FftSettings* p = new FftSettings(m_fftCfg);
        if (!parent->GetParent()->PostMessage(WM_APP_FFT_SETTINGS, 0, reinterpret_cast<LPARAM>(p)))
            delete p;
    }
}
