#pragma once
//
// TraceDefs.h -- host mirror of firmware Core/Inc/trace_defs.h.
// Keep byte-compatible with the MCU side.
//
// A trace frame (frame_id = FRAME_ID_TRACE, FRAME_FLAG_IS_TRACE) carries
// header.actual_samples records of TRACE_RECORD_SIZE bytes in its raw section.
// header.sample_rate_hz = TRACE_CLOCK_HZ, header.fft_peak_bin = dropped count,
// header.timestamp_ms = HAL tick at capture start.
//

#include "pch.h"
#include <cstdint>

constexpr uint32_t TRACE_RECORD_SIZE = 24;
constexpr uint32_t TRACE_CLOCK_HZ    = 1000000;   // t_us = MCU TIM2 @ 1 MHz

#pragma pack(push, 1)
struct TraceRecord {
    uint32_t t_us;      // 1 us ticks, free-running (wraps ~71 min)
    uint32_t tick_ms;   // HAL_GetTick()
    uint16_t ev;        // TraceEvent
    uint8_t  ctx;       // 0 = main loop, 1 = ISR
    uint8_t  seq;       // rolling sequence (gap = dropped records)
    uint32_t a, b, c;   // event arguments
};
#pragma pack(pop)
static_assert(sizeof(TraceRecord) == TRACE_RECORD_SIZE, "TraceRecord must be 24 bytes");

enum TraceEvent : uint16_t {
    TR_NONE            = 0,
    TR_CYCLE_START     = 1,
    TR_CHIRP_PARAMS    = 2,
    TR_CHIRP_TABLE     = 3,
    TR_CHIRP_ARMED     = 4,
    TR_CAPTURE_START   = 5,
    TR_VSYNC_HIGH      = 6,
    TR_CHIRP_NEXT      = 7,
    TR_CHIRP_DONE      = 8,
    TR_DMA_CHUNK       = 9,
    TR_MDMA_DONE       = 10,
    TR_CAPTURE_DONE    = 11,
    TR_CAPTURE_TIMEOUT = 12,
    TR_CAPTURE_ABORT   = 13,
    TR_MEM_STATE       = 14,
    TR_FFT_START       = 15,
    TR_FFT_DONE        = 16,
    TR_TX_START        = 17,
    TR_TX_DONE         = 18,
    TR_TX_SKIP         = 19,
    TR_CYCLE_END       = 20,
    TR_CMD_RX          = 21,
    TR_ERROR           = 22,
    TR_PHASE_A         = 23,
    TR_MARK            = 24,
    TR_VSYNC_LOW       = 25,
    TR_FSM_CAPTURING   = 26,
};

// Human-readable decoding (implemented in TraceTab.cpp).
LPCTSTR TraceEventName(uint16_t ev);
CString TraceEventDetails(const TraceRecord& r);
