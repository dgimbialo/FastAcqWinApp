#pragma once
//
// Theme -- centralized light color palette shared by every view.
// Keeping the colors in one place lets the whole UI stay visually consistent.
//

#include "pch.h"

namespace Theme {

// -- Surfaces ---------------------------------------------------------------
constexpr COLORREF Bg       = RGB(238, 240, 244);   // window base / gaps
constexpr COLORREF Panel    = RGB(248, 249, 251);   // command bar, footers
constexpr COLORREF PanelAlt = RGB(231, 234, 239);   // alternating rows / raised
constexpr COLORREF Plot     = RGB(255, 255, 255);   // plot canvas
constexpr COLORREF PlotAxis = RGB(243, 245, 248);   // axis strips
constexpr COLORREF Border   = RGB(198, 203, 212);   // separators / outlines

// -- Text -------------------------------------------------------------------
constexpr COLORREF Text     = RGB( 32,  36,  44);
constexpr COLORREF TextDim  = RGB(108, 116, 128);
constexpr COLORREF TextHdr  = RGB( 36,  70, 125);

// -- Accents ----------------------------------------------------------------
constexpr COLORREF Accent    = RGB(  0, 102, 204);  // primary (blue)
constexpr COLORREF AccentOk  = RGB( 22, 145, 105);  // success / connect
constexpr COLORREF AccentWarn= RGB(205, 125,  20);  // caution
constexpr COLORREF Danger    = RGB(200,  60,  60);  // destructive (clear)

// -- Plot elements ----------------------------------------------------------
constexpr COLORREF Grid     = RGB(224, 227, 233);
constexpr COLORREF Axis     = RGB(120, 145, 132);
constexpr COLORREF AxisText = RGB( 85, 105,  95);
constexpr COLORREF Wave     = RGB( 20, 140,  60);
constexpr COLORREF Peak     = RGB(215, 145,   0);

// Scale a color's brightness by factor f (clamped to 0..255).
inline COLORREF Shade(COLORREF c, double f)
{
    auto cl = [&](int v) { int r = static_cast<int>(v * f + 0.5);
                           return r < 0 ? 0 : (r > 255 ? 255 : r); };
    return RGB(cl(GetRValue(c)), cl(GetGValue(c)), cl(GetBValue(c)));
}

} // namespace Theme
