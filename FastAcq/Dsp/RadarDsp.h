#pragma once
//
// RadarDsp.h -- FMCW beat-signal processing pipeline (PC side).
//
//   raw samples -> segmentation (chirps, UP/DOWN ramps, guard) -> decimation
//   -> detrend -> window -> zero-pad -> real FFT -> dBFS -> trace averaging
//   -> noise/CFAR -> peaks (sub-bin) -> range/velocity (UP/DOWN pairing)
//   -> range-Doppler map (burst) -> phase tracking -> target tracks.
//
// Portable: no Windows/MFC dependencies. One instance per processing thread.
//

#include "../ChirpStore.h"
#include "Cfar.h"
#include "Decimator.h"
#include "FftPlan.h"
#include "PeakFinder.h"
#include "ToneEstimator.h"
#include "Window.h"

#include <complex>
#include <cstdint>
#include <map>
#include <memory>
#include <vector>

namespace dsp {

constexpr double kSpeedOfLight = 299792458.0;

enum class RampShape {
    Triangle = 0,   // each chirp = UP ramp followed by DOWN ramp
    Sawtooth,       // each chirp = one UP ramp
    Single,         // whole frame is one ramp (no segmentation)
    Count
};
const char* RampShapeName(RampShape s);

enum class TraceMode {
    ClearWrite = 0,
    Average,        // exponential average of power
    MaxHold,
    MinHold,
    Count
};
const char* TraceModeName(TraceMode m);

struct RadarParams {
    double f0Hz{5.5e9};           // carrier = centre of the sweep (device: 5..6 GHz)
    double bandwidthHz{1.0e9};    // swept bandwidth B (0 = unknown -> no range axis)
    double rampSec{0.0};          // ramp duration; 0 = derive from chirp frequency and shape
    double rangeOffsetM{0.0};     // zero-range calibration (cables, antenna delay)
    double pairMaxVelocityMps{30.0}; // UP/DOWN pairing gate

