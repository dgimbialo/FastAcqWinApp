#include "pch.h"
#include "WaterfallView.h"
#include "ColorMap.h"

#include <algorithm>
#include <cstring>

BEGIN_MESSAGE_MAP(WaterfallView, CWnd)
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
    ON_WM_SIZE()
END_MESSAGE_MAP()

WaterfallView::~WaterfallView()
{
    FreeDib();
}

BOOL WaterfallView::CreateView(CWnd* parent, UINT id)
{
    LPCTSTR cls = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW,
                                      ::LoadCursor(nullptr, IDC_ARROW),
                                      reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)),
                                      nullptr);
    return Create(cls, nullptr, WS_CHILD | WS_VISIBLE | WS_BORDER,
                  CRect(0, 0, 10, 10), parent, id);
}

void WaterfallView::OnSize(UINT t, int cx, int cy)
{
    CWnd::OnSize(t, cx, cy);
    Invalidate(FALSE);
}

void WaterfallView::FreeDib()
{
    if (m_hDib) { ::DeleteObject(m_hDib); m_hDib = nullptr; }
    m_pPixels  = nullptr;
    m_dibWidth = 0;
}

void WaterfallView::EnsureDib(int width)
{
    if (width <= 0) return;
    if (m_hDib && width == m_dibWidth) return;

    FreeDib();
    BITMAPINFO bi{};
    bi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth       = width;
    bi.bmiHeader.biHeight      = -m_dibHeight;   // top-down
    bi.bmiHeader.biPlanes      = 1;
    bi.bmiHeader.biBitCount    = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    HDC hdc = ::GetDC(nullptr);
    m_hDib = ::CreateDIBSection(hdc, &bi, DIB_RGB_COLORS,
                                reinterpret_cast<void**>(&m_pPixels),
                                nullptr, 0);
    ::ReleaseDC(nullptr, hdc);

    m_dibWidth = width;
    if (m_pPixels)
        std::memset(m_pPixels, 0, static_cast<size_t>(width) * m_dibHeight * 4);
}

void WaterfallView::Clear()
{
    if (m_pPixels)
        std::memset(m_pPixels, 0, static_cast<size_t>(m_dibWidth) * m_dibHeight * 4);
    m_maxSeen = 1.0f;
    if (::IsWindow(m_hWnd)) Invalidate(FALSE);
}

void WaterfallView::PushFft(const float* mag, size_t n)
{
    if (!mag || n == 0 || !::IsWindow(m_hWnd)) return;

    // Only use first half of FFT (real signal -> symmetric).
    size_t use = n / 2;
    if (use == 0) use = n;

    // Width of the DIB == use (cap at 4096).
    int w = static_cast<int>((std::min)(use, static_cast<size_t>(4096)));
    EnsureDib(w);
    if (!m_pPixels) return;

    // Update running max with slow decay.
    m_maxSeen *= 0.995f;
    for (size_t i = 0; i < use; ++i)
        if (_finite(mag[i]) && mag[i] > m_maxSeen) m_maxSeen = mag[i];
    if (m_maxSeen < 1e-6f) m_maxSeen = 1e-6f;

    // Scroll pixels down by one row.
    const size_t rowBytes = static_cast<size_t>(m_dibWidth) * 4;
    std::memmove(reinterpret_cast<uint8_t*>(m_pPixels) + rowBytes,
                 m_pPixels,
                 rowBytes * (m_dibHeight - 1));

    // Fill new top row from FFT using Jet palette.
    const COLORREF* jet = ColorMap::Jet();
    DWORD* row = m_pPixels;
    for (int x = 0; x < m_dibWidth; ++x) {
        size_t srcIdx = (static_cast<size_t>(x) * use) / static_cast<size_t>(m_dibWidth);
        // Guard against NaN/inf: comparisons with NaN always return false,
        // so a raw clamp would leave v == NaN and cause static_cast<int>(NaN)
        // to produce 0x80000000 on x64, sending jet[] wildly out of bounds.
        float v = mag[srcIdx];
        if (!_finite(v) || v < 0.0f) v = 0.0f;
        v /= m_maxSeen;
        if (v > 1.0f) v = 1.0f;
        int idx = static_cast<int>(v * 255.0f + 0.5f);
        if (idx < 0)   idx = 0;
        if (idx > 255) idx = 255;
        COLORREF c = jet[idx];
        // COLORREF is 0x00BBGGRR, DIB pixel 32bpp is 0x00RRGGBB.
        row[x] = (GetRValue(c) << 16) | (GetGValue(c) << 8) | GetBValue(c);
    }

    Invalidate(FALSE);
}

void WaterfallView::OnPaint()
{
    CPaintDC dc(this);
    CRect rc; GetClientRect(&rc);
    dc.FillSolidRect(rc, RGB(0, 0, 0));
    if (!m_hDib || m_dibWidth <= 0) return;

    BITMAPINFO bi{};
    bi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth       = m_dibWidth;
    bi.bmiHeader.biHeight      = -m_dibHeight;
    bi.bmiHeader.biPlanes      = 1;
    bi.bmiHeader.biBitCount    = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    ::SetStretchBltMode(dc.GetSafeHdc(), HALFTONE);
    ::StretchDIBits(dc.GetSafeHdc(),
                    rc.left, rc.top, rc.Width(), rc.Height(),
                    0, 0, m_dibWidth, m_dibHeight,
                    m_pPixels, &bi, DIB_RGB_COLORS, SRCCOPY);
}
