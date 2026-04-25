#include "pch.h"
#include "ProtocolParser.h"

#include <cstring>
#include <algorithm>

// Reasonable upper bounds to protect against corrupt headers.
static constexpr uint32_t kMaxRawBytes = 16u * 1024u * 1024u; // 16 MiB
static constexpr uint32_t kMaxFftBytes = 16u * 1024u * 1024u;

ProtocolParser::ProtocolParser(FrameCallback cb)
    : m_cb(std::move(cb))
{
}

void ProtocolParser::Reset()
{
    m_state        = State::WaitMagic;
    m_magicShift   = 0;
    m_hdrBytesRead = 0;
    m_rawBuf.clear();
    m_fftBuf.clear();
    m_rawNeeded    = 0;
    m_fftNeeded    = 0;
    m_crcBytesRead = 0;
    m_bytesDropped = 0;
    m_framesOk     = 0;
    m_framesBadCrc = 0;
}

void ProtocolParser::Feed(const uint8_t* data, size_t len)
{
    size_t i = 0;
    while (i < len) {
        switch (m_state) {
        case State::WaitMagic:
            HandleWaitMagic(data[i++]);
            break;
        case State::ReadHeader:
            HandleReadHeader(data, i, len);
            break;
        case State::ReadRaw:
            HandleReadRaw(data, i, len);
            break;
        case State::ReadFft:
            HandleReadFft(data, i, len);
            break;
        case State::ReadCrc32:
            HandleReadCrc(data, i, len);
            break;
        }
    }
}

void ProtocolParser::HandleWaitMagic(uint8_t b)
{
    // Slide a 32-bit little-endian window looking for FRAME_MAGIC.
    m_magicShift = (m_magicShift >> 8) | (static_cast<uint32_t>(b) << 24);
    m_bytesDropped++;
    if (m_magicShift == FRAME_MAGIC) {
        // Magic bytes consumed; seed header with them and continue reading the remainder.
        std::memset(&m_hdr, 0, sizeof(m_hdr));
        m_hdr.magic   = FRAME_MAGIC;
        m_hdrBytesRead = sizeof(uint32_t);
        m_state        = State::ReadHeader;
        m_bytesDropped -= 4; // those 4 bytes are part of a valid frame
    }
}

void ProtocolParser::HandleReadHeader(const uint8_t* data, size_t& i, size_t len)
{
    uint8_t* dst = reinterpret_cast<uint8_t*>(&m_hdr);
    size_t need  = sizeof(FrameHeader) - m_hdrBytesRead;
    size_t avail = len - i;
    size_t take  = (std::min)(need, avail);
    std::memcpy(dst + m_hdrBytesRead, data + i, take);
    m_hdrBytesRead += take;
    i              += take;
    if (m_hdrBytesRead == sizeof(FrameHeader)) {
        ValidateHeaderAndAdvance();
    }
}

void ProtocolParser::ValidateHeaderAndAdvance()
{
    if (m_hdr.magic != FRAME_MAGIC ||
        m_hdr.raw_data_bytes > kMaxRawBytes ||
        m_hdr.fft_data_bytes > kMaxFftBytes)
    {
        // Corrupt header: drop and resync.
        Reset();
        return;
    }
    m_rawNeeded = m_hdr.raw_data_bytes;
    m_fftNeeded = m_hdr.fft_data_bytes;
    m_rawBuf.clear();
    m_fftBuf.clear();
    m_rawBuf.reserve(m_rawNeeded);
    m_fftBuf.reserve(m_fftNeeded);
    if (m_rawNeeded > 0) {
        m_state = State::ReadRaw;
    } else if (m_fftNeeded > 0) {
        m_state = State::ReadFft;
    } else {
        m_state = State::ReadCrc32;
    }
}

void ProtocolParser::HandleReadRaw(const uint8_t* data, size_t& i, size_t len)
{
    size_t need  = m_rawNeeded - m_rawBuf.size();
    size_t avail = len - i;
    size_t take  = (std::min)(need, avail);
    m_rawBuf.insert(m_rawBuf.end(), data + i, data + i + take);
    i += take;
    if (m_rawBuf.size() == m_rawNeeded) {
        m_state = (m_fftNeeded > 0) ? State::ReadFft : State::ReadCrc32;
    }
}

void ProtocolParser::HandleReadFft(const uint8_t* data, size_t& i, size_t len)
{
    size_t need  = m_fftNeeded - m_fftBuf.size();
    size_t avail = len - i;
    size_t take  = (std::min)(need, avail);
    m_fftBuf.insert(m_fftBuf.end(), data + i, data + i + take);
    i += take;
    if (m_fftBuf.size() == m_fftNeeded) {
        m_state = State::ReadCrc32;
    }
}

void ProtocolParser::HandleReadCrc(const uint8_t* data, size_t& i, size_t len)
{
    size_t need  = 4 - m_crcBytesRead;
    size_t avail = len - i;
    size_t take  = (std::min)(need, avail);
    std::memcpy(m_crcBuf + m_crcBytesRead, data + i, take);
    m_crcBytesRead += take;
    i              += take;
    if (m_crcBytesRead == 4) {
        FinalizeFrame();
        m_crcBytesRead = 0;
        m_state        = State::WaitMagic;
        m_magicShift   = 0;
        m_hdrBytesRead = 0;
    }
}

void ProtocolParser::FinalizeFrame()
{
    uint32_t crcRx = static_cast<uint32_t>(m_crcBuf[0])
                   | (static_cast<uint32_t>(m_crcBuf[1]) << 8)
                   | (static_cast<uint32_t>(m_crcBuf[2]) << 16)
                   | (static_cast<uint32_t>(m_crcBuf[3]) << 24);

    // CRC is computed over header + raw + fft (concatenated).
    std::vector<uint8_t> all;
    all.reserve(sizeof(m_hdr) + m_rawBuf.size() + m_fftBuf.size());
    all.insert(all.end(), reinterpret_cast<uint8_t*>(&m_hdr),
                          reinterpret_cast<uint8_t*>(&m_hdr) + sizeof(m_hdr));
    all.insert(all.end(), m_rawBuf.begin(), m_rawBuf.end());
    all.insert(all.end(), m_fftBuf.begin(), m_fftBuf.end());
    uint32_t crcCalc = Crc32(all.data(), all.size());

    m_lastCrcCalc = crcCalc;
    m_lastCrcRx   = crcRx;
    m_lastFrameId = m_hdr.frame_id;
    m_lastRawLen  = m_rawBuf.size();
    m_lastFftLen  = m_fftBuf.size();

    if (crcCalc != crcRx) {
        m_framesBadCrc++;
        return;
    }

    ChirpFrame f;
    f.header     = m_hdr;
    f.rx_tick_ms = ::GetTickCount();
    if (!m_rawBuf.empty()) {
        size_t n = m_rawBuf.size() / sizeof(uint16_t);
        f.raw.resize(n);
        std::memcpy(f.raw.data(), m_rawBuf.data(), n * sizeof(uint16_t));
    }
    if (!m_fftBuf.empty()) {
        size_t n = m_fftBuf.size() / sizeof(float);
        f.fft.resize(n);
        std::memcpy(f.fft.data(), m_fftBuf.data(), n * sizeof(float));
    }

    m_framesOk++;
    if (m_cb) m_cb(std::move(f));
}
