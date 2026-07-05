#include "pch.h"
#include "SpectrumView.h"
#include "ColorMap.h"

#include <algorithm>
#include <cmath>

BEGIN_MESSAGE_MAP(SpectrumView, CWnd)
    ON_WM_CREATE()
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
    ON_WM_SIZE()
    ON_WM_VSCROLL()
END_MESSAGE_MAP()

// Vertical scale is LOGARITHMIC: magnitudes are mapped to dB relative to the
// running maximum, with a fixed display floor. 0 dB = top, kDbFloor = bottom.
static constexpr float kDbFloor = -60.0f;

static float NormDb(float v, float maxSeen)
{
    if (v <= 0.0f || maxSeen <= 0.0f) return 0.0f;
    float db = 20.0f * log10f(v / maxSeen);      /* <= 0 dB */
    float nv = (db - kDbFloor) / (0.0f - kDbFloor);
    if (nv < 0.0f) nv = 0.0f;
    if (nv > 1.0f) nv = 1.0f;
    return nv;
}

static CString FormatFreqLabel(float hz)
{
    long long iHz = static_cast<long long>(hz + 0.5f);
    CString s;
    if (iHz >= 1000)
        s.Format(_T("%lld.%03lld kHz"), iHz / 1000, iHz % 1000);
    else
        s.Format(_T("%lld Hz"), iHz);
    return s;
}

BOOL SpectrumView::CreateView(CWnd* parent, UINT id)
{
    LPCTSTR cls = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW,
                                      ::LoadCursor(nullptr, IDC_ARROW),
                                      reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1),
                                      nullptr);
    return Create(cls, nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER,
                  CRect(0, 0, 10, 10), parent, id);
}

int SpectrumView::OnCreate(LPCREATESTRUCT lpcs)
{
    if (CWnd::OnCreate(lpcs) == -1) return -1;

    m_smallFont.CreateFont(-9, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                           DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                           CLEARTYPE_QUALITY, VARIABLE_PITCH | FF_SWISS, _T("Segoe UI"));

    m_axisFont.CreateFont(-9, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                          DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                          CLEARTYPE_QUALITY, VARIABLE_PITCH | FF_SWISS, _T("Segoe UI"));

    m_peakFont.CreateFont(-18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                          DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                          CLEARTYPE_QUALITY, VARIABLE_PITCH | FF_SWISS, _T("Segoe UI"));

    m_slider.Create(WS_CHILD | WS_VISIBLE | TBS_VERT | TBS_REVERSED | TBS_NOTICKS,
                    CRect(0, 0, kSliderW, 100), this, 5001);
    m_slider.SetRange(0, 100);
    m_slider.SetPos(95); // 5% threshold after inversion in OnVScroll
    m_thresholdPct = 0.05f;
    return 0;
}

void SpectrumView::OnSize(UINT t, int cx, int cy)
{
    CWnd::OnSize(t, cx, cy);
    if (m_slider.GetSafeHwnd()) {
        int panelH = cy - kAxisH;
        if (panelH < 10) panelH = 10;
        int plotW = cx - kPanelW;
        m_slider.MoveWindow(plotW, 0, kSliderW, panelH);
    }
    m_cachedPlotW = -1;
    Invalidate(FALSE);
}

void SpectrumView::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pBar)
{
    if (pBar && pBar->GetSafeHwnd() == m_slider.GetSafeHwnd()) {
        CWnd::OnVScroll(nSBCode, nPos, pBar);
        m_thresholdPct = 1.0f - static_cast<float>(m_slider.GetPos()) / 100.0f;
        if (m_thresholdPct < 0.0f) m_thresholdPct = 0.0f;
        if (m_thresholdPct > 1.0f) m_thresholdPct = 1.0f;

        CRect sliderRc;
        m_slider.GetWindowRect(&sliderRc);
        ScreenToClient(&sliderRc);

        CRect leftRc;
        GetClientRect(&leftRc);
        leftRc.right = sliderRc.left;
        if (!leftRc.IsRectEmpty())
            InvalidateRect(&leftRc, FALSE);

        CRect rightRc;
        GetClientRect(&rightRc);
        rightRc.left = sliderRc.right;
        if (!rightRc.IsRectEmpty())
            InvalidateRect(&rightRc, FALSE);
        return;
    }
    CWnd::OnVScroll(nSBCode, nPos, pBar);
}

