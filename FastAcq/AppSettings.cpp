#include "pch.h"
#include "AppSettings.h"
#include "ProtocolDefs.h"

namespace {

class Ini {
public:
    explicit Ini(const CString& path) : m_path(path) {}

    CString GetStr(LPCTSTR sec, LPCTSTR key, LPCTSTR def) const {
        TCHAR buf[1024]{};
        ::GetPrivateProfileString(sec, key, def, buf, 1024, m_path);
        return buf;
    }
    int GetInt(LPCTSTR sec, LPCTSTR key, int def) const {
        CString s = GetStr(sec, key, _T(""));
        return s.IsEmpty() ? def : _ttoi(s);
    }
    double GetDouble(LPCTSTR sec, LPCTSTR key, double def) const {
        CString s = GetStr(sec, key, _T(""));
        return s.IsEmpty() ? def : _tcstod(s, nullptr);
    }
    bool GetBool(LPCTSTR sec, LPCTSTR key, bool def) const { return GetInt(sec, key, def ? 1 : 0) != 0; }

    void Set(LPCTSTR sec, LPCTSTR key, const CString& v) const { ::WritePrivateProfileString(sec, key, v, m_path); }
    void SetInt(LPCTSTR sec, LPCTSTR key, long long v) const { CString s; s.Format(_T("%lld"), v); Set(sec, key, s); }
    void SetDouble(LPCTSTR sec, LPCTSTR key, double v) const { CString s; s.Format(_T("%.10g"), v); Set(sec, key, s); }
    void SetBool(LPCTSTR sec, LPCTSTR key, bool v) const { SetInt(sec, key, v ? 1 : 0); }

private:
    CString m_path;
};

template <typename E>
E ClampEnum(int v, E count) {
    if (v < 0 || v >= static_cast<int>(count)) v = 0;
    return static_cast<E>(v);
}

} // namespace

core::VcoCurve VcoSettings::Curve() const
{
    core::VcoCurve c;
    if (!curveText.empty() && c.Parse(curveText)) return c;
    return core::VcoCurve::Hmc431Typical();
}

core::VcoSweep VcoSettings::SweepFor(int amplitude) const
{
    if (amplitude < 1) amplitude = 1;
    if (amplitude > static_cast<int>(AMPLITUDE_MAX)) amplitude = AMPLITUDE_MAX;
    const double vHigh = vtuneAtDac0V + (vtuneAtDacFullV - vtuneAtDac0V) * amplitude / static_cast<double>(AMPLITUDE_MAX);
    return core::ComputeVcoSweep(Curve(), vtuneAtDac0V, vHigh);
}

core::VcoSweep AppSettings::ApplyVcoToRadar()
{
    core::VcoSweep s = vco.SweepFor(acq.amplitude);
    if (vco.useCurve && s.valid && s.bandwidthHz > 0.0) {
        radar.f0Hz        = s.f0Hz;
        radar.bandwidthHz = s.bandwidthHz;
    }
    return s;
}

CString AppSettings::DefaultPath()
{
    TCHAR exe[MAX_PATH]{};
    ::GetModuleFileName(nullptr, exe, MAX_PATH);
    CString p(exe);
    int slash = p.ReverseFind(_T('\\'));
    if (slash >= 0) p = p.Left(slash + 1);
    return p + _T("FastAcq.ini");
}

