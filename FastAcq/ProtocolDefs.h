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
constexpr uint32_t FRAME_MAGIC = 0xFACEDA7AUL;

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

// Reserved frame_id values for service frames (header-only)
constexpr uint32_t FRAME_ID_PONG   = 0xFFFFFFFFu;
constexpr uint32_t FRAME_ID_STATUS = 0xFFFFFFFEu;
constexpr uint32_t FRAME_ID_ACK    = 0xFFFFFFFDu;

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

#pragma pack(pop)

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