// ?????????????????????????????????????????????????????????????????????????????
void SpectrumView::SetFft(const float* mag, size_t n, float freqResHz)
{
    size_t use = n / 2;
    if (use == 0) use = n;
    m_mag.assign(mag, mag + use);
    m_freqRes = freqResHz;

    m_maxSeen *= 0.9f;
    for (float v : m_mag) if (v > m_maxSeen) m_maxSeen = v;
    if (m_maxSeen < 1e-6f) m_maxSeen = 1e-6f;

    m_cachedPlotW = -1;
    if (::IsWindow(m_hWnd)) Invalidate(FALSE);
}

void SpectrumView::RebuildCaches(int plotWidth)
{
    m_cachedPlotW = plotWidth;
    m_colNorm.clear();
    m_peakCandidates.clear();

    if (plotWidth <= 0 || m_mag.empty() || m_maxSeen <= 0.0f) return;

    const size_t nb = m_mag.size();
    m_colNorm.resize(static_cast<size_t>(plotWidth));
    for (int x = 0; x < plotWidth; ++x) {
        size_t i0 = (static_cast<size_t>(x) * nb) / static_cast<size_t>(plotWidth);
        size_t i1 = (static_cast<size_t>(x + 1) * nb) / static_cast<size_t>(plotWidth);
        if (i1 <= i0) i1 = i0 + 1;
        if (i1 > nb)  i1 = nb;
        float v = 0.0f;
        for (size_t i = i0; i < i1; ++i) if (m_mag[i] > v) v = m_mag[i];
        m_colNorm[static_cast<size_t>(x)] = NormDb(v, m_maxSeen);   /* log scale */
    }

    if (nb >= 3) {
        m_peakCandidates.reserve(nb / 8);
        for (size_t i = 1; i + 1 < nb; ++i) {
            float nv = NormDb(m_mag[i], m_maxSeen);                 /* log scale */
            if (m_mag[i] > m_mag[i - 1] && m_mag[i] >= m_mag[i + 1])
                m_peakCandidates.push_back({ i, nv, m_freqRes * static_cast<float>(i) });
        }
        std::sort(m_peakCandidates.begin(), m_peakCandidates.end(),
                  [](const Peak& a, const Peak& b) { return a.normMag > b.normMag; });
    }
}

std::vector<SpectrumView::Peak> SpectrumView::DetectPeaks() const
{
    std::vector<Peak> out;
    out.reserve(m_peakCandidates.size());
    for (const auto& p : m_peakCandidates) {
        if (p.normMag >= m_thresholdPct)
            out.push_back(p);
    }
    return out;
}

