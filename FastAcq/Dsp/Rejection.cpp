#include "Rejection.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace dsp {

void RejectPeaks(std::vector<Peak>& peaks, double freqResHz, const RejectionParams& p, RejectionStats& stats)
{
    stats = RejectionStats{};
    if (peaks.empty()) return;
    std::vector<bool> drop(peaks.size(), false);

    if (p.minSnrDb > 0.0f) {
        for (size_t i = 0; i < peaks.size(); ++i)
            if (peaks[i].snrDb < p.minSnrDb) { drop[i] = true; ++stats.lowSnr; }
    }
    if (!p.spurs.empty()) {
        for (size_t i = 0; i < peaks.size(); ++i) {
            if (drop[i]) continue;
            for (const auto& b : p.spurs) {
                if (std::fabs(peaks[i].freqHz - b.centerHz) <= b.halfWidthHz) { drop[i] = true; ++stats.spurs; break; }
            }
        }
    }
    if (p.harmonics && freqResHz > 0.0) {
        const int kMax = (std::max)(2, p.maxHarmonic);
        for (size_t i = 0; i < peaks.size(); ++i) {           // i = candidate fundamental (strong)
            if (drop[i] || peaks[i].freqHz <= 0.0) continue;
            for (size_t j = 0; j < peaks.size(); ++j) {       // j = candidate harmonic (weaker)
                if (j == i || drop[j]) continue;
                if (peaks[j].ampDb > peaks[i].ampDb - p.harmonicMinDropDb) continue;
                const double ratio = peaks[j].freqHz / peaks[i].freqHz;
                const int k = static_cast<int>(std::lround(ratio));
                if (k < 2 || k > kMax) continue;
                const double tol = p.harmonicTolBins * freqResHz * k;
                if (std::fabs(peaks[j].freqHz - k * peaks[i].freqHz) <= tol) { drop[j] = true; ++stats.harmonics; }
            }
        }
    }
    std::vector<Peak> kept;
    kept.reserve(peaks.size());
    for (size_t i = 0; i < peaks.size(); ++i) if (!drop[i]) kept.push_back(peaks[i]);
    peaks.swap(kept);
}

std::vector<SpurBand> ParseSpurList(const std::string& text, double defaultHalfWidthHz)
{
    std::vector<SpurBand> out;
    std::string tok;
    auto flush = [&]() {
        if (tok.empty()) return;
        SpurBand b;
        const size_t sep = tok.find(':');
        char* e = nullptr;
        b.centerHz = std::strtod(tok.c_str(), &e);
        if (sep != std::string::npos) b.halfWidthHz = std::strtod(tok.c_str() + sep + 1, nullptr);
        if (!(b.halfWidthHz > 0.0)) b.halfWidthHz = defaultHalfWidthHz;
        if (b.centerHz > 0.0 && e != tok.c_str()) out.push_back(b);
        tok.clear();
    };
    for (char ch : text) {
        if (ch == ',' || ch == ';' || ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') flush();
        else tok.push_back(ch);
    }
    flush();
    std::sort(out.begin(), out.end(), [](const SpurBand& a, const SpurBand& b) { return a.centerHz < b.centerHz; });
    return out;
}

std::string FormatSpurList(const std::vector<SpurBand>& spurs)
{
    std::string s;
    char buf[64];
    for (size_t i = 0; i < spurs.size(); ++i) {
        std::snprintf(buf, sizeof(buf), "%s%.0f:%.0f", i ? ", " : "", spurs[i].centerHz, spurs[i].halfWidthHz);
        s += buf;
    }
    return s;
}

} // namespace dsp