bool AppSettings::Load(const CString& path)
{
    if (::GetFileAttributes(path) == INVALID_FILE_ATTRIBUTES) return false;
    Ini ini(path);

    acq.mode        = ini.GetInt(_T("Acq"), _T("Mode"),        acq.mode);
    acq.chirpFreqHz = ini.GetInt(_T("Acq"), _T("ChirpFreqHz"), acq.chirpFreqHz);
    acq.samples     = ini.GetInt(_T("Acq"), _T("Samples"),     acq.samples);
    acq.intervalMs  = ini.GetInt(_T("Acq"), _T("IntervalMs"),  acq.intervalMs);
    acq.riseUs      = ini.GetInt(_T("Acq"), _T("RiseUs"),      acq.riseUs);
    acq.fallUs      = ini.GetInt(_T("Acq"), _T("FallUs"),      acq.fallUs);
    acq.amplitude   = ini.GetInt(_T("Acq"), _T("Amplitude"),   acq.amplitude);
    acq.burst       = ini.GetInt(_T("Acq"), _T("Burst"),       acq.burst);
    acq.sendRaw     = ini.GetBool(_T("Acq"), _T("SendRaw"),    acq.sendRaw);
    acq.sendFft     = ini.GetBool(_T("Acq"), _T("SendFft"),    acq.sendFft);

    radar.f0Hz              = ini.GetDouble(_T("Radar"), _T("F0Hz"),        radar.f0Hz);
    radar.bandwidthHz       = ini.GetDouble(_T("Radar"), _T("BandwidthHz"), radar.bandwidthHz);
    // Earlier builds wrote a 24 GHz / 200 MHz placeholder; the device sweeps 5..6 GHz.
    if (radar.f0Hz == 24.0e9 && radar.bandwidthHz == 200.0e6) { radar.f0Hz = 5.5e9; radar.bandwidthHz = 1.0e9; }
    radar.rampSec           = ini.GetDouble(_T("Radar"), _T("RampSec"),     radar.rampSec);
    radar.rangeOffsetM      = ini.GetDouble(_T("Radar"), _T("RangeOffsetM"),radar.rangeOffsetM);
    radar.pairMaxVelocityMps= ini.GetDouble(_T("Radar"), _T("PairMaxVel"),  radar.pairMaxVelocityMps);
    vco.useCurve        = ini.GetBool(_T("Vco"), _T("UseCurve"), vco.useCurve);
    vco.vtuneAtDac0V    = ini.GetDouble(_T("Vco"), _T("VtuneAtDac0V"), vco.vtuneAtDac0V);
    vco.vtuneAtDacFullV = ini.GetDouble(_T("Vco"), _T("VtuneAtDacFullV"), vco.vtuneAtDacFullV);
    vco.curveText       = std::string(CStringA(ini.GetStr(_T("Vco"), _T("Curve"), _T(""))));
    if (!(vco.vtuneAtDacFullV > vco.vtuneAtDac0V)) { vco.vtuneAtDac0V = 0.0; vco.vtuneAtDacFullV = 10.0; }

    dsp.chirpsInFrame   = ini.GetInt(_T("Dsp"), _T("ChirpsInFrame"), dsp.chirpsInFrame);
    dsp.shape           = ClampEnum(ini.GetInt(_T("Dsp"), _T("Shape"), static_cast<int>(dsp.shape)), dsp::RampShape::Count);
    dsp.guardPct        = static_cast<float>(ini.GetDouble(_T("Dsp"), _T("GuardPct"), dsp.guardPct));
    dsp.detrend         = ini.GetBool(_T("Dsp"), _T("Detrend"), dsp.detrend);
    dsp.decimation      = ini.GetInt(_T("Dsp"), _T("Decimation"), dsp.decimation);
    dsp.maxRangeM       = ini.GetDouble(_T("Dsp"), _T("MaxRangeM"), dsp.maxRangeM);
    dsp.window          = ClampEnum(ini.GetInt(_T("Dsp"), _T("Window"), static_cast<int>(dsp.window)), dsp::WindowType::Count);
    dsp.kaiserBeta      = static_cast<float>(ini.GetDouble(_T("Dsp"), _T("KaiserBeta"), dsp.kaiserBeta));
    dsp.zeroPad         = ini.GetInt(_T("Dsp"), _T("ZeroPad"), dsp.zeroPad);
    dsp.rangeGainDbPerDecade = static_cast<float>(ini.GetDouble(_T("Dsp"), _T("RangeGain"), dsp.rangeGainDbPerDecade));
    dsp.trace           = ClampEnum(ini.GetInt(_T("Dsp"), _T("Trace"), static_cast<int>(dsp.trace)), dsp::TraceMode::Count);
    dsp.avgAlpha        = static_cast<float>(ini.GetDouble(_T("Dsp"), _T("AvgAlpha"), dsp.avgAlpha));
    dsp.detector.type   = ClampEnum(ini.GetInt(_T("Dsp"), _T("Detector"), static_cast<int>(dsp.detector.type)), dsp::DetectorType::Count);
    dsp.detector.thresholdDb = static_cast<float>(ini.GetDouble(_T("Dsp"), _T("ThresholdDb"), dsp.detector.thresholdDb));
    dsp.detector.guardCells  = ini.GetInt(_T("Dsp"), _T("CfarGuard"), dsp.detector.guardCells);
    dsp.detector.trainCells  = ini.GetInt(_T("Dsp"), _T("CfarTrain"), dsp.detector.trainCells);
    dsp.detector.pfa         = static_cast<float>(ini.GetDouble(_T("Dsp"), _T("Pfa"), dsp.detector.pfa));
    dsp.interp          = ClampEnum(ini.GetInt(_T("Dsp"), _T("Interp"), static_cast<int>(dsp.interp)), dsp::PeakInterp::Count);
    dsp.maxPeaks        = ini.GetInt(_T("Dsp"), _T("MaxPeaks"), dsp.maxPeaks);
    dsp.mti             = ini.GetBool(_T("Dsp"), _T("Mti"), dsp.mti);
    dsp.firmwareGeometry= ini.GetBool(_T("Dsp"), _T("FirmwareGeometry"), dsp.firmwareGeometry);
    dsp.toneEstimate    = ini.GetBool(_T("Dsp"), _T("ToneEstimate"), dsp.toneEstimate);
    dsp.useMcuFft       = ini.GetBool(_T("Dsp"), _T("UseMcuFft"), dsp.useMcuFft);
    dsp.trackTargets    = ini.GetBool(_T("Dsp"), _T("Track"), dsp.trackTargets);

    display.dbTop        = static_cast<float>(ini.GetDouble(_T("Display"), _T("DbTop"), display.dbTop));
    display.dbBottom     = static_cast<float>(ini.GetDouble(_T("Display"), _T("DbBottom"), display.dbBottom));
    display.autoDb       = ini.GetBool(_T("Display"), _T("AutoDb"), display.autoDb);
    display.palette      = ini.GetInt(_T("Display"), _T("Palette"), display.palette);
    display.waterfallRows= ini.GetInt(_T("Display"), _T("WaterfallRows"), display.waterfallRows);
    display.darkTheme    = ini.GetBool(_T("Display"), _T("Dark"), display.darkTheme);
    display.adcBits      = ini.GetInt(_T("Display"), _T("AdcBits"), display.adcBits);
    display.vRef         = static_cast<float>(ini.GetDouble(_T("Display"), _T("VRef"), display.vRef));
    display.showVolts    = ini.GetBool(_T("Display"), _T("ShowVolts"), display.showVolts);
    display.showUp       = ini.GetBool(_T("Display"), _T("ShowUp"), display.showUp);
    display.showDown     = ini.GetBool(_T("Display"), _T("ShowDown"), display.showDown);
    display.showThreshold= ini.GetBool(_T("Display"), _T("ShowThreshold"), display.showThreshold);
    display.showNoise    = ini.GetBool(_T("Display"), _T("ShowNoise"), display.showNoise);
    display.showPeaks    = ini.GetBool(_T("Display"), _T("ShowPeaks"), display.showPeaks);
    display.showRangeDoppler = ini.GetBool(_T("Display"), _T("ShowRD"), display.showRangeDoppler);
    display.dots         = ini.GetBool(_T("Display"), _T("Dots"), display.dots);

    sampleRateCalHz = static_cast<uint32_t>(ini.GetInt(_T("App"), _T("SampleRateCalHz"), static_cast<int>(sampleRateCalHz)));
    fsPpm           = ini.GetDouble(_T("Calibration"), _T("FsPpm"), fsPpm);
    chirpsFromBurst = ini.GetBool(_T("App"), _T("ChirpsFromBurst"), chirpsFromBurst);
    verboseLog      = ini.GetBool(_T("App"), _T("VerboseLog"), verboseLog);
    autoConnect     = ini.GetBool(_T("App"), _T("AutoConnect"), autoConnect);
    logAutoScroll   = ini.GetBool(_T("App"), _T("LogAutoScroll"), logAutoScroll);
    lastPort        = ini.GetStr(_T("App"), _T("LastPort"), lastPort);
    lastDir         = ini.GetStr(_T("App"), _T("LastDir"), lastDir);
    activeTab       = ini.GetInt(_T("App"), _T("ActiveTab"), activeTab);
    windowRect.left   = ini.GetInt(_T("Window"), _T("Left"),   0);
    windowRect.top    = ini.GetInt(_T("Window"), _T("Top"),    0);
    windowRect.right  = ini.GetInt(_T("Window"), _T("Right"),  0);
    windowRect.bottom = ini.GetInt(_T("Window"), _T("Bottom"), 0);
    windowMax         = ini.GetBool(_T("Window"), _T("Max"),   false);
    splitRadar1 = static_cast<float>(ini.GetDouble(_T("Window"), _T("SplitRadar1"), splitRadar1));
    splitRadar2 = static_cast<float>(ini.GetDouble(_T("Window"), _T("SplitRadar2"), splitRadar2));
    splitScope  = static_cast<float>(ini.GetDouble(_T("Window"), _T("SplitScope"),  splitScope));

    // Sanity clamps.
    if (dsp.chirpsInFrame < 1) dsp.chirpsInFrame = 1;
    if (dsp.zeroPad < 1) dsp.zeroPad = 1;
    if (dsp.maxPeaks < 1) dsp.maxPeaks = 1;
    if (display.waterfallRows < 64) display.waterfallRows = 64;
    if (display.waterfallRows > 4096) display.waterfallRows = 4096;
    if (display.dbTop <= display.dbBottom) { display.dbTop = 0.0f; display.dbBottom = -120.0f; }
    if (display.palette < 0 || display.palette >= static_cast<int>(Palette::Count)) display.palette = 0;
    if (sampleRateCalHz == 0) sampleRateCalHz = 60058600;
    if (!(fsPpm > -100000.0 && fsPpm < 100000.0)) fsPpm = 0.0;
    if (acq.riseUs < 0 || acq.riseUs > 65535) acq.riseUs = 0;
    if (acq.fallUs < 0 || acq.fallUs > 65535) acq.fallUs = 0;
    return true;
}

