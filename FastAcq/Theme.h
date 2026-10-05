#pragma once
//
// Theme.h -- light / dark palettes shared by every custom-drawn view.
//

#include "pch.h"

namespace Theme {

struct Palette {
    // Surfaces
    COLORREF bg;          // window base / gaps
    COLORREF panel;       // tool strips, footers
    COLORREF panelAlt;    // alternating rows
    COLORREF plot;        // plot canvas
    COLORREF axisStrip;   // axis background
    COLORREF border;
    // Text
    COLORREF text;
    COLORREF textDim;
    COLORREF textHdr;
    // Plot elements
    COLORREF grid;
    COLORREF gridMinor;
    COLORREF axis;
    COLORREF axisText;
    COLORREF traceUp;
    COLORREF traceDn;
    COLORREF traceAvg;
    COLORREF threshold;
    COLORREF noise;
    COLORREF peak;
    COLORREF markerA;
    COLORREF markerB;
    COLORREF cursor;
    COLORREF readoutBg;
    COLORREF readoutText;
    COLORREF segUp;
    COLORREF segDn;
    COLORREF segGuard;
    COLORREF wave;
    // Accents
    COLORREF accent;
    COLORREF ok;
    COLORREF warn;
    COLORREF danger;
};

inline const Palette& Light()
{
    static const Palette p = {
        RGB(238, 240, 244), RGB(248, 249, 251), RGB(231, 234, 239), RGB(255, 255, 255), RGB(243, 245, 248), RGB(198, 203, 212),
        RGB( 32,  36,  44), RGB(108, 116, 128), RGB( 36,  70, 125),
        RGB(224, 227, 233), RGB(240, 242, 246), RGB(120, 130, 145), RGB( 85,  95, 110),
        RGB( 20, 110, 200), RGB(215, 100,  20), RGB(120, 120, 120), RGB(200,  40,  40), RGB(140, 140, 140),
        RGB(190,  90,   0), RGB(  0, 150,  90), RGB(150,   0, 160), RGB( 90,  90,  90), RGB(255, 255, 230), RGB( 30,  30,  30),
        RGB(222, 240, 225), RGB(222, 232, 245), RGB(235, 235, 235), RGB(  0,  90, 180),
        RGB(  0, 102, 204), RGB( 22, 145, 105), RGB(205, 125,  20), RGB(200,  60,  60)
    };
    return p;
}

inline const Palette& Dark()
{
    static const Palette p = {
        RGB( 30,  32,  36), RGB( 40,  43,  48), RGB( 48,  52,  58), RGB( 18,  19,  22), RGB( 34,  36,  40), RGB( 70,  74,  82),
        RGB(225, 228, 232), RGB(150, 156, 165), RGB(130, 170, 230),
        RGB( 52,  56,  64), RGB( 40,  43,  50), RGB(130, 140, 155), RGB(165, 172, 182),
        RGB( 90, 170, 255), RGB(255, 150,  60), RGB(170, 170, 170), RGB(255,  90,  90), RGB(140, 140, 140),
        RGB(255, 190,  60), RGB( 60, 220, 140), RGB(220, 120, 255), RGB(200, 200, 200), RGB( 45,  48,  55), RGB(235, 235, 235),
        RGB( 28,  50,  34), RGB( 28,  38,  58), RGB( 40,  40,  44), RGB( 80, 170, 255),
        RGB( 70, 140, 230), RGB( 60, 190, 130), RGB(230, 160,  50), RGB(230,  90,  90)
    };
    return p;
}

inline bool& DarkFlag() { static bool dark = false; return dark; }
inline bool IsDark() { return DarkFlag(); }
inline void SetDark(bool dark) { DarkFlag() = dark; }
inline const Palette& Get() { return IsDark() ? Dark() : Light(); }

// Background brush for editable fields (edit boxes, combo lists): white in
// the light theme, the plot canvas colour in the dark one. Read-only edits
// are painted through CTLCOLOR_STATIC and keep the window background, so an
// editable field is always distinguishable from a computed read-out.
inline HBRUSH FieldBrush()
{
    static CBrush light, dark;
    CBrush& b = IsDark() ? dark : light;
    if (!b.GetSafeHandle()) b.CreateSolidBrush(Get().plot);
    return static_cast<HBRUSH>(b.GetSafeHandle());
}

// Scale a color's brightness by factor f (clamped to 0..255).
inline COLORREF Shade(COLORREF c, double f)
{
    auto cl = [&](int v) { int r = static_cast<int>(v * f + 0.5);
                           return r < 0 ? 0 : (r > 255 ? 255 : r); };
    return RGB(cl(GetRValue(c)), cl(GetGValue(c)), cl(GetBValue(c)));
}

// Linear blend a -> b by t in [0,1].
inline COLORREF Blend(COLORREF a, COLORREF b, double t)
{
    auto l = [&](int x, int y) { return static_cast<int>(x + (y - x) * t + 0.5); };
    return RGB(l(GetRValue(a), GetRValue(b)), l(GetGValue(a), GetGValue(b)), l(GetBValue(a), GetBValue(b)));
}

} // namespace Theme
