#pragma once
//
// WaterfallView -- DIBSection-backed waterfall (newest row at top).
//

#include "pch.h"

class WaterfallView : public CWnd {
public:
    WaterfallView() = default;
    ~WaterfallView() override;

    BOOL CreateView(CWnd* parent, UINT id);

    // Push a new FFT magnitude row. Internally normalized by peak over recent rows.
    void PushFft(const float* mag, size_t n);

    void Clear();

protected:
    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC*) { return TRUE; }
    afx_msg void OnSize(UINT, int, int);
    DECLARE_MESSAGE_MAP()

private:
    void EnsureDib(int width);
    void FreeDib();

    HBITMAP m_hDib{nullptr};
    DWORD*  m_pPixels{nullptr};     // 32bpp ARGB (top-down)
    int     m_dibWidth{0};
    int     m_dibHeight{256};       // history rows

    float   m_maxSeen{1.0f};        // running max for auto-range
};
