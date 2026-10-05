#pragma once
//
// Dpi.h -- per-monitor DPI helpers (no dependency on the Windows 10 SDK
// version: GetDpiForWindow is resolved at run time).
//

#include "pch.h"

#ifndef WM_DPICHANGED_AFTERPARENT
#define WM_DPICHANGED_AFTERPARENT 0x02E3
#endif

namespace Dpi {

inline UINT Of(HWND hwnd)
{
    using Fn = UINT (WINAPI*)(HWND);
    static Fn fn = reinterpret_cast<Fn>(
        ::GetProcAddress(::GetModuleHandleW(L"user32.dll"), "GetDpiForWindow"));
    if (fn && hwnd) {
        UINT d = fn(hwnd);
        if (d) return d;
    }
    HDC dc = ::GetDC(nullptr);
    int d = ::GetDeviceCaps(dc, LOGPIXELSX);
    ::ReleaseDC(nullptr, dc);
    return d > 0 ? static_cast<UINT>(d) : 96u;
}

inline int Scale(HWND hwnd, int px96) { return ::MulDiv(px96, static_cast<int>(Of(hwnd)), 96); }

// Negative logical height for CreateFont from a point size.
inline int FontHeight(HWND hwnd, int pt) { return -::MulDiv(pt, static_cast<int>(Of(hwnd)), 72); }

inline void MakeFont(CFont& f, HWND hwnd, int pt, bool bold = false, LPCTSTR face = _T("Segoe UI"))
{
    if (f.GetSafeHandle()) f.DeleteObject();
    f.CreateFont(FontHeight(hwnd, pt), 0, 0, 0, bold ? FW_BOLD : FW_NORMAL, FALSE, FALSE, FALSE,
                 DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                 VARIABLE_PITCH | FF_SWISS, face);
}

// Pixel width of a control's caption in its own font.
inline int TextWidth(CWnd& w)
{
    if (!w.GetSafeHwnd()) return 0;
    CString t; w.GetWindowText(t);
    if (t.IsEmpty()) return 0;
    CClientDC dc(&w);
    CFont* f = w.GetFont();
    CFont* old = f ? dc.SelectObject(f) : nullptr;
    const int cx = dc.GetTextExtent(t).cx;
    if (old) dc.SelectObject(old);
    return cx;
}

// Height of one text line in the control's font (for multi-line read-outs).
inline int LineHeight(CWnd& w)
{
    if (!w.GetSafeHwnd()) return 16;
    CClientDC dc(&w);
    CFont* f = w.GetFont();
    CFont* old = f ? dc.SelectObject(f) : nullptr;
    const int cy = dc.GetTextExtent(_T("Xg")).cy;
    if (old) dc.SelectObject(old);
    return cy > 0 ? cy : 16;
}

// Width a control needs for its caption (buttons, check boxes, radios and
// static labels), never less than `fallbackPx`. Other controls (combos,
// edits, sliders) keep the fallback. Used so translated captions always fit.
inline int FitWidth(CWnd& w, int fallbackPx)
{
    if (!w.GetSafeHwnd()) return fallbackPx;
    TCHAR cls[32]{};
    ::GetClassName(w.GetSafeHwnd(), cls, 31);
    const int tw = TextWidth(w);
    if (tw == 0) return fallbackPx;
    const HWND h = w.GetSafeHwnd();
    if (_tcsicmp(cls, _T("Button")) == 0) {
        const LONG style = ::GetWindowLong(h, GWL_STYLE) & BS_TYPEMASK;
        const bool box = (style == BS_AUTOCHECKBOX || style == BS_CHECKBOX || style == BS_AUTORADIOBUTTON ||
                          style == BS_RADIOBUTTON || style == BS_3STATE || style == BS_AUTO3STATE);
        return (std::max)(fallbackPx, tw + Scale(h, box ? 26 : 18));
    }
    if (_tcsicmp(cls, _T("Static")) == 0) return (std::max)(fallbackPx, tw + Scale(h, 6));
    return fallbackPx;
}

inline void MakeMonoFont(CFont& f, HWND hwnd, int pt)
{
    if (f.GetSafeHandle()) f.DeleteObject();
    f.CreateFont(FontHeight(hwnd, pt), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                 DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                 FIXED_PITCH | FF_MODERN, _T("Consolas"));
}

} // namespace Dpi
