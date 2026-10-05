#pragma once
//
// ProtocolDefs.h
// Host-side mirror of firmware usb_protocol.h (STM32H743 Fast Acquisition Device).
// Keep layouts byte-compatible with the MCU side.
// Portable: no Windows/MFC dependencies (shared with the unit tests).
//

#include <cstdint>
#include <cstddef>

// Magic at the start of every frame header.
//   v1 (0xFACEDA7A): 56-byte header = FrameHeader
//   v2 (0xFACEDA7B): FrameHeader + FrameHeaderExt, total ext.header_size bytes
//                    (>= 64); bytes beyond the known extension are skipped.
constexpr uint32_t FRAME_MAGIC_V1 = 0xFACEDA7AUL;
constexpr uint32_t FRAME_MAGIC_V2 = 0xFACEDA7BUL;
constexpr uint32_t FRAME_MAGIC    = FRAME_MAGIC_V1;   // legacy name
constexpr size_t   kHeaderV1Size  = 56;
constexpr size_t   kHeaderV2Size  = 64;               // minimum v2 header
constexpr size_t   kHeaderMaxSize = 256;              // sanity bound for header_size

// CMD opcodes (host -> MCU)
constexpr uint8_t CMD_START_CHIRP     = 0x01; // arg1 = chirp freq Hz (100..24000)
constexpr uint8_t CMD_GET_FRAME       = 0x02; // alias of CMD_TRIGGER (compat)
constexpr uint8_t CMD_PING            = 0x03;
constexpr uint8_t CMD_SET_SAMPLES     = 0x04; // samples = arg1 | (arg2 << 16); 0 = auto
constexpr uint8_t CMD_SET_MODE        = 0x05; // arg1 = MODE_*
constexpr uint8_t CMD_SET_DATA_MASK   = 0x06; // arg1 = bit0=RAW, bit1=FFT
constexpr uint8_t CMD_SET_INTERVAL    = 0x07; // arg1 = ms between capture cycles
constexpr uint8_t CMD_TRIGGER         = 0x08; // one-shot trigger (SINGLE/IDLE)
constexpr uint8_t CMD_GET_STATUS      = 0x09;
constexpr uint8_t CMD_SET_AMPLITUDE   = 0x0A; // arg1 = chirp amplitude, DAC counts 1..4095
constexpr uint8_t CMD_SET_BURST       = 0x0B; // arg1 = chirps per capture window 1..1024
constexpr uint8_t CMD_ABORT           = 0x0C; // abort current capture immediately
constexpr uint8_t CMD_SET_TRACE       = 0x0D; // arg1 = 1 enable / 0 disable MCU acquisition trace
constexpr uint8_t CMD_SET_RAMP        = 0x0E; // arg1 = rise_us, arg2 = fall_us (0/0 = symmetric from freq)
constexpr uint8_t CMD_GET_TRACE       = 0x0F; // send the trace buffer now

// Modes
constexpr uint16_t MODE_IDLE       = 0;
constexpr uint16_t MODE_CONTINUOUS = 1;
constexpr uint16_t MODE_SINGLE     = 2;

// data_flags bits
constexpr uint8_t FRAME_FLAG_HAS_RAW   = 1u << 0;
constexpr uint8_t FRAME_FLAG_HAS_FFT   = 1u << 1;
constexpr uint8_t FRAME_FLAG_FFT_VALID = 1u << 2;
constexpr uint8_t FRAME_FLAG_IS_STATUS = 1u << 3;
constexpr uint8_t FRAME_FLAG_IS_ACK    = 1u << 4;
constexpr uint8_t FRAME_FLAG_IS_TRACE  = 1u << 5;   // raw section = TraceRecord[] (TraceDefs.h)
// DATA frame: reserved0 = int8 sub-bin peak offset in 1/256 bin.
// Peak Hz = (fft_peak_bin + (int8_t)reserved0 / 256) * fft_freq_res_hz
constexpr uint8_t FRAME_FLAG_PEAK_FRAC = 1u << 6;

// Reserved frame_id values for service frames (header-only)
constexpr uint32_t FRAME_ID_PONG   = 0xFFFFFFFFu;
constexpr uint32_t FRAME_ID_STATUS = 0xFFFFFFFEu;
constexpr uint32_t FRAME_ID_ACK    = 0xFFFFFFFDu;
constexpr uint32_t FRAME_ID_TRACE  = 0xFFFFFFFCu;

// ACK status codes (carried in fft_peak_bin of an ACK frame)
constexpr uint32_t ACK_OK        = 0;
constexpr uint32_t ACK_BAD_ARG   = 1;
constexpr uint32_t ACK_BAD_STATE = 2;
constexpr uint32_t ACK_HW_FAIL   = 3;

// Firmware limits (mirrored here so the UI can clamp before sending).
constexpr uint32_t CHIRP_FREQ_MIN_HZ = 100;
constexpr uint32_t CHIRP_FREQ_MAX_HZ = 24000;
constexpr uint32_t SAMPLES_MAX       = 650000;
constexpr uint32_t INTERVAL_MIN_MS   = 5;
constexpr uint32_t INTERVAL_MAX_MS   = 10000;
constexpr uint32_t AMPLITUDE_MAX     = 4095;
constexpr uint32_t BURST_MAX         = 1024;

