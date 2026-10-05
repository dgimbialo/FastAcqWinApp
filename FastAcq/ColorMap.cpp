#include "pch.h"
#include "ColorMap.h"

namespace {

struct Anchor { float pos; unsigned rgb; };

// Anchor colors sampled from the matplotlib / Google palettes; linearly
// interpolated to 256 entries at start-up.
const Anchor kViridis[] = {
    {0.0f,0x440154},{0.1f,0x482475},{0.2f,0x414487},{0.3f,0x355f8d},{0.4f,0x2a788e},{0.5f,0x21918c},
    {0.6f,0x22a884},{0.7f,0x44bf70},{0.8f,0x7ad151},{0.9f,0xbddf26},{1.0f,0xfde725} };
const Anchor kInferno[] = {
    {0.0f,0x000004},{0.1f,0x160b39},{0.2f,0x420a68},{0.3f,0x6a176e},{0.4f,0x932667},{0.5f,0xbc3754},
    {0.6f,0xdd513a},{0.7f,0xf37819},{0.8f,0xfca50a},{0.9f,0xf6d746},{1.0f,0xfcffa4} };
const Anchor kTurbo[] = {
    {0.0f,0x30123b},{0.1f,0x4458cb},{0.2f,0x3e9bfe},{0.3f,0x18d6cb},{0.4f,0x46f884},{0.5f,0xa2fc3c},
    {0.6f,0xe1dd37},{0.7f,0xfea331},{0.8f,0xef5a11},{0.9f,0xc42503},{1.0f,0x7a0403} };
const Anchor kPlasma[] = {
    {0.0f,0x0d0887},{0.1f,0x41049d},{0.2f,0x6a00a8},{0.3f,0x8f0da4},{0.4f,0xb12a90},{0.5f,0xcc4778},
    {0.6f,0xe16462},{0.7f,0xf2844b},{0.8f,0xfca636},{0.9f,0xfcce25},{1.0f,0xf0f921} };
const Anchor kGray[] = { {0.0f,0x000000},{1.0f,0xffffff} };

COLORREF FromRgbHex(unsigned rgb) { return RGB((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF); }

void FillFromAnchors(COLORREF* out, const Anchor* a, int na)
{
    for (int i = 0; i < 256; ++i) {
        float v = static_cast<float>(i) / 255.0f;
        int j = 0;
        while (j + 1 < na - 1 && v > a[j + 1].pos) ++j;
        const Anchor& p = a[j];
        const Anchor& q = a[j + 1 < na ? j + 1 : j];
        float t = (q.pos > p.pos) ? (v - p.pos) / (q.pos - p.pos) : 0.0f;
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;
        auto ch = [&](int shift) {
            float x = static_cast<float>((p.rgb >> shift) & 0xFF);
            float y = static_cast<float>((q.rgb >> shift) & 0xFF);
            return static_cast<BYTE>(x + (y - x) * t + 0.5f);
        };
        out[i] = RGB(ch(16), ch(8), ch(0));
    }
}

// Classic Matlab "jet".
float JetR(float v) {
    if (v < 0.375f) return 0.0f;
    if (v < 0.625f) return (v - 0.375f) / 0.25f;
    if (v < 0.875f) return 1.0f;
    return (std::max)(0.0f, 1.0f - (v - 0.875f) / 0.125f * 0.5f);
}
float JetG(float v) {
    if (v < 0.125f) return 0.0f;
    if (v < 0.375f) return (v - 0.125f) / 0.25f;
    if (v < 0.625f) return 1.0f;
    if (v < 0.875f) return 1.0f - (v - 0.625f) / 0.25f;
    return 0.0f;
}
float JetB(float v) {
    if (v < 0.125f) return 0.5f + v / 0.125f * 0.5f;
    if (v < 0.375f) return 1.0f;
    if (v < 0.625f) return 1.0f - (v - 0.375f) / 0.25f;
    return 0.0f;
}

struct Tables {
    COLORREF t[static_cast<int>(Palette::Count)][256];
    Tables() {
        for (int i = 0; i < 256; ++i) {
            float v = static_cast<float>(i) / 255.0f;
            auto c = [](float x) { return static_cast<BYTE>(std::clamp(x * 255.0f, 0.0f, 255.0f)); };
            t[static_cast<int>(Palette::Jet)][i] = RGB(c(JetR(v)), c(JetG(v)), c(JetB(v)));
        }
        FillFromAnchors(t[static_cast<int>(Palette::Viridis)], kViridis, 11);
        FillFromAnchors(t[static_cast<int>(Palette::Inferno)], kInferno, 11);
        FillFromAnchors(t[static_cast<int>(Palette::Turbo)],   kTurbo,   11);
        FillFromAnchors(t[static_cast<int>(Palette::Plasma)],  kPlasma,  11);
        FillFromAnchors(t[static_cast<int>(Palette::Gray)],    kGray,     2);
        (void)FromRgbHex;
    }
};

const Tables& Tab() { static const Tables tables; return tables; }

} // namespace

const COLORREF* ColorMap::Table(Palette p)
{
    int i = static_cast<int>(p);
    if (i < 0 || i >= static_cast<int>(Palette::Count)) i = 0;
    return Tab().t[i];
}

LPCTSTR ColorMap::Name(Palette p)
{
    switch (p) {
    case Palette::Jet:     return _T("Jet");
    case Palette::Viridis: return _T("Viridis");
    case Palette::Inferno: return _T("Inferno");
    case Palette::Turbo:   return _T("Turbo");
    case Palette::Plasma:  return _T("Plasma");
    case Palette::Gray:    return _T("Gray");
    default:               return _T("?");
    }
}

COLORREF ColorMap::FromNorm(Palette p, float v)
{
    if (!(v > 0.0f)) v = 0.0f;
    if (v > 1.0f) v = 1.0f;
    int idx = static_cast<int>(v * 255.0f + 0.5f);
    if (idx < 0) idx = 0;
    if (idx > 255) idx = 255;
    return Table(p)[idx];
}
