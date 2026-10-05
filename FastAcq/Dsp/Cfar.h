#pragma once
//
// Cfar.h -- noise-floor estimation and CFAR detectors on a power spectrum.
//

#include <cmath>
#include <cstddef>
#include <vector>

namespace dsp {

enum class DetectorType {
    FixedAboveNoise = 0,   // threshold = median noise floor + thresholdDb
    CaCfar,                // cell-averaging CFAR
    OsCfar,                // ordered-statistic CFAR
    Count
};

const char* DetectorName(DetectorType t);

struct DetectorParams {
    DetectorType type{DetectorType::FixedAboveNoise};
    float thresholdDb{12.0f};   // FixedAboveNoise: dB above the median noise floor
    int   guardCells{2};        // CFAR: guard cells on each side
    int   trainCells{16};       // CFAR: training cells on each side
    float pfa{1e-4f};           // CFAR: probability of false alarm
    float osRankFrac{0.75f};    // OS-CFAR: rank as fraction of 2*trainCells
};

struct DetectorOutput {
    std::vector<float> thresholdDb;   // per bin; +inf outside [fromBin, toBin)
    std::vector<float> noiseDb;       // per bin noise estimate (dB)
    float globalNoiseDb{0.0f};        // median of the evaluated band
};

// powerLin: linear power per bin. Evaluates bins in [fromBin, toBin).
void RunDetector(const std::vector<float>& powerLin, size_t fromBin, size_t toBin,
                 const DetectorParams& p, DetectorOutput& out);

// Threshold multipliers (square-law detector, exponential noise).
double CaCfarAlpha(int nTrainTotal, double pfa);
double OsCfarAlpha(int nTrainTotal, int rank, double pfa);

// Median of v[from, to) (copy + nth_element).
float MedianOf(const std::vector<float>& v, size_t from, size_t to);

inline float PowerToDb(float p) { return (p > 1e-30f) ? 10.0f * std::log10(p) : -300.0f; }

} // namespace dsp
