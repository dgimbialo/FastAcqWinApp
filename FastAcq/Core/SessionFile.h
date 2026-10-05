#pragma once
//
// SessionFile.h -- ".facq" raw session recording: a file header followed by
// a sequence of frame records (record header + FrameHeader + raw + fft).
// Portable (std::fstream); used by the recorder, the replay player and tests.
//
//   SessionFileHeader (96 bytes)
//   repeat:
//     SessionRecordHeader (72 bytes) = {magic, rawBytes, fftBytes, rxTickMs, FrameHeader}
//     raw   [rawBytes]  (uint16 LE samples)
//     fft   [fftBytes]  (float32 LE magnitudes)
//

#include "../ChirpStore.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace core {

#pragma pack(push, 1)
struct SessionFileHeader {
    char     magic[8];        // "FACQSES1"
    uint32_t version;         // 1
    uint32_t headerSize;      // sizeof(SessionFileHeader)
    uint64_t createdUnixMs;   // wall-clock at creation
    uint32_t sampleRateHz;    // calibrated ADC rate at recording time
    uint32_t reserved;
    char     note[64];        // free text (UTF-8, zero padded)
};
static_assert(sizeof(SessionFileHeader) == 96, "SessionFileHeader must be 96 bytes");

struct SessionRecordHeader {
    uint32_t    magic;        // kRecordMagic
    uint32_t    rawBytes;
    uint32_t    fftBytes;
    uint32_t    rxTickMs;
    FrameHeader hdr;
};
static_assert(sizeof(SessionRecordHeader) == 72, "SessionRecordHeader must be 72 bytes");
#pragma pack(pop)

constexpr uint32_t kRecordMagic = 0x52434146u;   // 'FACR'
constexpr char     kFileMagic[8] = { 'F','A','C','Q','S','E','S','1' };

class SessionWriter {
public:
    ~SessionWriter() { Close(); }

    bool Open(const std::filesystem::path& path, uint32_t sampleRateHz,
              uint64_t createdUnixMs, const std::string& note = std::string());
    bool Write(const ChirpFrame& f);
    void Close();

    bool     IsOpen() const { return m_f.is_open(); }
    uint64_t FramesWritten() const { return m_frames; }
    uint64_t BytesWritten()  const { return m_bytes; }
    const std::string& LastError() const { return m_err; }

private:
    std::ofstream m_f;
    uint64_t      m_frames{0};
    uint64_t      m_bytes{0};
    std::string   m_err;
};

class SessionReader {
public:
    bool Open(const std::filesystem::path& path);
    void Close();

    bool   IsOpen() const { return m_f.is_open(); }
    size_t Count()  const { return m_offsets.size(); }
    const SessionFileHeader& Header() const { return m_hdr; }
    const std::string& LastError() const { return m_err; }

    // Reads record `index` into `out` (seq is left 0; the store assigns it).
    bool Read(size_t index, ChirpFrame& out);

    // MCU timestamp of record `index` (from the index built at open).
    uint32_t TimestampMs(size_t index) const { return index < m_ts.size() ? m_ts[index] : 0; }
    uint32_t FrameId(size_t index) const { return index < m_ids.size() ? m_ids[index] : 0; }

private:
    std::ifstream          m_f;
    SessionFileHeader      m_hdr{};
    std::vector<uint64_t>  m_offsets;
    std::vector<uint32_t>  m_ts;
    std::vector<uint32_t>  m_ids;
    std::string            m_err;
};

} // namespace core
