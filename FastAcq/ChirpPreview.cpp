#include "pch.h"
#include "ChirpPreview.h"
#include <algorithm>
#include <cmath>

template <class T> static T Clamp(T v, T lo, T hi) { return v < lo ? lo : (v > hi ? hi : v); }

// Firmware constants (chirp_dac.h / dcmi_adc.h)
static constexpr uint32_t kTim7ClkHz      = 240000000u;
static constexpr uint32_t kTriangleMax    = 8192u;
static constexpr uint32_t kMinTicksPerSmp = 64u;
static constexpr uint32_t kFreqMin        = 100u;
static constexpr uint32_t kFreqMax        = 24000u;
static constexpr uint32_t kAdcHz          = 60000000u;
static constexpr uint32_t kChunk          = 16384u;
static constexpr uint32_t kCaptureMax     = 650000u;

ChirpGeometry ComputeChirpGeometry(const ChirpParams& p)
{
    ChirpGeometry g;
    uint32_t periodTicks, riseTicks;
    if (p.riseUs && p.fallUs) {
        g.rampMode  = true;
        periodTicks = (p.riseUs + p.fallUs) * 240u;
        riseTicks   = p.riseUs * 240u;
    } else {
        if (p.freqHz < kFreqMin || p.freqHz > kFreqMax) return g;
        periodTicks = kTim7ClkHz / p.freqHz;
        riseTicks   = periodTicks / 2u;
    }
    if (periodTicks < kTim7ClkHz / kFreqMax || periodTicks > kTim7ClkHz / kFreqMin) return g;

    uint32_t tps = (periodTicks + kTriangleMax - 1u) / kTriangleMax;
    if (tps < kMinTicksPerSmp) tps = kMinTicksPerSmp;
    tps = (tps + 3u) & ~3u;
    uint32_t len = (periodTicks + tps / 2u) / tps;
    len = Clamp<uint32_t>(len, 2u, kTriangleMax);
    uint32_t rise = static_cast<uint32_t>((static_cast<uint64_t>(len) * riseTicks + periodTicks / 2u) / periodTicks);
    rise = Clamp<uint32_t>(rise, 1u, len - 1u);

    g.valid           = true;
    g.tableLen        = len;
    g.riseLen         = rise;
    g.ticksPerSample  = tps;
    g.periodTicks     = len * tps;
    g.samplesPerChirp = g.periodTicks / 4u;
    g.riseSamples     = (rise * tps) / 4u;
    g.riseUs          = static_cast<double>(rise) * tps / 240.0;
    g.fallUs          = static_cast<double>(len - rise) * tps / 240.0;
    g.periodUs        = g.riseUs + g.fallUs;
    g.freqHz          = 1e6 / g.periodUs;

    uint64_t burst = static_cast<uint64_t>(g.samplesPerChirp) * (std::max<uint32_t>)(1u, p.burst);
    g.samplesPerBurst = static_cast<uint32_t>(std::min<uint64_t>(burst, kCaptureMax));
    uint32_t tgt = ((g.samplesPerBurst + kChunk - 1u) / kChunk) * kChunk;
    if (p.samplesOvr) tgt = (((std::min)(p.samplesOvr, kCaptureMax) + kChunk - 1u) / kChunk) * kChunk;
    tgt = (std::min)(tgt, (kCaptureMax / kChunk) * kChunk);
    g.captureTarget = tgt;
    g.chunks        = tgt / kChunk;
    return g;
}

// ---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(ChirpPreviewCtrl, CWnd)
    ON_WM_CREATE()
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
    ON_WM_SIZE()
END_MESSAGE_MAP()

BOOL ChirpPreviewCtrl::CreateCtrl(CWnd* parent, UINT id)
{
    LPCTSTR cls = AfxRegisterWndClass(0, ::LoadCursor(nullptr, IDC_ARROW),
                                      reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1), nullptr);
    return Create(cls, nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER, CRect(0, 0, 10, 10), parent, id);
}

int ChirpPreviewCtrl::OnCreate(LPCREATESTRUCT lpcs)
{
    if (CWnd::OnCreate(lpcs) == -1) return -1;
    m_font.CreatePointFont(90, _T("Segoe UI"));
    m_smallFont.CreatePointFont(80, _T("Segoe UI"));
    m_boldFont.CreateFont(-14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                          OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                          VARIABLE_PITCH | FF_SWISS, _T("Segoe UI"));
    m_g = ComputeChirpGeometry(m_p);
    return 0;
}