bool AppSettings::Save(const CString& path) const
{
    Ini ini(path);
    ini.SetInt(_T("Acq"), _T("Mode"),        acq.mode);
    ini.SetInt(_T("Acq"), _T("ChirpFreqHz"), acq.chirpFreqHz);
    ini.SetInt(_T("Acq"), _T("Samples"),     acq.samples);
    ini.SetInt(_T("Acq"), _T("IntervalMs"),  acq.intervalMs);
    ini.SetInt(_T("Acq"), _T("RiseUs"),      acq.riseUs);
    ini.SetInt(_T("Acq"), _T("FallUs"),      acq.fallUs);
    ini.SetInt(_T("Acq"), _T("Amplitude"),   acq.amplitude);
    ini.SetInt(_T("Acq"), _T("Burst"),       acq.burst);
    ini.SetBool(_T("Acq"), _T("SendRaw"),    acq.sendRaw);
    ini.SetBool(_T("Acq"), _T("SendFft"),    acq.sendFft);

    ini.SetDouble(_T("Radar"), _T("F0Hz"),         radar.f0Hz);
    ini.SetDouble(_T("Radar"), _T("BandwidthHz"),  radar.bandwidthHz);
    ini.SetDouble(_T("Radar"), _T("RampSec"),      radar.rampSec);
    ini.SetDouble(_T("Radar"), _T("RangeOffsetM"), radar.rangeOffsetM);
    ini.SetBool(_T("Vco"), _T("UseCurve"),        vco.useCurve);
    ini.SetDouble(_T("Vco"), _T("VtuneAtDac0V"),  vco.vtuneAtDac0V);
    ini.SetDouble(_T("Vco"), _T("VtuneAtDacFullV"), vco.vtuneAtDacFullV);
    ini.Set(_T("Vco"), _T("Curve"),               CString(vco.Curve().Format().c_str()));
    ini.SetDouble(_T("Radar"), _T("PairMaxVel"),   radar.pairMaxVelocityMps);

    ini.SetInt(_T("Dsp"), _T("ChirpsInFrame"), dsp.chirpsInFrame);
    ini.SetInt(_T("Dsp"), _T("Shape"),         static_cast<int>(dsp.shape));
    ini.SetDouble(_T("Dsp"), _T("GuardPct"),   dsp.guardPct);
    ini.SetBool(_T("Dsp"), _T("Detrend"),      dsp.detrend);
    ini.SetInt(_T("Dsp"), _T("Decimation"),    dsp.decimation);
    ini.SetDouble(_T("Dsp"), _T("MaxRangeM"),  dsp.maxRangeM);
    ini.SetInt(_T("Dsp"), _T("Window"),        static_cast<int>(dsp.window));
    ini.SetDouble(_T("Dsp"), _T("KaiserBeta"), dsp.kaiserBeta);
    ini.SetInt(_T("Dsp"), _T("ZeroPad"),       dsp.zeroPad);
    ini.SetDouble(_T("Dsp"), _T("RangeGain"),  dsp.rangeGainDbPerDecade);
    ini.SetInt(_T("Dsp"), _T("Trace"),         static_cast<int>(dsp.trace));
    ini.SetDouble(_T("Dsp"), _T("AvgAlpha"),   dsp.avgAlpha);
    ini.SetInt(_T("Dsp"), _T("Detector"),      static_cast<int>(dsp.detector.type));
    ini.SetDouble(_T("Dsp"), _T("ThresholdDb"),dsp.detector.thresholdDb);
    ini.SetInt(_T("Dsp"), _T("CfarGuard"),     dsp.detector.guardCells);
    ini.SetInt(_T("Dsp"), _T("CfarTrain"),     dsp.detector.trainCells);
    ini.SetDouble(_T("Dsp"), _T("Pfa"),        dsp.detector.pfa);
    ini.SetInt(_T("Dsp"), _T("Interp"),        static_cast<int>(dsp.interp));
    ini.SetInt(_T("Dsp"), _T("MaxPeaks"),      dsp.maxPeaks);
    ini.SetBool(_T("Dsp"), _T("Mti"),          dsp.mti);
    ini.SetBool(_T("Dsp"), _T("FirmwareGeometry"), dsp.firmwareGeometry);
    ini.SetBool(_T("Dsp"), _T("ToneEstimate"), dsp.toneEstimate);
    ini.SetBool(_T("Dsp"), _T("UseMcuFft"),    dsp.useMcuFft);
    ini.SetBool(_T("Dsp"), _T("Track"),        dsp.trackTargets);

    ini.SetDouble(_T("Display"), _T("DbTop"),     display.dbTop);
    ini.SetDouble(_T("Display"), _T("DbBottom"),  display.dbBottom);
    ini.SetBool(_T("Display"), _T("AutoDb"),      display.autoDb);
    ini.SetInt(_T("Display"), _T("Palette"),      display.palette);
    ini.SetInt(_T("Display"), _T("WaterfallRows"),display.waterfallRows);
    ini.SetBool(_T("Display"), _T("Dark"),        display.darkTheme);
    ini.SetInt(_T("Display"), _T("AdcBits"),      display.adcBits);
    ini.SetDouble(_T("Display"), _T("VRef"),      display.vRef);
    ini.SetBool(_T("Display"), _T("ShowVolts"),   display.showVolts);
    ini.SetBool(_T("Display"), _T("ShowUp"),      display.showUp);
    ini.SetBool(_T("Display"), _T("ShowDown"),    display.showDown);
    ini.SetBool(_T("Display"), _T("ShowThreshold"), display.showThreshold);
    ini.SetBool(_T("Display"), _T("ShowNoise"),   display.showNoise);
    ini.SetBool(_T("Display"), _T("ShowPeaks"),   display.showPeaks);
    ini.SetBool(_T("Display"), _T("ShowRD"),      display.showRangeDoppler);
    ini.SetBool(_T("Display"), _T("Dots"),        display.dots);

    ini.SetInt(_T("App"), _T("SampleRateCalHz"), sampleRateCalHz);
    ini.SetDouble(_T("Calibration"), _T("FsPpm"), fsPpm);
    ini.SetBool(_T("App"), _T("ChirpsFromBurst"), chirpsFromBurst);
    ini.SetBool(_T("App"), _T("VerboseLog"),      verboseLog);
    ini.SetBool(_T("App"), _T("AutoConnect"),     autoConnect);
    ini.SetBool(_T("App"), _T("LogAutoScroll"),   logAutoScroll);
    ini.Set(_T("App"), _T("LastPort"),            lastPort);
    ini.Set(_T("App"), _T("LastDir"),             lastDir);
    ini.SetInt(_T("App"), _T("ActiveTab"),        activeTab);
    ini.SetInt(_T("Window"), _T("Left"),   windowRect.left);
    ini.SetInt(_T("Window"), _T("Top"),    windowRect.top);
    ini.SetInt(_T("Window"), _T("Right"),  windowRect.right);
    ini.SetInt(_T("Window"), _T("Bottom"), windowRect.bottom);
    ini.SetBool(_T("Window"), _T("Max"),   windowMax);
    ini.SetDouble(_T("Window"), _T("SplitRadar1"), splitRadar1);
    ini.SetDouble(_T("Window"), _T("SplitRadar2"), splitRadar2);
    ini.SetDouble(_T("Window"), _T("SplitScope"),  splitScope);
    return true;
}
