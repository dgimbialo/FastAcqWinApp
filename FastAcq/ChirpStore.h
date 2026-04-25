#pragma once
//
// ChirpStore.h
// Ring buffer of received chirp frames. Thread-safe (single CRITICAL_SECTION).
//

#include "pch.h"
#include "ProtocolDefs.h"

struct ChirpFrame {
    FrameHeader           header{};
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
    // Returns the assigned local index (0..capacity-1) used by UI list.
    size_t Push(ChirpFrame&& frame);

    // Copy frame at logical index (0 = oldest) to `out`. Returns false if empty/out of range.
    bool GetAt(size_t index, ChirpFrame& out) const;

    // Copy most recently pushed frame. Returns false if store is empty.
    bool GetLatest(ChirpFrame& out) const;

    size_t Size() const;
    size_t Capacity() const { return m_capacity; }

    void Clear();

private:
    mutable CRITICAL_SECTION m_cs;
    std::deque<ChirpFrame>   m_frames;
    size_t                   m_capacity;
};