void ChirpPreviewCtrl::SetParams(const ChirpParams& p)
{
    m_p = p;
    m_g = ComputeChirpGeometry(p);
    if (GetSafeHwnd()) Invalidate(FALSE);
}

void ChirpPreviewCtrl::OnSize(UINT t, int cx, int cy)
{
    CWnd::OnSize(t, cx, cy);
    Invalidate(FALSE);
}

void ChirpPreviewCtrl::OnPaint()
{
    CPaintDC dc(this);
    CRect rc; GetClientRect(&rc);
    if (rc.Width() < 10 || rc.Height() < 10) return;
    CDC mem; mem.CreateCompatibleDC(&dc);
    CBitmap bmp; bmp.CreateCompatibleBitmap(&dc, rc.Width(), rc.Height());
    CBitmap* old = mem.SelectObject(&bmp);
    Render(mem, rc);
    dc.BitBlt(0, 0, rc.Width(), rc.Height(), &mem, 0, 0, SRCCOPY);
    mem.SelectObject(old);
}

// ---------------------------------------------------------------------------
static CString FmtUs(double us)
{
    CString s;
    if (us >= 1000.0) s.Format(_T("%.3f ms"), us / 1000.0); else s.Format(_T("%.1f us"), us);
    return s;
}

static void DimLine(CDC& dc, int x1, int x2, int y, COLORREF c)
{
    CPen pen(PS_SOLID, 1, c);
    CPen* op = dc.SelectObject(&pen);
    dc.MoveTo(x1, y); dc.LineTo(x2, y);
    // arrow heads
    dc.MoveTo(x1, y); dc.LineTo(x1 + 5, y - 3); dc.MoveTo(x1, y); dc.LineTo(x1 + 5, y + 3);
    dc.MoveTo(x2, y); dc.LineTo(x2 - 5, y - 3); dc.MoveTo(x2, y); dc.LineTo(x2 - 5, y + 3);
    dc.SelectObject(op);
}

