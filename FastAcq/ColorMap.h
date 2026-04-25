#pragma once
//
// ColorMap.h
// Precomputed "Jet" palette for waterfall/spectrum rendering.
//

#include "pch.h"

class ColorMap {
public:
    // Returns a precomputed Jet palette of 256 colors.
    // Index 0 = deep blue, 255 = dark red.
    static const COLORREF* Jet();

    // Map a normalized value in [0, 1] to a Jet palette entry.
    static COLORREF JetFromNorm(float v);
};
