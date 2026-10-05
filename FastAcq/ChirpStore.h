#pragma once
//
// ChirpStore.h
// Ring buffer of received chirp frames with monotonic sequence numbers.
// Frames are immutable once pushed and shared via shared_ptr, so the UI,
// the DSP thread and the recorder never copy sample buffers.
// Portable: no Windows/MFC dependencies.
//

#include "ProtocolDefs.h"

#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <vector>

struct ChirpFrame {
    FrameHeader           header{};
    FrameHeaderExt        ext{};        // protocol v2 extension (zeros when !hasExt)
    bool                  hasExt{false};
    std::vector<uint16_t> raw;          // raw ADC samples (all chirps of the capture)
    std::vector<float>    fft;          // magnitudes computed by the MCU, may be empty
    uint32_t              rx_tick_ms{0}; // host-side monotonic ms at reception
    uint64_t              seq{0};       // assigned by ChirpStore::Push (monotonic, never reused)

    size_t Bytes() const {
        return sizeof(FrameHeader) + raw.size() * sizeof(uint16_t) + fft.size() * sizeof(float);
    }

    // Peak frequency reported by the MCU, Hz (0 if the frame carries no FFT peak).
    // v2 frames carry a full-precision float; v1 frames with FRAME_FLAG_PEAK_FRAC
    // carry a 1/256-bin fractional offset in reserved0.
    double McuPeakHz() const {
        if (hasExt && ext.peak_freq_hz > 0.0f) return ext.peak_freq_hz;
        double bin = header.fft_peak_bin;
        if (header.data_flags & FRAME_FLAG_PEAK_FRAC)
            bin += static_cast<int8_t>(header.reserved0) / 256.0;
        return bin * header.fft_freq_res_hz;
    }
};

using ChirpFramePtr = std::shared_ptr<const ChirpFrame>;

class ChirpStore {
public:
    static constexpr uint64_t kInvalidSeq = ~0ull;

    explicit ChirpStore(size_t maxFrames = 200, size_t maxBytes = 256ull * 1024 * 1024);

    ChirpStore(const ChirpStore&)            = delete;
    ChirpStore& operator=(const ChirpStore&) = delete;

    // Push a new frame (ownership moved). Returns the assigned sequence number.
    uint64_t Push(ChirpFrame&& frame);

    // Frame with the given sequence number, or nullptr if evicted / unknown.
    ChirpFramePtr Get(uint64_t seq) const;

    // Most recently pushed frame, or nullptr if the store is empty.
    ChirpFramePtr Latest() const;

    // Sequence number of the oldest frame still stored (kInvalidSeq if empty).
    uint64_t FirstSeq() const;
    // Sequence number the next pushed frame will get.
    uint64_t NextSeq() const;

    size_t Size() const;
    size_t Bytes() const;
    size_t Capacity() const { return m_maxFrames; }
    size_t ByteCapacity() const { return m_maxBytes; }

    void SetLimits(size_t maxFrames, size_t maxBytes);

    // Drops all frames; sequence numbers keep increasing.
    void Clear();

private:
    void EvictLocked();

    mutable std::mutex          m_mx;
    std::deque<ChirpFramePtr>   m_frames;
    size_t                      m_maxFrames;
    size_t                      m_maxBytes;
    size_t                      m_bytes{0};
    uint64_t                    m_nextSeq{0};
};
