#pragma once
//
// TargetListCtrl -- detected targets (range, velocity, beat frequencies,
// amplitude, SNR) for the current frame.
//

#include "pch.h"
#include "Dsp/RadarDsp.h"

class TargetListCtrl : public CListCtrl {
public:
    void Init();
    void SetTargets(const std::vector<dsp::Target>& t, bool haveRange, bool triangle);
    void ApplyTheme();
    CString ToCsv() const;   // header + rows (current targets)
    static CString CsvHeader();
    static CString CsvRow(uint32_t frameId, uint32_t tsMs, const dsp::Target& t);

private:
    std::vector<dsp::Target> m_targets;
};
