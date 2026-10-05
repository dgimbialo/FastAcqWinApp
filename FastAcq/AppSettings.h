#pragma once
//
// AppSettings.h -- all user-adjustable state, persisted in FastAcq.ini next
// to the executable (plain Win32 profile API, no dependencies).
//

#include "pch.h"
#include "ColorMap.h"
#include "Core/VcoCurve.h"
#include "Dsp/RadarDsp.h"

#include <string>

struct AcqSettings {
    int  mode{1};            // MODE_IDLE / MODE_CONTINUOUS / MODE_SINGLE
    int  chirpFreqHz{458};
    int  samples{0};         // 0 = auto
    int  intervalMs{30};
    int  amplitude{4095};
    int  offset{0};          // chirp DAC offset (base level): triangle runs offset..offset+amplitude
    int  burst{1};
    int  riseUs{0};          // CMD_SET_RAMP rise, us (0/0 = symmetric from chirpFreqHz)
    int  fallUs{0};
    bool sendRaw{true};
    bool sendFft{true};
};

// Radar VCO: the chirp DAC (0..4095) drives the tuning voltage linearly
// between vtuneAtDac0V and vtuneAtDacFullV; the tuning curve turns that into
// the swept band (f_start, f_stop), i.e. f0 and B of the radar geometry.
struct VcoSettings {
    bool        useCurve{true};        // f0 / B follow the curve (else typed by hand)
    double      vtuneAtDac0V{0.0};     // tuning-voltage offset (bias) at DAC code 0
    double      vtuneAtDacFullV{10.0}; // tuning voltage at DAC code 4095
    std::string curveText;             // "V:GHz,V:GHz,..." (empty = HMC431 typical)

    core::VcoCurve Curve() const;
    // Sweep for a chirp running from DAC code `offset` to `offset + amplitude`.
    core::VcoSweep SweepFor(int offset, int amplitude) const;
    double VtuneOfDac(int code) const;
};

struct DisplaySettings {
    float dbTop{0.0f};
    float dbBottom{-120.0f};
    bool  autoDb{false};
    int   palette{static_cast<int>(Palette::Viridis)};
    int   waterfallRows{512};
    bool  darkTheme{false};
    int   adcBits{12};
    float vRef{3.3f};
    bool  showVolts{true};
    bool  showUp{true};
    bool  showDown{true};
    bool  showThreshold{true};
    bool  showNoise{false};
    bool  showPeaks{true};
    bool  showRangeDoppler{true};
    bool  dots{false};
};

struct AppSettings {
    AcqSettings      acq;
    dsp::DspSettings dsp;
    dsp::RadarParams radar;
    DisplaySettings  display;
    VcoSettings      vco;

    uint32_t sampleRateCalHz{60058600};   // calibrated ADC rate (fallback when header has none)
    double   fsPpm{0.0};                  // ADC clock correction applied to the header's nominal rate
    bool     chirpsFromBurst{true};       // dsp.chirpsInFrame follows the MCU burst setting
    bool     verboseLog{false};           // per-frame RX lines in the communication log
    bool     autoConnect{true};           // connect to the first FastAcq port at start-up
    int      language{0};                 // Lang::Id (0 English, 1 Ukrainian), applied at start-up
    bool     logAutoScroll{true};
    CString  lastPort;
    CString  lastDir;
    int      activeTab{0};
    CRect    windowRect{0, 0, 0, 0};
    bool     windowMax{false};
    float    splitRadar1{0.42f};
    float    splitRadar2{0.78f};
    float    splitScope{0.5f};

    // Multiplier for the header's nominal sample rate: 1 + ppm * 1e-6.
    double FsFactor() const { return 1.0 + fsPpm * 1e-6; }

    // When vco.useCurve: radar.f0Hz / bandwidthHz := band swept by the chirp
    // amplitude through the VCO curve. Returns the sweep (valid=false if the
    // curve is unusable; the typed f0 / B are then left as they are).
    core::VcoSweep ApplyVcoToRadar();

    static CString DefaultPath();
    bool Load(const CString& path);
    bool Save(const CString& path) const;
};
