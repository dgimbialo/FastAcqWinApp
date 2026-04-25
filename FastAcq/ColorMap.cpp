#include "pch.h"
#include "ColorMap.h"

#include <algorithm>

namespace {

// Classic Matlab "jet" colormap.
// Piecewise-linear ramps for R, G, B in [0, 1].
static inline float JetR(float v) {
    if (v < 0.375f) return 0.0f;
    if (v < 0.625f) return (v - 0.375f) / 0.25f;
    if (v < 0.875f) return 1.0f;
    return (std::max)(0.0f, 1.0f - (v - 0.875f) / 0.125f * 0.5f);
}
static inline float JetG(float v) {
    if (v < 0.125f) return 0.0f;
    if (v < 0.375f) return (v - 0.125f) / 0.25f;
    if (v < 0.625f) return 1.0f;
    if (v < 0.875f) return 1.0f - (v - 0.625f) / 0.25f;
    return 0.0f;
}
static inline float JetB(float v) {
    if (v < 0.125f) return 0.5f + v / 0.125f * 0.5f;
    if (v < 0.375f) return 1.0f;
    if (v < 0.625f) return 1.0f - (v - 0.375f) / 0.25f;
    return 0.0f;
}

struct JetTable {
    COLORREF entries[256];
    JetTable() {
        for (int i = 0; i < 256; ++i) {
            float v = static_cast<float>(i) / 255.0f;
            BYTE r = static_cast<BYTE>(std::clamp(JetR(v) * 255.0f, 0.0f, 255.0f));
            BYTE g = static_cast<BYTE>(std::clamp(JetG(v) * 255.0f, 0.0f, 255.0f));
            BYTE b = static_cast<BYTE>(std::clamp(JetB(v) * 255.0f, 0.0f, 255.0f));
            entries[i] = RGB(r, g, b);
        }
    }
};

static const JetTable g_jet;

} // namespace

const COLORREF* ColorMap::Jet()
{
    return g_jet.entries;
}

COLORREF ColorMap::JetFromNorm(float v)
{
    if (!(v > 0.0f)) return g_jet.entries[0];
    if (v >= 1.0f)   return g_jet.entries[255];
    int idx = static_cast<int>(v * 255.0f + 0.5f);
    if (idx < 0)   idx = 0;
    if (idx > 255) idx = 255;
    return g_jet.entries[idx];
}
