#pragma once
//
// AppSettings.h -- all user-adjustable state, persisted in FastAcq.ini next
// to the executable (plain Win32 profile API, no dependencies).
//

#include "pch.h"
#include "ColorMap.h"
#include "Dsp/RadarDsp.h"

struct AcqSettings {
    int  mode{1};            // MODE_IDLE / MODE_CONTINUOUS / MODE_SINGLE
    int  chirpFreqHz{458};
    int  samples{0};         // 0 = auto
    int  intervalMs{30};
    int  amplitude{4095};
    int  burst{1};
    bool sendRaw{true};
    bool sendFft{true};
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

    uint32_t sampleRateCalHz{60058600};   // calibrated ADC rate (fallback when header has none)
    bool     chirpsFromBurst{true};       // dsp.chirpsInFrame follows the MCU burst setting
    bool     verboseLog{false};           // per-frame RX lines in the communication log
    bool     autoConnect{true};           // connect to the first FastAcq port at start-up
    bool     logAutoScroll{true};
    CString  lastPort;
    CString  lastDir;
    int      activeTab{0};
    CRect    windowRect{0, 0, 0, 0};
    bool     windowMax{false};
    float    splitRadar1{0.42f};
    float    splitRadar2{0.78f};
    float    splitScope{0.5f};

    static CString DefaultPath();
    bool Load(const CString& path);
    bool Save(const CString& path) const;
};
