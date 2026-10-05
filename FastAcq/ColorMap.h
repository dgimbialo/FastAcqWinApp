#pragma once
//
// ColorMap.h -- 256-entry palettes for waterfall / heat-map rendering.
//

#include "pch.h"

enum class Palette {
    Jet = 0,
    Viridis,
    Inferno,
    Turbo,
    Plasma,
    Gray,
    Count
};

class ColorMap {
public:
    static const COLORREF* Table(Palette p);
    static LPCTSTR Name(Palette p);
    static COLORREF FromNorm(Palette p, float v);   // v in [0,1]

    // Backward compatibility.
    static const COLORREF* Jet() { return Table(Palette::Jet); }
};
