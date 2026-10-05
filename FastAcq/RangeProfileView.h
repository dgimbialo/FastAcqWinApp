#pragma once
//
// RangeProfileView -- UP/DOWN beat spectra in dBFS over frequency (bottom
// axis) and range (top axis), CFAR threshold, noise floor, numbered peaks
// with range/velocity labels, cursor readout, A/B markers, zoom and pan.
//

#include "pch.h"
#include "AppSettings.h"
#include "Dsp/RadarDsp.h"
#include "PlotWnd.h"

class RangeProfileView : public PlotWnd {
public:
    RangeProfileView() = default;

    void SetResult(std::shared_ptr<const dsp::FrameResult> r);
    void SetDisplay(const DisplaySettings& d);
    void SetMaxRangeM(double m) { m_maxRangeM = m; Invalidate(FALSE); }

    // Shared x-range (Hz) with peer views. (0,0) = full span.
    void SetXRange(double f0, double f1);
    void GetXRange(double& f0, double& f1) const { f0 = m_x0; f1 = m_x1; }
    // Widest span the wheel may zoom out to: the undecimated fs/2 (the DSP
    // lowers the decimation when the view asks for more than it computed).
    double MaxSpanHz() const;
    bool HasZoom() const { return m_haveZoom; }
    void ResetZoom();
    void ClearMarkers();
    void AutoscaleDb();
    void SnapMarkersToPeaks();

    double FullSpanHz() const;

protected:
    void Render(CDC& dc, const CRect& rc) override;
    void OnPlotMouseMove(CPoint pt, UINT flags) override;
    void OnPlotLButtonDown(CPoint pt, UINT flags) override;
    void OnPlotLButtonUp(CPoint pt, UINT flags) override;
    void OnPlotLButtonDblClk(CPoint pt, UINT flags) override;
    void OnPlotRButtonUp(CPoint pt, UINT flags) override;
    BOOL OnPlotMouseWheel(UINT flags, short zDelta, CPoint pt) override;
    void OnPlotKeyDown(UINT nChar, UINT flags) override;

private:
    struct Layout { CRect plot, axisBottom, axisTop, axisLeft, title; };
    Layout     ComputeLayout(const CRect& rc) const;
    LinearAxis XAxis(const CRect& plot) const;
    LinearAxis YAxis(const CRect& plot) const;
    void       EffectiveX(double& f0, double& f1) const;
    void       EffectiveDb(double& top, double& bottom) const;
    void       NotifyXRange();

    void DrawTrace(CDC& dc, const CRect& plot, const LinearAxis& xa, const LinearAxis& ya,
                   const std::vector<float>& db, double freqRes, COLORREF color, int penStyle, int width);
    void DrawPeaks(CDC& dc, const CRect& plot, const LinearAxis& xa, const LinearAxis& ya);
    void DrawMarker(CDC& dc, const Layout& L, const LinearAxis& xa, double fHz, COLORREF color, LPCTSTR name, int& readoutX);
    void DrawCursor(CDC& dc, const Layout& L, const LinearAxis& xa, const LinearAxis& ya);
    double ValueAt(const dsp::RampSpectrum& s, double fHz, const std::vector<float>* vec = nullptr) const;
    double SnapToPeak(double fHz, const LinearAxis& xa) const;

    std::shared_ptr<const dsp::FrameResult> m_res;
    DisplaySettings m_disp;
    double  m_maxRangeM{100.0};
    double  m_x0{0.0}, m_x1{0.0};
    bool    m_haveZoom{false};
    bool    m_autoDbActive{false};
    double  m_autoTop{0.0}, m_autoBottom{-120.0};

    struct Marker { bool valid{false}; double fHz{0.0}; };
    Marker  m_mA, m_mB;

    bool    m_dragging{false};
    bool    m_dragMoved{false};
    CPoint  m_dragStart;
    double  m_dragX0{0.0}, m_dragX1{0.0};
};
