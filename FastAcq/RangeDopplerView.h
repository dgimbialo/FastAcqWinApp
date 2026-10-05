#pragma once
//
// RangeDopplerView -- heat map of the range-Doppler matrix computed from a
// multi-chirp (burst) frame: range on x, velocity on y, zero velocity in the
// middle, positive = approaching.
//

#include "pch.h"
#include "AppSettings.h"
#include "Dsp/RadarDsp.h"
#include "PlotWnd.h"

class RangeDopplerView : public PlotWnd {
public:
    RangeDopplerView() = default;
    ~RangeDopplerView() override;

    void SetResult(std::shared_ptr<const dsp::FrameResult> r);
    void SetDisplay(const DisplaySettings& d);
    bool HasData() const { return m_res && m_res->rd.nRange > 0 && m_res->rd.nDoppler > 0; }

protected:
    void Render(CDC& dc, const CRect& rc) override;
    void OnPlotRButtonUp(CPoint pt, UINT flags) override;

private:
    struct Layout { CRect plot, axisBottom, axisLeft, colorbar, cbLabels, title; };
    Layout ComputeLayout(const CRect& rc) const;
    void   BuildDib();
    void   FreeDib();

    std::shared_ptr<const dsp::FrameResult> m_res;
    DisplaySettings m_disp;
    HBITMAP m_hDib{nullptr};
    DWORD*  m_px{nullptr};
    int     m_dibW{0}, m_dibH{0};
    double  m_top{0.0}, m_bottom{-70.0};
    bool    m_dirty{true};
};
