#include "SessionFile.h"

#include <cstring>

namespace core {

static constexpr uint32_t kMaxSectionBytes = 64u * 1024u * 1024u;

bool SessionWriter::Open(const std::filesystem::path& path, uint32_t sampleRateHz,
                         uint64_t createdUnixMs, const std::string& note)
{
    Close();
    m_err.clear();
    m_f.open(path, std::ios::binary | std::ios::out | std::ios::trunc);
    if (!m_f) { m_err = "cannot create file"; return false; }

    SessionFileHeader h{};
    std::memcpy(h.magic, kFileMagic, sizeof(h.magic));
    h.version       = 1;
    h.headerSize    = sizeof(SessionFileHeader);
    h.createdUnixMs = createdUnixMs;
    h.sampleRateHz  = sampleRateHz;
    const size_t n = note.size() < sizeof(h.note) - 1 ? note.size() : sizeof(h.note) - 1;
    if (n) std::memcpy(h.note, note.data(), n);   // h.note is zero-initialized -> always terminated
    m_f.write(reinterpret_cast<const char*>(&h), sizeof(h));
    if (!m_f) { m_err = "write failed"; Close(); return false; }
    m_bytes  = sizeof(h);
    m_frames = 0;
    return true;
}

bool SessionWriter::Write(const ChirpFrame& f)
{
    if (!m_f.is_open()) return false;
    SessionRecordHeader rh{};
    rh.magic    = kRecordMagic;
    rh.rawBytes = static_cast<uint32_t>(f.raw.size() * sizeof(uint16_t));
    rh.fftBytes = static_cast<uint32_t>(f.fft.size() * sizeof(float));
    rh.rxTickMs = f.rx_tick_ms;
    rh.hdr      = f.header;
    m_f.write(reinterpret_cast<const char*>(&rh), sizeof(rh));
    if (rh.rawBytes) m_f.write(reinterpret_cast<const char*>(f.raw.data()), rh.rawBytes);
    if (rh.fftBytes) m_f.write(reinterpret_cast<const char*>(f.fft.data()), rh.fftBytes);
    if (!m_f) { m_err = "write failed"; return false; }
    m_bytes += sizeof(rh) + rh.rawBytes + rh.fftBytes;
    m_frames++;
    return true;
}

void SessionWriter::Close()
{
    if (m_f.is_open()) {
        m_f.flush();
        m_f.close();
    }
}

// ---------------------------------------------------------------------------
bool SessionReader::Open(const std::filesystem::path& path)
{
    Close();
    m_err.clear();
    m_f.open(path, std::ios::binary | std::ios::in);
    if (!m_f) { m_err = "cannot open file"; return false; }

    m_f.read(reinterpret_cast<char*>(&m_hdr), sizeof(m_hdr));
    if (!m_f || std::memcmp(m_hdr.magic, kFileMagic, sizeof(kFileMagic)) != 0) {
        m_err = "not a FastAcq session file";
        Close();
        return false;
    }
    if (m_hdr.headerSize < sizeof(SessionFileHeader)) {
        m_err = "bad header size";
        Close();
        return false;
    }

    // Build the index by scanning record headers (robust to truncated tails).
    uint64_t pos = m_hdr.headerSize;
    m_f.seekg(0, std::ios::end);
    const uint64_t end = static_cast<uint64_t>(m_f.tellg());
    while (pos + sizeof(SessionRecordHeader) <= end) {
        m_f.seekg(static_cast<std::streamoff>(pos));
        SessionRecordHeader rh{};
        m_f.read(reinterpret_cast<char*>(&rh), sizeof(rh));
        if (!m_f || rh.magic != kRecordMagic) break;
        if (rh.rawBytes > kMaxSectionBytes || rh.fftBytes > kMaxSectionBytes) break;
        const uint64_t next = pos + sizeof(rh) + rh.rawBytes + rh.fftBytes;
        if (next > end) break;                      // truncated record
        m_offsets.push_back(pos);
        m_ts.push_back(rh.hdr.timestamp_ms);
        m_ids.push_back(rh.hdr.frame_id);
        pos = next;
    }
    m_f.clear();
    return true;
}

void SessionReader::Close()
{
    if (m_f.is_open()) m_f.close();
    m_offsets.clear();
    m_ts.clear();
    m_ids.clear();
}

bool SessionReader::Read(size_t index, ChirpFrame& out)
{
    if (!m_f.is_open() || index >= m_offsets.size()) return false;
    m_f.clear();
    m_f.seekg(static_cast<std::streamoff>(m_offsets[index]));
    SessionRecordHeader rh{};
    m_f.read(reinterpret_cast<char*>(&rh), sizeof(rh));
    if (!m_f || rh.magic != kRecordMagic) return false;
    out = ChirpFrame{};
    out.header     = rh.hdr;
    out.rx_tick_ms = rh.rxTickMs;
    out.raw.resize(rh.rawBytes / sizeof(uint16_t));
    out.fft.resize(rh.fftBytes / sizeof(float));
    if (rh.rawBytes) m_f.read(reinterpret_cast<char*>(out.raw.data()), rh.rawBytes);
    if (rh.fftBytes) m_f.read(reinterpret_cast<char*>(out.fft.data()), rh.fftBytes);
    return static_cast<bool>(m_f);
}

} // namespace core