// DATA / TRACE frame header extension (reserved1, little-endian):
//   [0..1] amplitude DAC counts   [2..3] burst count
//   [4..5] rise_us                [6..7] fall_us   (ACTUAL chirp geometry)
// chirp_freq_hz carries the ACTUAL chirp frequency, timestamp_ms = capture start tick.
//
// STATUS frame field mapping (see firmware CONTROL_API_PLAN.md section 2.4):
//   fft_size        <- mode
//   fft_peak_bin    <- data_mask
//   fft_peak_mag    <- interval_ms (as float)
//   actual_samples  <- samples override (0 = auto)
//   chirp_freq_hz   <- chirp frequency
//   reserved1[0..1] <- amplitude, DAC counts (uint16 LE)
//   reserved1[2..3] <- burst count (uint16 LE)
//   reserved1[4]    <- MCU FSM state (0=idle-wait, 1=capturing)
//   reserved1[5]    <- last error code (0 = none)
//   reserved1[6..7] <- configured rise_us (0 = symmetric), fft_freq_res_hz <- fall_us
//   reserved0       <- trace flags: bit0 enabled, bit1 compiled in

#pragma pack(push, 1)

// 8-byte host command: [CMD 1B][ARG1 2B][ARG2 2B][ARG3 2B][CRC8 1B]
struct ProtocolCmd {
    uint8_t  cmd;
    uint16_t arg1;
    uint16_t arg2;
    uint16_t arg3;
    uint8_t  crc8;
};
static_assert(sizeof(ProtocolCmd) == 8, "ProtocolCmd must be 8 bytes");

// 56-byte frame header.
struct FrameHeader {
    uint32_t magic;            // FRAME_MAGIC
    uint32_t frame_id;         // monotonic
    uint32_t timestamp_ms;     // HAL_GetTick() at capture start
    uint32_t actual_samples;   // number of valid raw uint16 samples
    uint32_t sample_rate_hz;   // ADC sample rate
    uint16_t chirp_freq_hz;    // chirp frequency used
    uint8_t  data_flags;       // FRAME_FLAG_* bitmask
    uint8_t  reserved0;
    uint32_t fft_size;         // 0 if no FFT
    uint32_t fft_peak_bin;     // bin index of magnitude peak
    float    fft_peak_mag;     // magnitude at peak
    float    fft_freq_res_hz;  // Hz per FFT bin
    uint32_t raw_data_bytes;   // bytes of raw data following this header
    uint32_t fft_data_bytes;   // bytes of FFT magnitude data following raw
    uint8_t  reserved1[8];     // pad to 56
};
static_assert(sizeof(FrameHeader) == 56, "FrameHeader must be 56 bytes");

// v2 extension, follows FrameHeader in v2 frames (header bytes 56..).
// Fields beyond the received header_size are absent and read as 0.
struct FrameHeaderExt {
    float    peak_freq_hz;      // DATA frame with FFT: interpolated peak, Hz (full precision)
    uint16_t header_size;       // total header bytes incl. extension (>= 64)
    uint8_t  proto_version;     // 2
    uint8_t  reserved2;
    uint32_t samples_per_chirp; // header_size >= 72: exact ADC samples per chirp (0 = unknown)
    uint32_t rise_samples;      // header_size >= 72: exact ADC samples in the rising ramp
};
static_assert(sizeof(FrameHeaderExt) == 16, "FrameHeaderExt must be 16 bytes");
static_assert(sizeof(FrameHeader) + offsetof(FrameHeaderExt, header_size) == 60,
              "header_size must sit at header offset 60");

#pragma pack(pop)

// Little-endian u16 inside reserved1 (offset 0..6).
inline uint16_t HeaderU16(const FrameHeader& h, int off) {
    return static_cast<uint16_t>(h.reserved1[off] | (h.reserved1[off + 1] << 8));
}
inline uint16_t HeaderAmplitude(const FrameHeader& h) { return HeaderU16(h, 0); }
inline uint16_t HeaderBurst(const FrameHeader& h)     { return HeaderU16(h, 2); }
inline uint16_t HeaderRiseUs(const FrameHeader& h)    { return HeaderU16(h, 4); }
inline uint16_t HeaderFallUs(const FrameHeader& h)    { return HeaderU16(h, 6); }

