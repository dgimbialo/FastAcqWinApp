#include "pch.h"
#include "Tab1Wnd.h"

// ----- Tab1 -----
BEGIN_MESSAGE_MAP(Tab1Wnd, CWnd)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_WM_MOUSEMOVE()
    ON_WM_SETCURSOR()
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
    m_up.CreateView(this, 2001);
    m_dn.CreateView(this, 2002);
    m_up.SetTitle(_T("UP ramp"));
    m_dn.SetTitle(_T("DOWN ramp"));
    return 0;
}

void Tab1Wnd::OnSize(UINT t, int cx, int cy)
{
    CWnd::OnSize(t, cx, cy);
    m_lastCy = cy;
    ApplySplit(cy);
}

void Tab1Wnd::ApplySplit(int cy)
{
    if (cy <= kSplitH) return;
    CRect rc; GetClientRect(&rc);
    int cx = rc.Width();

    int upH = static_cast<int>((cy - kSplitH) * m_splitRatio);
    if (upH < 20)          upH = 20;
    if (upH > cy - kSplitH - 20) upH = cy - kSplitH - 20;

    int splitterY = upH;
    int dnY       = splitterY + kSplitH;
    int dnH       = cy - dnY;

    if (m_up.GetSafeHwnd()) m_up.MoveWindow(0, 0,   cx, upH);
    if (m_dn.GetSafeHwnd()) m_dn.MoveWindow(0, dnY, cx, dnH);

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
    dc.FillSolidRect(bar, RGB(60, 60, 72));

    // Grip dots in the centre of the bar.
    int midY = bar.top + kSplitH / 2;
    int midX = bar.left + bar.Width() / 2;
    for (int i = -3; i <= 3; ++i) {
        CRect dot(midX + i * 6 - 1, midY - 1, midX + i * 6 + 2, midY + 2);
        dc.FillSolidRect(dot, RGB(140, 140, 160));
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
            m_up.SetFrequency(LocalFft::PeakFrequencyHz(mag, freqRes));
        }
        {
            auto mag = LocalFft::Compute(f.raw.data() + half, n - half, sampleRateHz, cfg, freqRes);
            m_dn.SetFrequency(LocalFft::PeakFrequencyHz(mag, freqRes));
        }
    }
}

// ----- Tab2 -----
BEGIN_MESSAGE_MAP(Tab2Wnd, CWnd)
    ON_WM_CREATE()
    ON_WM_SIZE()
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
    m_waterfall.CreateView(this, 2101);
    m_specUp.CreateView(this, 2102);
    m_specDn.CreateView(this, 2103);
    m_specUp.SetTitle(_T("Spectrum UP"));
    m_specDn.SetTitle(_T("Spectrum DOWN"));
    return 0;
}

void Tab2Wnd::OnSize(UINT t, int cx, int cy)
{
    CWnd::OnSize(t, cx, cy);
    // Top: waterfall (60%). Bottom: two spectra side-by-side (40%).
    int waterH = cy * 6 / 10;
    int specH  = cy - waterH;
    if (m_waterfall.GetSafeHwnd()) m_waterfall.MoveWindow(0, 0, cx, waterH);
    int halfW = cx / 2;
    if (m_specUp.GetSafeHwnd()) m_specUp.MoveWindow(0,     waterH, halfW,        specH);
    if (m_specDn.GetSafeHwnd()) m_specDn.MoveWindow(halfW, waterH, cx - halfW,   specH);
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
