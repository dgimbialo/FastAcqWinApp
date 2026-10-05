#pragma once
//
// WaterfallView -- range/frequency-time intensity plot (newest row at top)
// in dB with a selectable palette, time axis, colour bar and cursor readout.
// The x-range (zoom) is shared with RangeProfileView.
//

#include "pch.h"
#include "AppSettings.h"
#include "Dsp/RadarDsp.h"
#include "PlotWnd.h"

class WaterfallView : public PlotWnd {
public:
    WaterfallView() = default;
    ~WaterfallView() override;

    // Adds the UP spectrum of a result as the newest row.
    void PushResult(const dsp::FrameResult& r);
    void SetDisplay(const DisplaySettings& d);
    void SetMaxRangeM(double m) { m_maxRangeM = m; m_dirty = true; Invalidate(FALSE); }
    void SetXRange(double f0, double f1);
    void GetXRange(double& f0, double& f1) const { f0 = m_x0; f1 = m_x1; }
    void ResetZoom();
    void Clear();
    void AutoscaleDb();
    size_t Rows() const { return m_rows.size(); }

protected:
    void Render(CDC& dc, const CRect& rc) override;
    void OnPlotMouseMove(CPoint pt, UINT flags) override;
    void OnPlotLButtonDblClk(CPoint pt, UINT flags) override;
    void OnPlotRButtonUp(CPoint pt, UINT flags) override;
    BOOL OnPlotMouseWheel(UINT flags, short zDelta, CPoint pt) override;
    void OnPlotSize(int cx, int cy) override;

private:
    struct Row {
        std::vector<float> db;   // pooled columns over [0, fsEff/2]
        double   hzPerCol{0.0};
        uint32_t tsMs{0};
        uint32_t frameId{0};
    };
    struct Layout { CRect plot, axisBottom, axisLeft, colorbar, cbLabels, title; };

    Layout  ComputeLayout(const CRect& rc) const;
    void    EffectiveX(double& f0, double& f1) const;
    void    EffectiveDb(double& top, double& bottom) const;
    double  FullSpanHz() const;
    void    NotifyXRange();
    void    EnsureDib(int w, int h);
    void    FreeDib();
    void    RenderRow(const Row& r, DWORD* dst, int w, double f0, double f1, double top, double bottom) const;
    void    RebuildDib(const CRect& plot);
    float   ValueAt(const Row& r, double fHz) const;

    std::deque<Row> m_rows;              // front = newest
    size_t   m_maxRows{512};
    static constexpr int kMaxCols = 4096;
    double   m_hzPerCol{0.0};
    double   m_fsEffHz{0.0};
    double   m_rangePerHz{0.0};
    double   m_rangeOffsetM{0.0};
    double   m_maxRangeM{100.0};
    DisplaySettings m_disp;
    bool     m_autoDbActive{false};
    double   m_autoTop{0.0}, m_autoBottom{-120.0};
    double   m_x0{0.0}, m_x1{0.0};
    bool     m_haveZoom{false};

    HBITMAP  m_hDib{nullptr};
    DWORD*   m_px{nullptr};
    int      m_dibW{0}, m_dibH{0};
    bool     m_dirty{true};
    double   m_dibF0{0.0}, m_dibF1{0.0}, m_dibTop{0.0}, m_dibBottom{0.0};
    int      m_dibPalette{-1};
};