    double LambdaM() const { return f0Hz > 0.0 ? kSpeedOfLight / f0Hz : 0.0; }
};

struct DspSettings {
    int         chirpsInFrame{1};          // burst count (>= 1)
    RampShape   shape{RampShape::Triangle};
    float       guardPct{5.0f};            // % of each ramp dropped at both ends
    bool        detrend{true};             // remove linear trend (mean is always removed)
    int         decimation{0};             // 0 = auto (from maxRangeM), 1 = none, else factor
    double      maxRangeM{100.0};          // range of interest (auto decimation, map limits)
    WindowType  window{WindowType::Hann};
    float       kaiserBeta{9.0f};
    int         zeroPad{2};                // 1, 2, 4, 8
    float       rangeGainDbPerDecade{0.0f};// 0 = off; 20/40 = R^2 / R^4 compensation
    TraceMode   trace{TraceMode::ClearWrite};
    float       avgAlpha{0.3f};            // Average: weight of the newest frame
    DetectorParams detector{DetectorType::CaCfar, 12.0f, 2, 16, 1e-4f, 0.75f};
    PeakInterp  interp{PeakInterp::ParabolicDb};
    int         maxPeaks{10};
    double      minPeakDistBins{0.0};      // 0 = auto (window main lobe * zeroPad)
    int         skipDcBins{0};             // 0 = auto
    bool        mti{true};                 // Doppler: subtract mean over chirps
    int         phaseTrackBin{-1};         // -1 = strongest target
    bool        useMcuFft{false};          // use magnitudes computed on the MCU
    bool        trackTargets{true};
    bool        firmwareGeometry{true};    // segment chirps from the header's rise/fall (v2 exact samples)
    bool        toneEstimate{true};        // precise single-tone estimate per ramp (double FFT)
};

struct Segment {
    enum Kind { Up = 0, Down, Guard };
    size_t start{0};
    size_t length{0};
    Kind   kind{Up};
    int    chirp{0};
};

struct RampSpectrum {
    bool                              valid{false};
    std::vector<float>                db;        // dBFS per bin (after trace mode)
    std::vector<float>                thrDb;     // detector threshold (+inf = not evaluated)
    std::vector<float>                noiseDb;   // noise estimate per bin
    std::vector<std::complex<float>>  cplx;      // complex spectrum of the first chirp (scaled)
    double                            freqResHz{0.0};
    double                            fsEffHz{0.0};
    size_t                            nSamples{0};   // samples per ramp in the FFT
    size_t                            nFft{0};
    size_t                            firstBin{0};   // first evaluated bin (after DC skip)
    float                             noiseFloorDb{0.0f};
    std::vector<Peak>                 peaks;         // sorted by amplitude, descending
    ToneEstimate                      tone;          // precise estimate of the strongest tone (first chirp)
};

struct Target {
    int    id{-1};
    double fUpHz{0.0};
    double fDnHz{0.0};
    double rangeM{0.0};
    double velocityMps{0.0};   // > 0 = approaching
    float  ampDb{0.0f};
    float  snrDb{0.0f};
    bool   paired{false};
};

struct RangeDopplerMap {
    int    nRange{0};
    int    nDoppler{0};
    std::vector<float> db;       // nDoppler rows x nRange columns, zero velocity in the middle row
    double rangeBinM{0.0};       // metres per column (0 if bandwidth unknown)
    double freqResHz{0.0};       // Hz per column
    double velBinMps{0.0};       // m/s per row
    double velMaxMps{0.0};       // unambiguous +/- velocity
    int    peakRange{-1};
    int    peakDoppler{-1};
    float  peakDb{-300.0f};
    float  minDb{-300.0f};
    float  maxDb{-300.0f};
};

struct PhaseSample {
    bool   valid{false};
    int    bin{-1};
    double freqHz{0.0};
    double phaseRad{0.0};
    double displacementMm{0.0};  // accumulated since the track started
};

struct FrameResult {
    bool      valid{false};
    uint64_t  seq{0};
    uint32_t  frameId{0};
    uint32_t  timestampMs{0};
    uint32_t  rxTickMs{0};
    double    fsHz{0.0};
    double    rampSec{0.0};      // UP ramp duration
    double    fallSec{0.0};      // DOWN ramp duration (Triangle; equals rampSec when symmetric)
    double    periodSec{0.0};
    double    chirpFreqHz{0.0};
    int       chirps{1};
    bool      geometryFromHeader{false}; // segmentation taken from the frame header (firmware)
    size_t    samplesPerChirp{0};
    size_t    riseSamples{0};
    RampShape shape{RampShape::Triangle};
    int       decimation{1};
    size_t    samplesPerFrame{0};
    RampSpectrum         up;
    RampSpectrum         down;
    std::vector<Target>  targets;
    RangeDopplerMap      rd;
    PhaseSample          phase;
    std::vector<Segment> segments;
    double    rangePerHz{0.0};   // m per Hz of beat frequency (0 = unknown)
    double    rangeOffsetM{0.0}; // zero-range calibration applied to targets
    double    lambdaM{0.0};
    double    processingMs{0.0};
    bool      fromMcuFft{false};
    double    mcuPeakHz{0.0};    // peak reported by the MCU (ppm-corrected), 0 if none
};

struct DerivedValues {
    double rampSec{0.0};
    double periodSec{0.0};
    double rangeResM{0.0};        // c / (2B)
    double rangeResEffM{0.0};     // with guard removal
    double rangeMaxM{0.0};        // from effective sample rate
    double velResMps{0.0};        // lambda / (2 M T_PRI)
    double velMaxMps{0.0};        // lambda / (4 T_PRI)
    double fsEffHz{0.0};
    double binHz{0.0};            // Hz per bin (with zero padding)
    double rangeBinM{0.0};
    double beatPerMeterHz{0.0};
    int    decimation{1};
    size_t samplesPerRamp{0};
    size_t samplesUsed{0};
    size_t fftSize{0};
    double usbMBps{0.0};
};

DerivedValues ComputeDerived(const RadarParams& rp, const DspSettings& ds,
                             double fsHz, double chirpFreqHz, size_t samplesPerFrame,
                             double intervalMs, bool sendRaw, bool sendFft, size_t mcuFftSize);

class RadarDsp {
public:
    RadarDsp();