void ChirpPreviewCtrl::Render(CDC& dc, const CRect& full)
{
    const COLORREF cBg = ::GetSysColor(COLOR_WINDOW), cText = ::GetSysColor(COLOR_WINDOWTEXT);
    const COLORREF cChirp = RGB(0, 102, 204), cCapture = RGB(228, 239, 252), cGrid = RGB(228, 231, 236);
    const COLORREF cDim = RGB(200, 90, 0), cMuted = RGB(120, 128, 140), cNext = RGB(160, 180, 210);
    const COLORREF cCap = RGB(60, 90, 140);

    dc.FillSolidRect(full, cBg);
    dc.SetBkMode(TRANSPARENT);

    CFont* of = dc.SelectObject(&m_boldFont);
    dc.SetTextColor(cText);
    dc.TextOut(full.left + 10, full.top + 6, _T("Chirp preview (what the MCU will generate)"));

    if (!m_g.valid) {
        dc.SelectObject(&m_font);
        dc.SetTextColor(cDim);
        dc.TextOut(full.left + 10, full.top + 30,
                   _T("Parameters out of range: frequency 100..24000 Hz, period 41.7 us .. 10 ms."));
        dc.SelectObject(of);
        return;
    }

    // Layout: title row | axis captions | plot | dim row 1 (rise/fall, interval) |
    //         dim row 2 (period) | info line 1 | info line 2
    const int mL = 66, mR = 82, mT = 46;
    const int dimRow = 26;           // one dimension row (line + text)
    const int infoLine = 18;
    const int mB = 8 + 2 * dimRow + 2 * infoLine + 6;
    CRect plot(full.left + mL, full.top + mT, full.right - mR, full.bottom - mB);
    if (plot.Width() < 160 || plot.Height() < 40) { dc.SelectObject(of); return; }

    const uint32_t shown = (std::min<uint32_t>)(m_p.burst, 4u);
    const int breakPx = 56, nextPx = (std::min)(80, plot.Width() / 7);
    const int chirpsPx = plot.Width() - breakPx - nextPx;
    const double usPerPx = (m_g.periodUs * shown) / static_cast<double>(chirpsPx);
    auto X = [&](double us) { return plot.left + static_cast<int>(us / usPerPx + 0.5); };
    const double ampMax = 4095.0;
    const int    headroom = 22;      // space above full-scale for the amplitude label
    auto Y = [&](double counts) {
        return plot.bottom - static_cast<int>(counts / ampMax * (plot.Height() - headroom) + 0.5);
    };
    const int chirpsEnd = plot.left + chirpsPx;
    const int breakX    = chirpsEnd + breakPx / 2;
    const int nextX0    = chirpsEnd + breakPx;

    // ADC capture window (never drawn past the burst region)
    const double captureUs = static_cast<double>(m_g.captureTarget) / kAdcHz * 1e6;
    int capR = (std::min)(static_cast<int>(X(captureUs)), chirpsEnd);
    dc.FillSolidRect(plot.left, plot.top, capR - plot.left, plot.Height(), cCapture);

    // Grid + axes
    {
        CPen grid(PS_SOLID, 1, cGrid); CPen* op = dc.SelectObject(&grid);
        for (int i = 1; i <= 4; ++i) { int y = Y(ampMax * i / 4); dc.MoveTo(plot.left, y); dc.LineTo(plot.right, y); }
        dc.SelectObject(op);
        CPen axis(PS_SOLID, 1, cMuted); op = dc.SelectObject(&axis);
        dc.MoveTo(plot.left, plot.top); dc.LineTo(plot.left, plot.bottom); dc.LineTo(plot.right, plot.bottom);
        // axis break mark
        dc.MoveTo(breakX - 7, plot.bottom + 6); dc.LineTo(breakX - 1, plot.bottom - 6);
        dc.MoveTo(breakX - 1, plot.bottom + 6); dc.LineTo(breakX + 5, plot.bottom - 6);
        dc.SelectObject(op);
    }
    // Axis labels and captions
    dc.SelectObject(&m_smallFont);
    for (int i = 0; i <= 4; ++i) {
        double c = ampMax * i / 4; int y = Y(c);
        CString l; l.Format(_T("%.0f"), c);
        dc.SetTextColor(cMuted);
        dc.DrawText(l, CRect(full.left + 4, y - 7, plot.left - 5, y + 7), DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
        l.Format(_T("%.2f V"), c * 3.3 / 4095.0);
        dc.DrawText(l, CRect(plot.right + 5, y - 7, full.right - 4, y + 7), DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }
    dc.DrawText(_T("DAC code"), CRect(full.left + 4, plot.top - 18, plot.left - 5, plot.top - 4),
                DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
    dc.DrawText(_T("V (3.3 V ref)"), CRect(plot.right + 5, plot.top - 18, full.right - 4, plot.top - 4),
                DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    // Chirp triangles (burst)
    {
        CPen pen(PS_SOLID, 2, cChirp); CPen* op = dc.SelectObject(&pen);
        for (uint32_t k = 0; k < shown; ++k) {
            double t0 = k * m_g.periodUs;
            dc.MoveTo(X(t0), Y(0));
            dc.LineTo(X(t0 + m_g.riseUs), Y(m_p.amplitude));
            dc.LineTo(X(t0 + m_g.periodUs), Y(0));
        }
        dc.SelectObject(op);
    }
    // Faded start of the next cycle
    {
        double pxPerUs = 1.0 / usPerPx;
        int riseW = (std::max)(1, static_cast<int>(m_g.riseUs * pxPerUs));
        double frac = (std::min)(1.0, (plot.right - nextX0) / static_cast<double>(riseW));
        CPen nxt(PS_SOLID, 2, cNext); CPen* op = dc.SelectObject(&nxt);
        dc.MoveTo(nextX0, Y(0));
        dc.LineTo((std::min)(static_cast<int>(plot.right), nextX0 + riseW), Y(m_p.amplitude * frac));
        dc.SelectObject(op);
    }
    // Amplitude: dotted level line + label centred above the first peak (headroom keeps it clear)
    {
        int px = X(m_g.riseUs), py = Y(m_p.amplitude);
        CPen dash(PS_DOT, 1, cDim); CPen* op = dc.SelectObject(&dash);
        dc.MoveTo(plot.left, py); dc.LineTo(px, py);
        dc.SelectObject(op);
        CString a; a.Format(_T("A = %u DAC (%.2f V)"), m_p.amplitude, m_p.amplitude * 3.3 / 4095.0);
        dc.SetTextColor(cDim);
        dc.SelectObject(&m_font);
        CRect ar(px - 110, py - 19, px + 110, py - 3);
        if (ar.left < plot.left) ar.OffsetRect(plot.left - ar.left, 0);
        dc.DrawText(a, ar, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP);
    }

    // Dimension row 1: rise / fall under the first chirp, interval label at the break, next cycle
    dc.SelectObject(&m_smallFont);
    {
        int y = plot.bottom + 12;
        int xr0 = X(0), xr1 = X(m_g.riseUs), xf1 = X(m_g.periodUs);
        DimLine(dc, xr0, xr1, y, cDim);
        DimLine(dc, xr1, xf1, y, cDim);
        dc.SetTextColor(cDim);
        dc.DrawText(_T("rise ") + FmtUs(m_g.riseUs), CRect(xr0, y + 3, xr1, y + 17), DT_CENTER | DT_SINGLELINE | DT_NOCLIP);
        dc.DrawText(_T("fall ") + FmtUs(m_g.fallUs), CRect(xr1, y + 3, xf1, y + 17), DT_CENTER | DT_SINGLELINE | DT_NOCLIP);
        if (shown > 1) {
            dc.SetTextColor(cChirp);
            CString b; b.Format(_T("burst x%u%s"), m_p.burst, m_p.burst > shown ? _T(" (first 4 drawn)") : _T(""));
            dc.DrawText(b, CRect(xf1, y + 3, chirpsEnd, y + 17), DT_CENTER | DT_SINGLELINE | DT_NOCLIP);
        }
        CString gap;
        if (m_p.mode == 1)      gap.Format(_T("interval %u ms"), m_p.intervalMs);
        else if (m_p.mode == 2) gap = _T("wait TRIGGER");
        else                    gap = _T("IDLE: trigger only");
        dc.SetTextColor(cMuted);
        dc.DrawText(gap, CRect(breakX - 70, y + 3, breakX + 70, y + 17), DT_CENTER | DT_SINGLELINE | DT_NOCLIP);
        dc.SetTextColor(cNext);
        dc.DrawText(_T("next cycle"), CRect(nextX0, plot.top + 2, plot.right, plot.top + 16),
                    DT_RIGHT | DT_SINGLELINE | DT_NOCLIP);
    }
    // Dimension row 2: period / frequency across one chirp
    {
        int y = plot.bottom + 12 + dimRow;
        int xr0 = X(0), xf1 = X(m_g.periodUs);
        DimLine(dc, xr0, xf1, y, cChirp);
        CString s;
        s.Format(_T("T = %s   f = %.3f Hz   (%s)"), FmtUs(m_g.periodUs).GetString(), m_g.freqHz,
                 m_g.rampMode ? _T("ramp mode: rise/fall") : _T("freq mode: symmetric"));
        dc.SetTextColor(cChirp);
        dc.DrawText(s, CRect(xr0, y + 3, (std::max)(xf1, xr0 + 320), y + 17), DT_CENTER | DT_SINGLELINE | DT_NOCLIP);
    }
    // Info lines
    dc.SelectObject(&m_font);
    {
        CString s;
        s.Format(_T("ADC capture window (shaded): %u samples = %.1f us, %u DMA chunks x 16384 @ 60 MS/s"),
                 m_g.captureTarget, captureUs, m_g.chunks);
        if (m_p.samplesOvr) s += _T("  (samples override)");
        dc.SetTextColor(cCap);
        dc.DrawText(s, CRect(full.left + 10, full.bottom - 2 * infoLine - 4, full.right - 6, full.bottom - infoLine - 4),
                    DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        s.Format(_T("%u samples per chirp (rise %u + fall %u)  |  DAC table %u pts, %u ticks/pt = %.3f MS/s  |  mode %s"),
                 m_g.samplesPerChirp, m_g.riseSamples, m_g.samplesPerChirp - m_g.riseSamples,
                 m_g.tableLen, m_g.ticksPerSample, 240.0 / m_g.ticksPerSample,
                 m_p.mode == 1 ? _T("CONTINUOUS") : m_p.mode == 2 ? _T("SINGLE") : _T("IDLE"));
        dc.SetTextColor(cText);
        dc.DrawText(s, CRect(full.left + 10, full.bottom - infoLine - 4, full.right - 6, full.bottom - 4),
                    DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    }
    dc.SelectObject(of);
}
