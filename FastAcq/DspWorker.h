#pragma once
//
// DspWorker -- background thread that runs dsp::RadarDsp on frames. Keeps
// only the most recent pending frame (coalescing), so the UI never lags
// behind the stream. Results are posted to the target window as
// WM_APP_RESULT_READY (lParam = new std::shared_ptr<const dsp::FrameResult>*).
//

#include "pch.h"
#include "ChirpStore.h"
#include "Dsp/RadarDsp.h"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

class DspWorker {
public:
    explicit DspWorker(HWND target);
    ~DspWorker();

    DspWorker(const DspWorker&)            = delete;
    DspWorker& operator=(const DspWorker&) = delete;

    void Start();
    void Stop();

    void Configure(const dsp::DspSettings& s, const dsp::RadarParams& p, double fallbackFs);
    void ResetState();

    // live = true: part of the stream (updates averaging / phase / tracks);
    // live = false: a frame picked by the user (state is left untouched).
    void Submit(ChirpFramePtr frame, bool live);

    double LastProcessingMs() const { return m_lastMs.load(); }

private:
    void Loop();

    HWND                    m_hwnd;
    std::thread             m_thread;
    std::mutex              m_mx;
    std::condition_variable m_cv;
    bool                    m_quit{false};
    bool                    m_configDirty{false};
    bool                    m_resetPending{false};
    dsp::DspSettings        m_settings;
    dsp::RadarParams        m_params;
    double                  m_fallbackFs{60058600.0};
    ChirpFramePtr           m_liveFrame;
    ChirpFramePtr           m_selFrame;
    std::atomic<double>     m_lastMs{0.0};
    dsp::RadarDsp           m_dsp;    // worker thread only
};
