#include "ProtocolParser.h"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstring>

// Reasonable upper bounds to protect against corrupt headers.
static constexpr uint32_t kMaxRawBytes = 16u * 1024u * 1024u; // 16 MiB
static constexpr uint32_t kMaxFftBytes = 16u * 1024u * 1024u;

ProtocolParser::ProtocolParser(FrameCallback cb)
    : m_cb(std::move(cb))
{
}

uint32_t ProtocolParser::NowMs()
{
    using namespace std::chrono;
    return static_cast<uint32_t>(
        duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}

void ProtocolParser::ResetStream()
{
    m_state        = State::WaitMagic;
    m_magicShift   = 0;
    m_hdrBytesRead = 0;
    m_hdrExpected  = 0;
    m_hasExt       = false;
    m_rawBuf.clear();
    m_fftBuf.clear();
    m_rawNeeded    = 0;
    m_fftNeeded    = 0;
    m_crcBytesRead = 0;
    m_crcState     = 0;
}

void ProtocolParser::Reset()
{
    ResetStream();
    m_bytesDropped    = 0;
    m_bytesTotal      = 0;
    m_framesOk        = 0;
    m_framesBadCrc    = 0;
    m_framesBadHeader = 0;
    m_framesLost      = 0;
    m_haveLastDataId  = false;
    m_lastDataFrameId = 0;
}

void ProtocolParser::Feed(const uint8_t* data, size_t len)
{
    m_bytesTotal += len;
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
    // Slide a 32-bit little-endian window looking for a v1 or v2 magic.
    m_magicShift = (m_magicShift >> 8) | (static_cast<uint32_t>(b) << 24);
    m_bytesDropped++;
    if (m_magicShift == FRAME_MAGIC_V1 || m_magicShift == FRAME_MAGIC_V2) {
        // Magic bytes consumed; seed the header bytes with them and read the remainder.
        std::memset(&m_hdr, 0, sizeof(m_hdr));
        std::memset(&m_ext, 0, sizeof(m_ext));
        std::memcpy(m_hdrBytes, &m_magicShift, sizeof(uint32_t));
        m_hasExt       = (m_magicShift == FRAME_MAGIC_V2);
        m_hdrExpected  = m_hasExt ? kHeaderV2Size : kHeaderV1Size;
        m_hdrBytesRead = sizeof(uint32_t);
        m_state        = State::ReadHeader;
        m_bytesDropped -= 4; // those 4 bytes are part of a valid frame
        m_magicShift   = 0;
    }
}

void ProtocolParser::HandleReadHeader(const uint8_t* data, size_t& i, size_t len)
{
    size_t need  = m_hdrExpected - m_hdrBytesRead;
    size_t avail = len - i;
    size_t take  = (std::min)(need, avail);
    std::memcpy(m_hdrBytes + m_hdrBytesRead, data + i, take);
    m_hdrBytesRead += take;
    i              += take;
    if (m_hdrBytesRead < m_hdrExpected) return;

    if (m_hasExt && m_hdrExpected == kHeaderV2Size) {
        // The minimum v2 header is in; header_size tells how much more follows
        // (known extension fields plus an unknown tail from newer firmware).
        uint16_t hsz = 0;
        std::memcpy(&hsz, m_hdrBytes + kHeaderV1Size + offsetof(FrameHeaderExt, header_size), sizeof(hsz));
        if (hsz < kHeaderV2Size || hsz > kHeaderMaxSize) {
            m_framesBadHeader++;
            m_bytesDropped += m_hdrExpected;
            ResetStream();
            return;
        }
        if (hsz > kHeaderV2Size) {
            m_hdrExpected = hsz;
            return;          // keep reading the remaining header bytes
        }
    }
    ValidateHeaderAndAdvance();
}

void ProtocolParser::ValidateHeaderAndAdvance()
{
    std::memcpy(&m_hdr, m_hdrBytes, sizeof(m_hdr));
    if (m_hasExt) {
        // Copy the extension fields that were actually sent; absent ones stay 0.
        std::memset(&m_ext, 0, sizeof(m_ext));
        const size_t extBytes = (std::min)(m_hdrExpected - kHeaderV1Size, sizeof(m_ext));
        std::memcpy(&m_ext, m_hdrBytes + kHeaderV1Size, extBytes);
    }

    const bool magicOk = m_hasExt ? (m_hdr.magic == FRAME_MAGIC_V2)
                                  : (m_hdr.magic == FRAME_MAGIC_V1);
    const bool sizesOk =
        m_hdr.raw_data_bytes <= kMaxRawBytes &&
        m_hdr.fft_data_bytes <= kMaxFftBytes &&
        (m_hdr.raw_data_bytes % sizeof(uint16_t)) == 0 &&
        (m_hdr.fft_data_bytes % sizeof(float)) == 0;

    if (!magicOk || !sizesOk) {
        // Corrupt header: count it, drop and resync (statistics are kept).
        m_framesBadHeader++;
        m_bytesDropped += m_hdrExpected;
        ResetStream();
        return;
    }
    m_rawNeeded = m_hdr.raw_data_bytes;
    m_fftNeeded = m_hdr.fft_data_bytes;
    m_rawBuf.clear();
    m_fftBuf.clear();
    m_rawBuf.reserve(m_rawNeeded);
    m_fftBuf.reserve(m_fftNeeded);

    // CRC is computed over header (exactly as received, incl. any v2 extension)
    // + raw + fft (concatenated); start with the header bytes.
    m_crcState = Crc32Update(Crc32Init(), m_hdrBytes, m_hdrExpected);
    m_crcBytesRead = 0;

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
    m_crcState = Crc32Update(m_crcState, data + i, take);
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
    m_crcState = Crc32Update(m_crcState, data + i, take);
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
        m_hdrExpected  = 0;
    }
}

void ProtocolParser::FinalizeFrame()
{
    uint32_t crcRx = static_cast<uint32_t>(m_crcBuf[0])
                   | (static_cast<uint32_t>(m_crcBuf[1]) << 8)
                   | (static_cast<uint32_t>(m_crcBuf[2]) << 16)
                   | (static_cast<uint32_t>(m_crcBuf[3]) << 24);

    uint32_t crcCalc = Crc32Final(m_crcState);

    m_lastCrcCalc = crcCalc;
    m_lastCrcRx   = crcRx;
    m_lastFrameId = m_hdr.frame_id;
    m_lastRawLen  = m_rawBuf.size();
    m_lastFftLen  = m_fftBuf.size();

    if (crcCalc != crcRx) {
        m_framesBadCrc++;
        m_bytesDropped += m_hdrExpected + m_rawBuf.size() + m_fftBuf.size() + 4;
        return;
    }

    // Lost-frame accounting on data frames only (service frames use reserved ids).
    const bool isService =
        (m_hdr.data_flags & (FRAME_FLAG_IS_STATUS | FRAME_FLAG_IS_ACK | FRAME_FLAG_IS_TRACE)) != 0 ||
        m_hdr.frame_id >= FRAME_ID_TRACE;
    if (!isService) {
        if (m_haveLastDataId && m_hdr.frame_id > m_lastDataFrameId + 1)
            m_framesLost += (m_hdr.frame_id - m_lastDataFrameId - 1);
        m_haveLastDataId  = true;
        m_lastDataFrameId = m_hdr.frame_id;
    }

    ChirpFrame f;
    f.header     = m_hdr;
    f.hasExt     = m_hasExt;
    if (m_hasExt) f.ext = m_ext;
    f.rx_tick_ms = NowMs();
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
