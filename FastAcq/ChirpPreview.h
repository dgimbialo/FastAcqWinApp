#pragma once
//
// ChirpPreviewCtrl -- live graphical preview of the chirp the MCU will
// generate from the values typed on the Settings tab: triangle waveform with
// rise / fall / amplitude / period annotations, burst repetition, ADC capture
// window (samples, DMA chunks) and the interval / trigger gap to the next
// cycle. Geometry comes from core::ComputeChirpGeometry (the same integer
// tick rules as the firmware), so the numbers shown are what the device
// applies. Follows the application theme and the monitor DPI.
//

#include "pch.h"
#include "Core/ChirpGeometry.h"

class ChirpPreviewCtrl : public CWnd {
public:
    BOOL CreateCtrl(CWnd* parent, UINT id);
    void SetParams(const core::ChirpParams& p);
    const core::ChirpGeometry& Geometry() const { return m_g; }
    void ApplyTheme();

protected:
    afx_msg int  OnCreate(LPCREATESTRUCT);
    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC*) { return TRUE; }
    afx_msg void OnSize(UINT, int, int);
    afx_msg LRESULT OnDpiChangedAfterParent(WPARAM, LPARAM);
    DECLARE_MESSAGE_MAP()

private:
    void MakeFonts();
    void Render(CDC& dc, const CRect& rc);

    core::ChirpParams   m_p;
    core::ChirpGeometry m_g;
    CFont m_font, m_boldFont, m_smallFont;
};