// ?????????????????????????????????????????????????????????????????????????????
void SpectrumView::DrawPeakTable(CDC& dc, const CRect& panelRc,
                                 const std::vector<Peak>& peaks)
{
    CRect tableRc = panelRc;
    tableRc.left += kSliderW;

    const int headerH = 18;
    const int rowH    = 16;
    const int noColW  = 34;

    // Native look: white body, system button-face header strip.
    dc.FillSolidRect(tableRc, ::GetSysColor(COLOR_WINDOW));
    CRect hdrRc(tableRc.left, tableRc.top, tableRc.right, tableRc.top + headerH);
    dc.FillSolidRect(hdrRc, ::GetSysColor(COLOR_BTNFACE));

    // Zebra striping for the data rows.
    {
        int ry = tableRc.top + headerH;
        int band = 0;
        while (ry < tableRc.bottom) {
            if (band & 1)
                dc.FillSolidRect(tableRc.left, ry, tableRc.Width(), rowH, RGB(240,240,240));
            ry += rowH;
            ++band;
        }
    }

    CPen gridPen(PS_SOLID, 1, RGB(180,180,180));
    CPen* pOldPen = dc.SelectObject(&gridPen);
    CBrush* pOldBrush = static_cast<CBrush*>(dc.SelectStockObject(NULL_BRUSH));

    dc.Rectangle(tableRc);
    dc.MoveTo(tableRc.left + noColW, tableRc.top);
    dc.LineTo(tableRc.left + noColW, tableRc.bottom);
    dc.MoveTo(tableRc.left, tableRc.top + headerH);
    dc.LineTo(tableRc.right, tableRc.top + headerH);

    int y = tableRc.top + headerH;
    while (y < tableRc.bottom) {
        dc.MoveTo(tableRc.left, y);
        dc.LineTo(tableRc.right, y);
        y += rowH;
    }
    if (pOldBrush) dc.SelectObject(pOldBrush);
    dc.SelectObject(pOldPen);

    CFont* pOldFont = dc.SelectObject(&m_smallFont);
    dc.SetBkMode(TRANSPARENT);

    CRect noHdr(tableRc.left + 2, tableRc.top + 2, tableRc.left + noColW - 2, tableRc.top + headerH - 2);
    CRect fqHdr(tableRc.left + noColW + 2, tableRc.top + 2, tableRc.right - 2, tableRc.top + headerH - 2);
    dc.SetTextColor(::GetSysColor(COLOR_WINDOWTEXT));
    dc.DrawText(_T("#"), noHdr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    dc.DrawText(_T("Frequency"), fqHdr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    int rowTop = tableRc.top + headerH;
    int rank = 1;
    for (const auto& p : peaks) {
        if (rowTop + rowH > tableRc.bottom) break;

        CString idxStr;
        idxStr.Format(_T("%d"), rank);
        CString freqStr = FormatFreqLabel(p.freqHz);

        CRect idxRc(tableRc.left + 2, rowTop + 1, tableRc.left + noColW - 2, rowTop + rowH - 1);
        CRect freqRc(tableRc.left + noColW + 2, rowTop + 1, tableRc.right - 2, rowTop + rowH - 1);
        dc.SetTextColor(RGB(110,110,110));
        dc.DrawText(idxStr, idxRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        dc.SetTextColor(rank == 1 ? RGB(180,90,0) : ::GetSysColor(COLOR_WINDOWTEXT));
        dc.DrawText(freqStr, freqRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        rowTop += rowH;
        ++rank;
    }

    dc.SelectObject(pOldFont);
}

// ?????????????????????????????????????????????????????????????????????????????
void SpectrumView::OnPaint()
{
    CPaintDC dc(this);
    CRect rc; GetClientRect(&rc);
    if (rc.Width() <= 2 || rc.Height() <= 2) return;

    CDC mem;
    mem.CreateCompatibleDC(&dc);
    CBitmap bmp;
    bmp.CreateCompatibleBitmap(&dc, rc.Width(), rc.Height());
    CBitmap* pOldBmp = mem.SelectObject(&bmp);

    Render(mem, rc);

    dc.BitBlt(0, 0, rc.Width(), rc.Height(), &mem, 0, 0, SRCCOPY);
    mem.SelectObject(pOldBmp);
}

void SpectrumView::Render(CDC& dc, const CRect& rc)
{
    dc.FillSolidRect(rc, ::GetSysColor(COLOR_WINDOW));

    const int plotW = rc.Width() - kPanelW;
    if (plotW < 8) return;

    // Sub-rectangles.
    CRect plotRc(rc.left,         rc.top,             rc.left + plotW,  rc.bottom - kAxisH);
    CRect axisRc (rc.left,         rc.bottom - kAxisH, rc.left + plotW,  rc.bottom);
    CRect panelRc(rc.left + plotW, rc.top,             rc.right,         rc.bottom - kAxisH);

    dc.FillSolidRect(axisRc, ::GetSysColor(COLOR_BTNFACE));

    const int w = plotRc.Width();
    const int h = plotRc.Height();

    if (m_cachedPlotW != w)
        RebuildCaches(w);

    // ?? Grid (log scale: each line is a fixed dB step) ???????????????????????
    CPen gridPen(PS_SOLID, 1, RGB(210,210,210));
    CPen* pOldPen = dc.SelectObject(&gridPen);
    CFont* pGridFont = dc.SelectObject(&m_axisFont);
    dc.SetBkMode(TRANSPARENT);
    for (int i = 1; i < 5; ++i) {
        int y = plotRc.top + h * i / 5;
        dc.MoveTo(plotRc.left, y);
        dc.LineTo(plotRc.right, y);
        CString dbLbl;
        dbLbl.Format(_T("%.0f dB"), kDbFloor * static_cast<float>(i) / 5.0f);
        dc.SetTextColor(RGB(80,80,80));
        dc.TextOut(plotRc.left + 2, y - 12, dbLbl);
    }
    dc.SelectObject(pGridFont);
    dc.SelectObject(pOldPen);

    // ?? Title ?????????????????????????????????????????????????????????????????
    if (!m_title.IsEmpty()) {
        dc.SetBkMode(TRANSPARENT);
        dc.SetTextColor(::GetSysColor(COLOR_WINDOWTEXT));
        dc.TextOut(rc.left + 4, rc.top + 3, m_title);
    }

    if (m_mag.empty()) {
        DrawPeakTable(dc, panelRc, {});
        return;
    }

    const size_t nb = m_mag.size();
    const COLORREF* jet = ColorMap::Jet();
    auto peaks = DetectPeaks();

    // ?? Spectrum bars ????????????????????????????????????????????????????????
    for (int x = 0; x < w; ++x) {
        float nv = (x >= 0 && x < static_cast<int>(m_colNorm.size())) ? m_colNorm[static_cast<size_t>(x)] : 0.0f;
        int barH = static_cast<int>(nv * h);
        int idx = static_cast<int>(nv * 255.0f + 0.5f);
        if (idx > 255) idx = 255;
        COLORREF c = jet[idx];

        if (barH > 0)
            dc.FillSolidRect(plotRc.left + x, plotRc.bottom - barH, 1, barH, c);
    }

    // ?? Threshold horizontal line (yellow) ???????????????????????????????????
    int thrY = plotRc.bottom - static_cast<int>(m_thresholdPct * h);
    {
        CPen thrPen(PS_DOT, 1, RGB(180,90,0));
        CPen* pS = dc.SelectObject(&thrPen);
        dc.MoveTo(plotRc.left,  thrY);
        dc.LineTo(plotRc.right, thrY);
        dc.SelectObject(pS);
    }

    // ?? Peak triangles + labels (only above-threshold peaks) ?????????????????
    CBrush markBrush(RGB(180,90,0));
    CFont* pOldFont = dc.SelectObject(&m_peakFont);
    dc.SetBkMode(TRANSPARENT);
    dc.SetTextColor(RGB(180,90,0));

    int rank = 1;
    for (const auto& pk : peaks) {
        int px = plotRc.left + static_cast<int>((pk.bin * static_cast<size_t>(w)) / nb);
        float nv = pk.normMag > 1.0f ? 1.0f : pk.normMag;
        int barTop = plotRc.bottom - static_cast<int>(nv * h);

        CRect markRc(px - 4, barTop - 8, px + 5, barTop + 1);
        dc.FillSolidRect(markRc, RGB(180,90,0));

        CString idxLbl;
        idxLbl.Format(_T("%d"), rank++);
        CRect idxRc(px - 12, barTop - 28, px + 12, barTop - 8);
        if (idxRc.top < plotRc.top) idxRc.OffsetRect(0, plotRc.top - idxRc.top);
        dc.DrawText(idxLbl, idxRc, DT_CENTER | DT_TOP | DT_SINGLELINE);
    }
    dc.SelectObject(pOldFont);

    // ?? Frequency X-axis labels ???????????????????????????????????????????????
    if (m_freqRes > 0.0f) {
        float maxFreqKHz = m_freqRes * static_cast<float>(nb) / 1000.0f;
        float stepKHz = 1.0f;
        const float steps[] = { 0.1f, 0.2f, 0.5f, 1.0f, 2.0f, 5.0f, 10.0f, 20.0f, 50.0f, 100.0f, 200.0f, 500.0f };
        for (float s : steps) {
            if (maxFreqKHz / s <= 7.0f) { stepKHz = s; break; }
        }

        CFont* pAxisFont = dc.SelectObject(&m_axisFont);
        dc.SetBkMode(TRANSPARENT);
        CPen tickPen(PS_SOLID, 1, RGB(90,90,90));
        for (float fKHz = 0.0f; fKHz <= maxFreqKHz * 1.001f; fKHz += stepKHz) {
            int x = rc.left + static_cast<int>((fKHz / maxFreqKHz) * w);
            CPen* pSave = dc.SelectObject(&tickPen);
            dc.MoveTo(x, plotRc.bottom);
            dc.LineTo(x, plotRc.bottom + 3);
            dc.SelectObject(pSave);

            CString lbl;
            if (stepKHz < 1.0f)
                lbl.Format(_T("%.0f"), fKHz * 1000.0f);
            else
                lbl.Format(_T("%.0fk"), fKHz);

            dc.SetTextColor(RGB(80,80,80));
            CRect tickLblRc(x - 18, axisRc.top + 2, x + 18, axisRc.bottom);
            dc.DrawText(lbl, tickLblRc, DT_CENTER | DT_TOP | DT_SINGLELINE);
        }
        dc.SelectObject(pAxisFont);
    }

    DrawPeakTable(dc, panelRc, peaks);
}
