#include "Export.h"

#include <cmath>
#include <cstring>
#include <fstream>

namespace core {

namespace {
void Put32(std::ofstream& f, uint32_t v) { f.write(reinterpret_cast<const char*>(&v), 4); }
void Put16(std::ofstream& f, uint16_t v) { f.write(reinterpret_cast<const char*>(&v), 2); }
}

bool WriteWav16(const std::filesystem::path& path, const float* samples, size_t n,
                uint32_t sampleRateHz, std::string* err)
{
    std::ofstream f(path, std::ios::binary | std::ios::out | std::ios::trunc);
    if (!f) { if (err) *err = "cannot create file"; return false; }

    const uint32_t dataBytes = static_cast<uint32_t>(n * 2);
    f.write("RIFF", 4);
    Put32(f, 36 + dataBytes);
    f.write("WAVE", 4);
    f.write("fmt ", 4);
    Put32(f, 16);
    Put16(f, 1);                       // PCM
    Put16(f, 1);                       // mono
    Put32(f, sampleRateHz);
    Put32(f, sampleRateHz * 2);        // byte rate
    Put16(f, 2);                       // block align
    Put16(f, 16);                      // bits per sample
    f.write("data", 4);
    Put32(f, dataBytes);

    std::vector<int16_t> buf(n);
    for (size_t i = 0; i < n; ++i) {
        float v = samples[i];
        if (!(v == v)) v = 0.0f;
        if (v > 1.0f) v = 1.0f;
        if (v < -1.0f) v = -1.0f;
        buf[i] = static_cast<int16_t>(std::lround(v * 32767.0f));
    }
    if (n) f.write(reinterpret_cast<const char*>(buf.data()), static_cast<std::streamsize>(dataBytes));
    if (!f) { if (err) *err = "write failed"; return false; }
    return true;
}

bool WriteWav16FromCodes(const std::filesystem::path& path, const uint16_t* codes, size_t n,
                         uint32_t sampleRateHz, std::string* err)
{
    std::vector<float> s(n);
    for (size_t i = 0; i < n; ++i)
        s[i] = (static_cast<float>(codes[i] & 0x0FFFu) - 2048.0f) / 2048.0f;
    return WriteWav16(path, s.data(), n, sampleRateHz, err);
}

bool WriteFrameCsv(const std::filesystem::path& path, const ChirpFrame& f, std::string* err)
{
    std::ofstream o(path, std::ios::out | std::ios::trunc);
    if (!o) { if (err) *err = "cannot create file"; return false; }
    const FrameHeader& h = f.header;
    o << "# frame_id," << h.frame_id << "\n"
      << "# timestamp_ms," << h.timestamp_ms << "\n"
      << "# sample_rate_hz," << h.sample_rate_hz << "\n"
      << "# chirp_freq_hz," << h.chirp_freq_hz << "\n"
      << "# fft_size," << h.fft_size << "\n"
      << "# fft_peak_bin," << h.fft_peak_bin << "\n"
      << "# fft_freq_res_hz," << h.fft_freq_res_hz << "\n"
      << "index,raw,fft_mag\n";
    const size_t n = f.raw.size() > f.fft.size() ? f.raw.size() : f.fft.size();
    for (size_t i = 0; i < n; ++i) {
        o << i << ',';
        if (i < f.raw.size()) o << f.raw[i];
        o << ',';
        if (i < f.fft.size()) o << f.fft[i];
        o << '\n';
    }
    if (!o) { if (err) *err = "write failed"; return false; }
    return true;
}

} // namespace core
