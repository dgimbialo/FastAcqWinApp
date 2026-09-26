#pragma once
//
// ChirpPreviewCtrl -- live graphical preview of the chirp the MCU will
// generate from the values typed on the Settings tab: triangle waveform with
// rise / fall / amplitude / period annotations, burst repetition, ADC capture
// window (samples, DMA chunks) and the interval / trigger gap to the next
// cycle. Geometry is computed with the same integer-tick rules as the
// firmware (chirp_dac.c), so the numbers shown are what the device applies.
//

#include "pch.h"

struct ChirpParams {
    uint32_t freqHz{458};      // used when riseUs == 0 || fallUs == 0
    uint32_t riseUs{0};
    uint32_t fallUs{0};
    uint32_t amplitude{4095};  // DAC counts
    uint32_t burst{1};         // chirps per capture
    uint32_t intervalMs{30};   // CONTINUOUS pause
    uint32_t samplesOvr{0};    // 0 = auto
    int      mode{1};          // 0 IDLE, 1 CONTINUOUS, 2 SINGLE
};

// Result of the firmware geometry rules (mirror of chirp_geometry_t).
struct ChirpGeometry {
    bool     valid{false};
    bool     rampMode{false};
    uint32_t tableLen{0}, riseLen{0}, ticksPerSample{0}, periodTicks{0};
    uint32_t samplesPerChirp{0}, riseSamples{0}, samplesPerBurst{0};
    uint32_t captureTarget{0}, chunks{0};
    double   riseUs{0}, fallUs{0}, periodUs{0}, freqHz{0};
};

ChirpGeometry ComputeChirpGeometry(const ChirpParams& p);

class ChirpPreviewCtrl : public CWnd {
public:
    BOOL CreateCtrl(CWnd* parent, UINT id);
    void SetParams(const ChirpParams& p);

protected:
    afx_msg int  OnCreate(LPCREATESTRUCT);
    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC*) { return TRUE; }
    afx_msg void OnSize(UINT, int, int);
    DECLARE_MESSAGE_MAP()

private:
    void Render(CDC& dc, const CRect& rc);

    ChirpParams   m_p;
    ChirpGeometry m_g;
    CFont m_font, m_boldFont, m_smallFont;
};
