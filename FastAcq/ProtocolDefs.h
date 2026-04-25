#pragma once
//
// ProtocolDefs.h
// Host-side mirror of firmware usb_protocol.h (STM32H743 Fast Acquisition Device).
// Keep layouts byte-compatible with the MCU side.
//

#include <cstdint>

// Magic at the start of every frame header.
constexpr uint32_t FRAME_MAGIC = 0xFACEDA7AUL;

// CMD opcodes (host -> MCU)
constexpr uint8_t CMD_START_CHIRP     = 0x01;
constexpr uint8_t CMD_GET_FRAME       = 0x02;
constexpr uint8_t CMD_PING            = 0x03;
constexpr uint8_t CMD_SET_SAMPLES     = 0x04;
constexpr uint8_t CMD_SET_MODE        = 0x05; // arg1 = MODE_*
constexpr uint8_t CMD_SET_DATA_MASK   = 0x06; // arg1 = bit0=RAW, bit1=FFT
constexpr uint8_t CMD_SET_INTERVAL    = 0x07; // arg1 = ms between chirps
constexpr uint8_t CMD_TRIGGER         = 0x08; // one-shot trigger
constexpr uint8_t CMD_GET_STATUS      = 0x09;

// Modes
constexpr uint16_t MODE_IDLE       = 0;
constexpr uint16_t MODE_CONTINUOUS = 1;
constexpr uint16_t MODE_SINGLE     = 2;

// data_flags bits
constexpr uint8_t FRAME_FLAG_HAS_RAW   = 1u << 0;
constexpr uint8_t FRAME_FLAG_HAS_FFT   = 1u << 1;
constexpr uint8_t FRAME_FLAG_FFT_VALID = 1u << 2;
constexpr uint8_t FRAME_FLAG_IS_STATUS = 1u << 3;

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

// CRC-32 (IEEE 802.3, reflected, init 0xFFFFFFFF, xor-out 0xFFFFFFFF).
// Matches STM32 HAL_CRC with default poly when configured in reflected mode.
// (If firmware uses hardware CRC with non-reflected mode, we'll adjust here.)
inline uint32_t Crc32(const uint8_t* data, size_t len, uint32_t seed = 0xFFFFFFFFu) {
    uint32_t crc = seed;
    for (size_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (int b = 0; b < 8; ++b) {
            crc = (crc & 1u) ? (crc >> 1) ^ 0xEDB88320u : (crc >> 1);
        }
    }
    return crc ^ 0xFFFFFFFFu;
}
