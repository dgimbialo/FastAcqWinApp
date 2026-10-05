#include "pch.h"
#include "ChirpPreview.h"
#include "Dpi.h"
#include "Theme.h"

#include <algorithm>
#include <cmath>

using core::ChirpParams;
using core::ChirpGeometry;

BEGIN_MESSAGE_MAP(ChirpPreviewCtrl, CWnd)
    ON_WM_CREATE()
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
    ON_WM_SIZE()
    ON_MESSAGE(WM_DPICHANGED_AFTERPARENT, &ChirpPreviewCtrl::OnDpiChangedAfterParent)
END_MESSAGE_MAP()

BOOL ChirpPreviewCtrl::CreateCtrl(CWnd* parent, UINT id)
{
    LPCTSTR cls = AfxRegisterWndClass(0, ::LoadCursor(nullptr, IDC_ARROW), nullptr, nullptr);
    return Create(cls, nullptr, WS_CHILD | WS_VISIBLE, CRect(0, 0, 10, 10), parent, id);
}

int ChirpPreviewCtrl::OnCreate(LPCREATESTRUCT lpcs)
{
    if (CWnd::OnCreate(lpcs) == -1) return -1;
    MakeFonts();
    m_g = core::ComputeChirpGeometry(m_p);
    return 0;
}

void ChirpPreviewCtrl::MakeFonts()
{
    Dpi::MakeFont(m_font, m_hWnd, 9);
    Dpi::MakeFont(m_smallFont, m_hWnd, 8);
    Dpi::MakeFont(m_boldFont, m_hWnd, 10, true);
}

LRESULT ChirpPreviewCtrl::OnDpiChangedAfterParent(WPARAM, LPARAM)
{
    MakeFonts();
    Invalidate(FALSE);
    return 0;
}

void ChirpPreviewCtrl::ApplyTheme() { if (GetSafeHwnd()) Invalidate(FALSE); }

void ChirpPreviewCtrl::SetParams(const ChirpParams& p)
{
    m_p = p;
    m_g = core::ComputeChirpGeometry(p);
    if (GetSafeHwnd()) Invalidate(FALSE);
}

