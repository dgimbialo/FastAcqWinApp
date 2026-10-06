#pragma once
//
// RadarPlanner.h -- turns operating requirements (range band, beat-frequency
// band, velocity band, chirp Vtune window) into MCU / processing settings:
// chirp repetition rate, DAC window, burst length, decimation, display range.
// Portable (no Windows / MFC): shared by the Settings tab calculator and the
// unit tests.
//
// Geometry (triangle FMCW, symmetric UP/DOWN ramps of duration T):
//   slope        S     = B / T                       [Hz/s]
//   beat tone    f_b   = 2 R S / c = R * (2 B / (c T))
//   range step   dR    = c / (2 B)
//   pair step    dv    = c / (4 f0 T)                UP/DOWN pairing
//   Doppler      f_d   = 2 v f0 / c  must stay below f_b(R_min) for pairing
//   burst N      dv_N  = c / (2 f0 N T_period),  v_unamb = c / (4 f0 T_period)
//

#include "ChirpGeometry.h"
#include "VcoCurve.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace core {

struct PlanInput {
    // Requirements.
    double rMinM{0.5};          // nearest target of interest
    double rMaxM{5.0};          // farthest target of interest
    double fbMinHz{2000.0};     // the beat tone of the nearest target should be at least this (above DC / 1/f)
    double fbMaxHz{500000.0};   // the beat tone of the farthest target must not exceed this
    double vMinMps{0.2};        // finest velocity step wanted
    double vMaxMps{5.0};        // highest |v| to handle
    double vLowV{0.0};          // chirp tuning-voltage window (base .. top)
    double vHighV{10.0};
    // Hardware context.
    double   vtuneAtDac0V{0.0};
    double   vtuneAtDacFullV{10.0};
    VcoCurve curve;
    double   fsHz{60e6};
    double   guardPct{5.0};
    uint32_t intervalMs{5};
};

enum class PlanNote {
    VtuneClamped = 0,        // the window was limited to what the DAC can produce
    VtuneOutsideCurve,       // the window leaves the tabulated curve (extrapolated)
    BeatBandConflict,        // fbMin / rMin > fbMax / rMax: both cannot hold at once
    ChirpFreqClampedLow,     // the ramp would be longer than the MCU allows (< 100 Hz)
    ChirpFreqClampedHigh,    // the ramp would be shorter than the MCU allows (> 24 kHz)
    DopplerExceedsBeatAtRmin,// at vMax the Doppler shift exceeds the beat of the nearest target
    BurstLimitedByCapture,   // the burst wanted for the velocity step does not fit one capture
    VelocityStepNotReached,  // even with the burst the velocity step stays coarser than asked
    SweepNonlinear,          // VCO sweep nonlinearity above 10 %
    Count
};

struct PlanResult {
    bool valid{false};
    std::vector<PlanNote> notes;

    // Chirp DAC window and the resulting sweep.
    int    dacOffset{0}, dacAmplitude{0};
    double vLowV{0.0}, vHighV{0.0};
    double fStartHz{0.0}, fStopHz{0.0}, f0Hz{0.0}, bandwidthHz{0.0}, nonlinearityPct{0.0};

    // Timing.
    double   slopeHzPerS{0.0};
    double   rampSec{0.0};
    uint32_t chirpFreqHz{0};
    uint32_t burst{1};
    ChirpGeometry geometry;     // firmware quantization for (chirpFreqHz, burst)
    double   frameRateHz{0.0};

    // Beat / range.
    double fbAtRMinHz{0.0}, fbAtRMaxHz{0.0};
    double beatPerMeterHz{0.0};
    double rangeResM{0.0}, rangeMaxM{0.0};
    int    decimation{1};
    double fsEffHz{0.0};
    size_t samplesPerRamp{0}, samplesUsed{0};

    // Velocity.
    double velStepPairMps{0.0};    // c / (4 f0 T)
    double velMaxPairMps{0.0};     // c f_b(rMin) / (2 f0)
    double velMaxDopplerMps{0.0};  // c / (4 f0 T_period)
    double velStepDopplerMps{0.0}; // c / (2 f0 N T_period)

    // Suggested processing settings.
    double pairGateMps{0.0};
    double rangeOfInterestM{0.0};

    bool Has(PlanNote n) const;
};

PlanResult PlanRadar(const PlanInput& in);

} // namespace core
