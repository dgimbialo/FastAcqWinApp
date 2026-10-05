#pragma once
//
// ChirpGeometry.h -- host mirror of the firmware chirp generator rules
// (chirp_dac.c / dcmi_adc.c): how a requested frequency or rise/fall pair is
// quantized into DAC table length, TIM7 ticks per table point, ADC samples
// per chirp and the DMA capture window. Portable: no Windows/MFC
// dependencies (shared by the Settings tab preview and the unit tests).
//

#include <cstdint>

namespace core {

// Firmware constants (chirp_dac.h / dcmi_adc.h).
constexpr uint32_t kChirpTim7ClkHz     = 240000000u;
constexpr uint32_t kChirpTriangleMax   = 8192u;      // DAC table points
constexpr uint32_t kChirpMinTicksPerPt = 64u;
constexpr uint32_t kChirpFreqMinHz     = 100u;
constexpr uint32_t kChirpFreqMaxHz     = 24000u;
constexpr uint32_t kChirpAdcHz         = 60000000u;  // nominal ADC rate (TIM7 / 4)
constexpr uint32_t kChirpDmaChunk      = 16384u;     // samples per DMA chunk
constexpr uint32_t kChirpCaptureMax    = 650000u;    // samples per capture

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
    bool     rampMode{false};  // rise/fall given explicitly (CMD_SET_RAMP)
    uint32_t tableLen{0}, riseLen{0}, ticksPerSample{0}, periodTicks{0};
    uint32_t samplesPerChirp{0}, riseSamples{0}, samplesPerBurst{0};
    uint32_t captureTarget{0}, chunks{0};
    double   riseUs{0}, fallUs{0}, periodUs{0}, freqHz{0};
};

ChirpGeometry ComputeChirpGeometry(const ChirpParams& p);

} // namespace core
