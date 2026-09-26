#pragma once
//
// ChirpStore.h
// Ring buffer of received chirp frames. Thread-safe (single CRITICAL_SECTION).
// Frames are addressed by a monotonically increasing SEQUENCE number, not by
// their position in the ring: when the oldest frame is evicted, sequence
// numbers of the remaining frames do not change, so UI rows never point at
// the wrong frame (they simply stop resolving once evicted).
//

#include "pch.h"
#include "ProtocolDefs.h"

struct ChirpFrame {
    FrameHeader           header{};
    FrameHeaderExt        ext{};    // valid only when hasExt (protocol v2 frame)
    bool                  hasExt{false};
    std::vector<uint16_t> raw;      // full raw samples (UP = front half, DOWN = back half)
    std::vector<float>    fft;      // magnitudes, may be empty
    DWORD                 rx_tick_ms{0}; // host-side GetTickCount at reception
};

class ChirpStore {
public:
    explicit ChirpStore(size_t capacity = 200);
    ~ChirpStore();

    ChirpStore(const ChirpStore&)            = delete;
    ChirpStore& operator=(const ChirpStore&) = delete;

    // Push a new frame. Ownership is moved.
    // Returns the frame's sequence number (monotonic, never reused).
    size_t Push(ChirpFrame&& frame);

    // Copy the frame with sequence `seq` to `out`. Returns false if it was
    // evicted or never existed.
    bool GetAt(size_t seq, ChirpFrame& out) const;

    // Copy most recently pushed frame. Returns false if store is empty.
    bool GetLatest(ChirpFrame& out) const;

    size_t Size() const;
    size_t Capacity() const { return m_capacity; }

    void Clear();

private:
    mutable CRITICAL_SECTION m_cs;
    std::deque<ChirpFrame>   m_frames;
    size_t                   m_capacity;
    size_t                   m_baseSeq{0};   // sequence number of m_frames.front()
};