void ChirpPreviewCtrl::SetVco(double vtuneAtDac0V, double vtuneAtDacFullV, const core::VcoCurve& curve)
{
    m_vLow = vtuneAtDac0V; m_vHigh = vtuneAtDacFullV; m_vco = curve;
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

static void DimLine(CDC& dc, int x1, int x2, int y, COLORREF c, int a)
{
    CPen pen(PS_SOLID, 1, c);
    CPen* op = dc.SelectObject(&pen);
    dc.MoveTo(x1, y); dc.LineTo(x2, y);
    dc.MoveTo(x1, y); dc.LineTo(x1 + a, y - a / 2 - 1); dc.MoveTo(x1, y); dc.LineTo(x1 + a, y + a / 2 + 1);
    dc.MoveTo(x2, y); dc.LineTo(x2 - a, y - a / 2 - 1); dc.MoveTo(x2, y); dc.LineTo(x2 - a, y + a / 2 + 1);
    dc.SelectObject(op);
}

void ChirpPreviewCtrl::Render(CDC& dc, const CRect& full)
{
    const Theme::Palette& th = Theme::Get();
    const int S = static_cast<int>(Dpi::Of(m_hWnd));
    auto sc = [&](int px) { return ::MulDiv(px, S, 96); };

    const COLORREF cBg = th.plot, cText = th.text, cChirp = th.traceUp, cGrid = th.grid;
    const COLORREF cCapture = th.segUp, cDim = th.traceDn, cMuted = th.textDim;
    const COLORREF cNext = Theme::Blend(th.traceUp, th.plot, 0.6), cBorder = th.border;

    dc.FillSolidRect(full, cBg);
    {
        CPen bp(PS_SOLID, 1, cBorder); CPen* op = dc.SelectObject(&bp);
        CBrush* ob = static_cast<CBrush*>(dc.SelectStockObject(NULL_BRUSH));
        dc.Rectangle(full);
        dc.SelectObject(op); dc.SelectObject(ob);
    }
    dc.SetBkMode(TRANSPARENT);

    CFont* of = dc.SelectObject(&m_boldFont);
    dc.SetTextColor(cText);
    dc.TextOut(full.left + sc(10), full.top + sc(6), _T("Chirp preview (what the MCU will generate)"));

    if (!m_g.valid) {
        dc.SelectObject(&m_font);
        dc.SetTextColor(th.warn);
        dc.TextOut(full.left + sc(10), full.top + sc(30),
                   _T("Parameters out of range: frequency 100..24000 Hz, period 41.7 us .. 10 ms."));
        dc.SelectObject(of);
        return;
    }

    // Layout: title row | axis captions | plot | dim row 1 (rise/fall, interval) |
    //         dim row 2 (period) | info line 1 | info line 2
    const bool haveVco = m_vco.Valid();
    auto vtuneOf = [&](double counts) { return m_vLow + (m_vHigh - m_vLow) * counts / 4095.0; };
    const double base = (std::min<double>)(m_p.offset, 4094.0);
    const double top  = (std::min<double>)(base + m_p.amplitude, 4095.0);
    const int mL = sc(66), mR = haveVco ? sc(128) : sc(82), mT = sc(46);
    const int dimRow = sc(26);
    const int infoLine = sc(18);
    const int mB = sc(8) + 2 * dimRow + 2 * infoLine + sc(6);
    CRect plot(full.left + mL, full.top + mT, full.right - mR, full.bottom - mB);
    if (plot.Width() < sc(160) || plot.Height() < sc(40)) { dc.SelectObject(of); return; }

    const uint32_t shown = (std::min<uint32_t>)(m_p.burst, 4u);
    const int breakPx = sc(56), nextPx = (std::min)(sc(80), plot.Width() / 7);
    const int chirpsPx = plot.Width() - breakPx - nextPx;
    const double usPerPx = (m_g.periodUs * shown) / static_cast<double>(chirpsPx);
    auto X = [&](double us) { return plot.left + static_cast<int>(us / usPerPx + 0.5); };
    const double ampMax = 4095.0;
    const int    headroom = sc(22);
    auto Y = [&](double counts) {
        return plot.bottom - static_cast<int>(counts / ampMax * (plot.Height() - headroom) + 0.5);
    };
    const int chirpsEnd = plot.left + chirpsPx;
    const int breakX    = chirpsEnd + breakPx / 2;
    const int nextX0    = chirpsEnd + breakPx;

    // ADC capture window (never drawn past the burst region)
    const double captureUs = m_g.captureUs;
    int capR = (std::min)(static_cast<int>(X(captureUs)), chirpsEnd);
    dc.FillSolidRect(plot.left, plot.top, capR - plot.left, plot.Height(), cCapture);
    if (!m_g.fitsInCapture && capR < chirpsEnd) {
        // Part of the burst that the capture window does not cover.
        dc.FillSolidRect(capR, plot.top, chirpsEnd - capR, plot.Height(), Theme::Blend(th.danger, th.plot, 0.85));
        CPen cut(PS_SOLID, sc(2), th.danger); CPen* op = dc.SelectObject(&cut);
        dc.MoveTo(capR, plot.top); dc.LineTo(capR, plot.bottom);
        dc.SelectObject(op);
    }

    // Grid + axes
    {
        CPen grid(PS_SOLID, 1, cGrid); CPen* op = dc.SelectObject(&grid);
        for (int i = 1; i <= 4; ++i) { int y = Y(ampMax * i / 4); dc.MoveTo(plot.left, y); dc.LineTo(plot.right, y); }
        dc.SelectObject(op);
        CPen axis(PS_SOLID, 1, th.axis); op = dc.SelectObject(&axis);
        dc.MoveTo(plot.left, plot.top); dc.LineTo(plot.left, plot.bottom); dc.LineTo(plot.right, plot.bottom);
        dc.MoveTo(breakX - sc(7), plot.bottom + sc(6)); dc.LineTo(breakX - 1, plot.bottom - sc(6));
        dc.MoveTo(breakX - 1, plot.bottom + sc(6)); dc.LineTo(breakX + sc(5), plot.bottom - sc(6));
        dc.SelectObject(op);
    }
    // Axis labels and captions
    dc.SelectObject(&m_smallFont);
    for (int i = 0; i <= 4; ++i) {
        double c = ampMax * i / 4; int y = Y(c);
        CString l; l.Format(_T("%.0f"), c);
        dc.SetTextColor(th.axisText);
        dc.DrawText(l, CRect(full.left + sc(4), y - sc(7), plot.left - sc(5), y + sc(7)), DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
        if (haveVco) l.Format(_T("%.2f V  %.3f GHz"), vtuneOf(c), m_vco.FreqHz(vtuneOf(c)) / 1e9);
        else         l.Format(_T("%.2f V"), vtuneOf(c));
        dc.DrawText(l, CRect(plot.right + sc(5), y - sc(7), full.right - sc(4), y + sc(7)), DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }
    dc.DrawText(_T("DAC code"), CRect(full.left + sc(4), plot.top - sc(18), plot.left - sc(5), plot.top - sc(4)),
                DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
    dc.DrawText(haveVco ? _T("Vtune  /  VCO out") : _T("Vtune"),
                CRect(plot.right + sc(5), plot.top - sc(18), full.right - sc(4), plot.top - sc(4)),
                DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    // Chirp triangles (burst)
    {
        CPen pen(PS_SOLID, sc(2), cChirp); CPen* op = dc.SelectObject(&pen);
        for (uint32_t k = 0; k < shown; ++k) {
            double t0 = k * m_g.periodUs;
            dc.MoveTo(X(t0), Y(base));
            dc.LineTo(X(t0 + m_g.riseUs), Y(top));
            dc.LineTo(X(t0 + m_g.periodUs), Y(base));
        }
        dc.SelectObject(op);
    }
    // Faded start of the next cycle
    {
        double pxPerUs = 1.0 / usPerPx;
        int riseW = (std::max)(1, static_cast<int>(m_g.riseUs * pxPerUs));
        double frac = (std::min)(1.0, (plot.right - nextX0) / static_cast<double>(riseW));
        CPen nxt(PS_SOLID, sc(2), cNext); CPen* op = dc.SelectObject(&nxt);
        dc.MoveTo(nextX0, Y(base));
        dc.LineTo((std::min)(static_cast<int>(plot.right), nextX0 + riseW), Y(base + (top - base) * frac));
        dc.SelectObject(op);
    }
    // Amplitude: dotted level line + label above the first peak
    {
        int px = X(m_g.riseUs), py = Y(top), pb = Y(base);
        CPen dash(PS_DOT, 1, cDim); CPen* op = dc.SelectObject(&dash);
        dc.MoveTo(plot.left, py); dc.LineTo(px, py);
        if (base > 0.0) { dc.MoveTo(plot.left, pb); dc.LineTo(plot.right, pb); }
        dc.SelectObject(op);
        CString a;
        if (haveVco)
            a.Format(_T("DAC %.0f..%.0f (offset %u + A %u): Vtune %.2f..%.2f V = %.3f..%.3f GHz (B %.0f MHz)"),
                     base, top, m_p.offset, m_p.amplitude,
                     vtuneOf(base), vtuneOf(top), m_vco.FreqHz(vtuneOf(base)) / 1e9, m_vco.FreqHz(vtuneOf(top)) / 1e9,
                     std::fabs(m_vco.FreqHz(vtuneOf(top)) - m_vco.FreqHz(vtuneOf(base))) / 1e6);
        else
            a.Format(_T("DAC %.0f..%.0f (offset %u + A %u): Vtune %.2f..%.2f V"), base, top, m_p.offset, m_p.amplitude,
                     vtuneOf(base), vtuneOf(top));
        dc.SetTextColor(cDim);
        dc.SelectObject(&m_font);
        CRect ar(px - sc(200), py - sc(19), px + sc(200), py - sc(3));
        if (ar.left < plot.left) ar.OffsetRect(plot.left - ar.left, 0);
        dc.DrawText(a, ar, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP);
    }

    // Dimension row 1: rise / fall under the first chirp, interval label at the break, next cycle
    dc.SelectObject(&m_smallFont);
    {
        int y = plot.bottom + sc(12);
        int xr0 = X(0), xr1 = X(m_g.riseUs), xf1 = X(m_g.periodUs);
        DimLine(dc, xr0, xr1, y, cDim, sc(5));
        DimLine(dc, xr1, xf1, y, cDim, sc(5));
        dc.SetTextColor(cDim);
        dc.DrawText(_T("rise ") + FmtUs(m_g.riseUs), CRect(xr0, y + sc(3), xr1, y + sc(17)), DT_CENTER | DT_SINGLELINE | DT_NOCLIP);
        dc.DrawText(_T("fall ") + FmtUs(m_g.fallUs), CRect(xr1, y + sc(3), xf1, y + sc(17)), DT_CENTER | DT_SINGLELINE | DT_NOCLIP);
        if (shown > 1) {
            dc.SetTextColor(cChirp);
            CString b; b.Format(_T("burst x%u%s"), m_p.burst, m_p.burst > shown ? _T(" (first 4 drawn)") : _T(""));
            dc.DrawText(b, CRect(xf1, y + sc(3), chirpsEnd, y + sc(17)), DT_CENTER | DT_SINGLELINE | DT_NOCLIP);
        }
        CString gap;
        if (m_p.mode == 1)      gap.Format(_T("interval %u ms"), m_p.intervalMs);
        else if (m_p.mode == 2) gap = _T("wait TRIGGER");
        else                    gap = _T("IDLE: trigger only");
        dc.SetTextColor(cMuted);
        dc.DrawText(gap, CRect(breakX - sc(70), y + sc(3), breakX + sc(70), y + sc(17)), DT_CENTER | DT_SINGLELINE | DT_NOCLIP);
        dc.SetTextColor(cNext);
        dc.DrawText(_T("next cycle"), CRect(nextX0, plot.top + sc(2), plot.right, plot.top + sc(16)),
                    DT_RIGHT | DT_SINGLELINE | DT_NOCLIP);
    }
    // Dimension row 2: period / frequency across one chirp
    {
        int y = plot.bottom + sc(12) + dimRow;
        int xr0 = X(0), xf1 = X(m_g.periodUs);
        DimLine(dc, xr0, xf1, y, cChirp, sc(5));
        CString s;
        s.Format(_T("T = %s   f = %.3f Hz   (%s)"), FmtUs(m_g.periodUs).GetString(), m_g.freqHz,
                 m_g.rampMode ? _T("ramp mode: rise/fall") : _T("freq mode: symmetric"));
        dc.SetTextColor(cChirp);
        dc.DrawText(s, CRect(xr0, y + sc(3), (std::max)(xf1, xr0 + sc(320)), y + sc(17)), DT_CENTER | DT_SINGLELINE | DT_NOCLIP);
    }
    // Info lines: what one capture at the fixed ADC rate has to hold, and
    // whether the whole burst (every rise and fall) fits into it.
    dc.SelectObject(&m_font);
    {
        const uint32_t burst = (std::max)(1u, m_p.burst);
        CString s;
        s.Format(_T("ADC @ %.0f MS/s: chirp = rise %u + fall %u = %u samples (%s)"),
                 core::kChirpAdcHz / 1e6, m_g.riseSamples, m_g.samplesPerChirp - m_g.riseSamples,
                 m_g.samplesPerChirp, FmtUs(m_g.periodUs).GetString());
        if (burst > 1) {
            CString b; b.Format(_T(";  burst x%u = %llu samples (%s)"), burst,
                                static_cast<unsigned long long>(m_g.burstSamplesNeeded), FmtUs(m_g.burstUs).GetString());
            s += b;
        }
        dc.SetTextColor(cText);
        dc.DrawText(s, CRect(full.left + sc(10), full.bottom - 2 * infoLine - sc(4), full.right - sc(6), full.bottom - infoLine - sc(4)),
                    DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

        const uint32_t maxCapture = (core::kChirpCaptureMax / core::kChirpDmaChunk) * core::kChirpDmaChunk;
        if (m_g.fitsInCapture) {
            s.Format(_T("OK: fits in ONE capture window of %u samples (%s, %u DMA chunks x %u) = %.1f chirps; MCU buffer %u samples (%s), %.0f%% used"),
                     m_g.captureTarget, FmtUs(m_g.captureUs).GetString(), m_g.chunks, core::kChirpDmaChunk,
                     m_g.chirpsCaptured, maxCapture, FmtUs(maxCapture * 1e6 / core::kChirpAdcHz).GetString(),
                     100.0 * static_cast<double>(m_g.burstSamplesNeeded) / maxCapture);
            dc.SetTextColor(th.ok);
        } else if (m_g.clippedByOverride) {
            s.Format(_T("DOES NOT FIT: samples override %u cuts the burst to %u samples (%s) = %.2f chirps of %u; set samples to 0 (auto) or >= %llu"),
                     m_p.samplesOvr, m_g.captureTarget, FmtUs(m_g.captureUs).GetString(), m_g.chirpsCaptured, burst,
                     static_cast<unsigned long long>(m_g.burstSamplesNeeded));
            dc.SetTextColor(th.danger);
        } else {
            s.Format(_T("DOES NOT FIT: burst needs %llu samples (%s), MCU capture holds %u (%s) = %.2f chirps of %u; reduce burst or shorten the chirp"),
                     static_cast<unsigned long long>(m_g.burstSamplesNeeded), FmtUs(m_g.burstUs).GetString(),
                     m_g.captureTarget, FmtUs(m_g.captureUs).GetString(), m_g.chirpsCaptured, burst);
            dc.SetTextColor(th.danger);
        }
        dc.DrawText(s, CRect(full.left + sc(10), full.bottom - infoLine - sc(4), full.right - sc(6), full.bottom - sc(4)),
                    DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    }
    dc.SelectObject(of);
}
