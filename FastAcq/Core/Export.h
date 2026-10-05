#pragma once
//
// Export.h -- portable file exporters: WAV (16-bit PCM mono) and CSV.
//

#include "../ChirpStore.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace core {

// Normalized samples in [-1, 1] -> 16-bit PCM mono WAV.
bool WriteWav16(const std::filesystem::path& path, const float* samples, size_t n,
                uint32_t sampleRateHz, std::string* err = nullptr);

// Raw 12-bit ADC codes (mid-scale 2048) -> normalized -> WAV.
bool WriteWav16FromCodes(const std::filesystem::path& path, const uint16_t* codes, size_t n,
                         uint32_t sampleRateHz, std::string* err = nullptr);

// One frame as CSV: header comments + "index,raw,fft_mag" rows.
bool WriteFrameCsv(const std::filesystem::path& path, const ChirpFrame& f,
                   std::string* err = nullptr);

} // namespace core