// Sample geometry of the FIRST chirp inside a RAW frame.
// Protocol v2 (ext with samples_per_chirp) gives exact sample counts; older
// firmware only has rise/fall in whole microseconds (off by a few samples),
// and firmware that does not fill rise/fall (0/0) is treated as unknown
// (returns 0 so the caller falls back to its own segmentation).
inline size_t ChirpPeriodSamples(const FrameHeader& h, size_t nRaw) {
    const uint32_t rise = HeaderRiseUs(h), fall = HeaderFallUs(h);
    if (rise && fall && h.sample_rate_hz) {
        const size_t per = static_cast<size_t>(
            (static_cast<uint64_t>(rise) + fall) * h.sample_rate_hz / 1000000ull);
        return (per > 0 && per <= nRaw) ? per : 0;
    }
    return 0;
}
inline size_t ChirpRiseSamples(const FrameHeader& h, size_t nRaw) {
    const uint32_t rise = HeaderRiseUs(h), fall = HeaderFallUs(h);
    if (rise && fall && h.sample_rate_hz) {
        const size_t per = ChirpPeriodSamples(h, nRaw);
        const size_t r = static_cast<size_t>(
            static_cast<uint64_t>(rise) * h.sample_rate_hz / 1000000ull);
        if (per == 0) return 0;
        return (r > 0 && r < per) ? r : per / 2;
    }
    return 0;
}
inline size_t ChirpPeriodSamples(const FrameHeader& h, const FrameHeaderExt* ext, size_t nRaw) {
    if (ext && ext->samples_per_chirp > 0 && ext->samples_per_chirp <= nRaw)
        return ext->samples_per_chirp;
    return ChirpPeriodSamples(h, nRaw);
}
inline size_t ChirpRiseSamples(const FrameHeader& h, const FrameHeaderExt* ext, size_t nRaw) {
    if (ext && ext->samples_per_chirp > 0 && ext->rise_samples > 0) {
        const size_t per = ChirpPeriodSamples(h, ext, nRaw);
        if (per == 0) return 0;
        return (ext->rise_samples < per) ? ext->rise_samples : per / 2;
    }
    return ChirpRiseSamples(h, nRaw);
}

// Decoded STATUS frame (helper for UI / log).
struct DeviceStatus {
    uint32_t mode{0};
    uint32_t dataMask{0};
    uint32_t intervalMs{0};
    uint32_t samples{0};       // 0 = auto
    uint16_t chirpFreqHz{0};
    uint16_t amplitude{0};
    uint16_t burst{1};
    uint8_t  fsmState{0};
    uint8_t  lastError{0};
    uint16_t riseUs{0};        // configured rise, us (0 = symmetric from freq)
    uint16_t fallUs{0};        // configured fall, us
    bool     traceOn{false};   // MCU acquisition trace enabled
    bool     traceAvail{false};// trace support compiled into firmware
};

inline DeviceStatus DecodeStatusFrame(const FrameHeader& h) {
    DeviceStatus s;
    s.mode        = h.fft_size;
    s.dataMask    = h.fft_peak_bin;
    s.intervalMs  = static_cast<uint32_t>(h.fft_peak_mag);
    s.samples     = h.actual_samples;
    s.chirpFreqHz = h.chirp_freq_hz;
    s.amplitude   = static_cast<uint16_t>(h.reserved1[0] | (h.reserved1[1] << 8));
    s.burst       = static_cast<uint16_t>(h.reserved1[2] | (h.reserved1[3] << 8));
    if (s.burst == 0) s.burst = 1;
    s.fsmState    = h.reserved1[4];
    s.lastError   = h.reserved1[5];
    s.riseUs      = HeaderU16(h, 6);
    s.fallUs      = static_cast<uint16_t>(h.fft_freq_res_hz);
    s.traceOn     = (h.reserved0 & 0x01) != 0;
    s.traceAvail  = (h.reserved0 & 0x02) != 0;
    return s;
}

// CRC-8 (poly 0x07, init 0x00) -- matches MCU implementation.
inline uint8_t Crc8(const uint8_t* data, size_t len) {
    uint8_t crc = 0x00;
    for (size_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (int b = 0; b < 8; ++b) {
            crc = (crc & 0x80) ? static_cast<uint8_t>((crc << 1) ^ 0x07)
                               : static_cast<uint8_t>(crc << 1);
        }
    }
    return crc;
}

// CRC-32 (IEEE 802.3, reflected, init 0xFFFFFFFF, xor-out 0xFFFFFFFF), table driven.
// Matches STM32 HAL_CRC with default poly when configured in reflected mode.
// Incremental form: state = Crc32Init(); state = Crc32Update(state, ...); crc = Crc32Final(state).
struct Crc32Table {
    uint32_t t[256];
    constexpr Crc32Table() : t{} {
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t c = i;
            for (int b = 0; b < 8; ++b)
                c = (c & 1u) ? (c >> 1) ^ 0xEDB88320u : (c >> 1);
            t[i] = c;
        }
    }
};
inline const Crc32Table& Crc32Tab() {
    static constexpr Crc32Table tab{};
    return tab;
}
inline uint32_t Crc32Init() { return 0xFFFFFFFFu; }
inline uint32_t Crc32Update(uint32_t state, const uint8_t* data, size_t len) {
    const uint32_t* t = Crc32Tab().t;
    for (size_t i = 0; i < len; ++i)
        state = t[(state ^ data[i]) & 0xFFu] ^ (state >> 8);
    return state;
}
inline uint32_t Crc32Final(uint32_t state) { return state ^ 0xFFFFFFFFu; }
inline uint32_t Crc32(const uint8_t* data, size_t len) {
    return Crc32Final(Crc32Update(Crc32Init(), data, len));
}
