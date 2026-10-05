#pragma once
//
// VcoCurve.h -- tuning characteristic of the radar VCO (output frequency as
// a function of the tuning voltage) and the sweep it produces when the chirp
// DAC drives Vtune between two voltages. The curve is a short table of
// (Vtune, f) points taken from the VCO datasheet, linearly interpolated;
// it is user-editable because every part differs a little from the typical
// plot. Portable: no Windows/MFC dependencies.
//

#include <string>
#include <vector>

namespace core {

struct VcoPoint {
    double vtuneV{0.0};
    double freqHz{0.0};
};

class VcoCurve {
public:
    // Typical tuning curve of the Analog Devices HMC431 (5.5..6.1 GHz,
    // Vtune 0..10 V). Approximate values read from the datasheet plot;
    // replace them with the measured ones for the actual part.
    static VcoCurve Hmc431Typical();

    // "0:5.50,1:5.59,2:5.67" -> points (Vtune in volts, frequency in GHz).
    // Separators: ',' ';' or whitespace between points, ':' or '=' inside.
    bool Parse(const std::string& text);
    std::string Format() const;

    bool   Valid() const { return m_pts.size() >= 2; }
    size_t Size()  const { return m_pts.size(); }
    const std::vector<VcoPoint>& Points() const { return m_pts; }
    void SetPoints(std::vector<VcoPoint> pts);

    double VminV() const { return Valid() ? m_pts.front().vtuneV : 0.0; }
    double VmaxV() const { return Valid() ? m_pts.back().vtuneV  : 0.0; }

    // Output frequency at a tuning voltage (linear interpolation, linear
    // extrapolation with the end segments outside the table).
    double FreqHz(double vtuneV) const;
    // Local tuning sensitivity, Hz per volt, of the segment containing vtuneV.
    double SensitivityHzPerV(double vtuneV) const;

private:
    std::vector<VcoPoint> m_pts;   // sorted by vtuneV, strictly increasing
};

// What one ramp of the chirp sweeps when Vtune goes from vLowV to vHighV.
struct VcoSweep {
    bool   valid{false};
    bool   outOfTable{false};      // vLow / vHigh outside the tabulated range
    double vLowV{0.0}, vHighV{0.0};
    double fStartHz{0.0}, fStopHz{0.0};
    double f0Hz{0.0};              // centre = (fStart + fStop) / 2
    double bandwidthHz{0.0};       // |fStop - fStart|
    double sensMinHzPerV{0.0}, sensMaxHzPerV{0.0};  // over the swept segment
    double nonlinearityPct{0.0};   // max |f(V) - chord| / B * 100 over the sweep
};

VcoSweep ComputeVcoSweep(const VcoCurve& curve, double vLowV, double vHighV);

} // namespace core
