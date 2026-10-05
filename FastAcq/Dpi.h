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

inline void MakeMonoFont(CFont& f, HWND hwnd, int pt)
{
    if (f.GetSafeHandle()) f.DeleteObject();
    f.CreateFont(FontHeight(hwnd, pt), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                 DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                 FIXED_PITCH | FF_MODERN, _T("Consolas"));
}

} // namespace Dpi