    void SetSettings(const DspSettings& s);
    void SetParams(const RadarParams& p);
    void SetFallbackSampleRate(double fsHz) { m_fallbackFs = fsHz; }
    // ADC clock correction applied to the header's nominal sample rate
    // (1 + ppm * 1e-6). The fallback rate is taken as already calibrated.
    void SetSampleRateScale(double scale) { m_fsScale = (scale > 0.5 && scale < 2.0) ? scale : 1.0; }

    const DspSettings& Settings() const { return m_s; }
    const RadarParams& Params()   const { return m_p; }

    // Process one frame. updateState=false computes without touching
    // averaging / phase / track state (used when viewing an old frame).
    FrameResult Process(const ChirpFrame& f, bool updateState = true);

    // Reset averaging, phase accumulation and tracks.
    void ResetState();

    // Helpers shared with the UI.
    static double RampSeconds(const RadarParams& p, RampShape shape, double chirpFreqHz,
                              size_t samplesPerFrame, double fsHz, int chirps);
    static int AutoDecimation(const RadarParams& p, double fsHz, double rampSec,
                              size_t rampLen, size_t guard, double maxRangeM);

private:
    struct SegSpectrum {
        std::vector<std::complex<float>> cplx;  // scaled complex spectrum (0..nFft/2)
        std::vector<float>               power; // linear power, full-scale sine = 1
        size_t nUsed{0};
        size_t nFft{0};
        double fsEff{0.0};
        bool   valid{false};
    };
    struct TraceState {
        std::vector<float> power;
        bool valid{false};
    };
    struct Track {
        int    id;
        double rangeM;
        double velocity;
        int    missed;
        int    age;
    };

    bool ProcessSegment(const uint16_t* raw, size_t len, size_t guard, int D,
                        double fsHz, SegSpectrum& out, bool keepCplx);
    void FinishSpectrum(const std::vector<float>& power, const SegSpectrum& ref,
                        TraceState* trace, bool updateState, RampSpectrum& out);
    void BuildMcuSpectrum(const ChirpFrame& f, RampSpectrum& out);
    void BuildTargets(FrameResult& r, bool updateState);
    void BuildRangeDoppler(const std::vector<SegSpectrum>& chirps, FrameResult& r);
    void TrackPhase(FrameResult& r, bool updateState);

    const std::vector<float>& WindowFor(size_t n);
    const FirDecimator&       DecimatorFor(int factor);
    const RealFft&            RealFftFor(size_t n);
    const FftPlan&            FftFor(size_t n);

    DspSettings m_s;
    RadarParams m_p;
    double      m_fallbackFs{60058600.0};
    double      m_fsScale{1.0};

    std::map<size_t, std::vector<float>>           m_windows;
    std::map<int, std::unique_ptr<FirDecimator>>   m_decimators;
    std::map<size_t, std::unique_ptr<RealFft>>     m_rfft;
    std::map<size_t, std::unique_ptr<FftPlan>>     m_cfft;
    double      m_windowCg{1.0};

    std::vector<float>               m_x;       // float samples scratch
    std::vector<float>               m_xd;      // decimated scratch
    std::vector<float>               m_fftIn;
    std::vector<std::complex<float>> m_fftOut;

    TraceState m_traceUp, m_traceDn;
    PhaseSample m_phasePrev;
    std::vector<Track> m_tracks;
    int m_nextTrackId{1};
};

} // namespace dsp
