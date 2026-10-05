#include "pch.h"
#include "DspWorker.h"
#include "AppMessages.h"

DspWorker::DspWorker(HWND target) : m_hwnd(target) {}

DspWorker::~DspWorker() { Stop(); }

void DspWorker::Start()
{
    if (m_thread.joinable()) return;
    m_quit = false;
    m_thread = std::thread([this] { Loop(); });
}

void DspWorker::Stop()
{
    {
        std::lock_guard<std::mutex> lock(m_mx);
        m_quit = true;
    }
    m_cv.notify_all();
    if (m_thread.joinable()) m_thread.join();
}

void DspWorker::Configure(const dsp::DspSettings& s, const dsp::RadarParams& p, double fallbackFs, double fsScale)
{
    {
        std::lock_guard<std::mutex> lock(m_mx);
        m_settings    = s;
        m_params      = p;
        m_fallbackFs  = fallbackFs;
        m_fsScale     = fsScale;
        m_configDirty = true;
    }
    m_cv.notify_all();
}

void DspWorker::ResetState()
{
    {
        std::lock_guard<std::mutex> lock(m_mx);
        m_resetPending = true;
    }
    m_cv.notify_all();
}

void DspWorker::Submit(ChirpFramePtr frame, bool live)
{
    if (!frame) return;
    {
        std::lock_guard<std::mutex> lock(m_mx);
        if (live) m_liveFrame = std::move(frame);
        else      m_selFrame  = std::move(frame);
    }
    m_cv.notify_all();
}

void DspWorker::Loop()
{
    for (;;) {
        ChirpFramePtr frame;
        bool live = false;
        bool applyConfig = false, reset = false;
        dsp::DspSettings s; dsp::RadarParams p; double fs = 0.0, fsScale = 1.0;
        {
            std::unique_lock<std::mutex> lock(m_mx);
            m_cv.wait(lock, [this] { return m_quit || m_liveFrame || m_selFrame || m_configDirty || m_resetPending; });
            if (m_quit) return;
            if (m_configDirty) { applyConfig = true; s = m_settings; p = m_params; fs = m_fallbackFs; fsScale = m_fsScale; m_configDirty = false; }
            if (m_resetPending) { reset = true; m_resetPending = false; }
            // A user-selected frame has priority over the stream.
            if (m_selFrame)       { frame = std::move(m_selFrame); m_selFrame.reset(); live = false; }
            else if (m_liveFrame) { frame = std::move(m_liveFrame); m_liveFrame.reset(); live = true; }
        }
        if (applyConfig) { m_dsp.SetParams(p); m_dsp.SetSettings(s); m_dsp.SetFallbackSampleRate(fs); m_dsp.SetSampleRateScale(fsScale); }
        if (reset) m_dsp.ResetState();
        if (!frame) continue;

        auto result = std::make_shared<dsp::FrameResult>(m_dsp.Process(*frame, live));
        m_lastMs.store(result->processingMs);
        if (::IsWindow(m_hwnd)) {
            auto* payload = new std::shared_ptr<const dsp::FrameResult>(std::move(result));
            if (!::PostMessage(m_hwnd, WM_APP_RESULT_READY, live ? 1 : 0, reinterpret_cast<LPARAM>(payload)))
                delete payload;
        }
    }
}
