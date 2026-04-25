#pragma once
//
// ProtocolParser.h
// Byte-stream state machine that reconstructs frames from the COM port.
// Usage: feed bytes via Feed(); on complete+valid frame, OnFrame callback fires.
//

#include "pch.h"
#include "ProtocolDefs.h"
#include "ChirpStore.h"

#include <functional>

class ProtocolParser {
public:
    using FrameCallback = std::function<void(ChirpFrame&&)>;

    explicit ProtocolParser(FrameCallback cb = nullptr);

    void SetCallback(FrameCallback cb) { m_cb = std::move(cb); }

    // Feed raw bytes received from serial port.
    void Feed(const uint8_t* data, size_t len);

    // Drop any partial frame state.
    void Reset();

    // Diagnostics
    uint64_t FramesOk()      const { return m_framesOk; }
    uint64_t FramesBadCrc()  const { return m_framesBadCrc; }
    uint64_t BytesDropped()  const { return m_bytesDropped; }

    // Last-frame CRC diagnostics (valid after at least one FinalizeFrame).
    uint32_t LastCrcCalc()  const { return m_lastCrcCalc; }
    uint32_t LastCrcRx()    const { return m_lastCrcRx; }
    uint32_t LastFrameId()  const { return m_lastFrameId; }
    size_t   LastRawLen()   const { return m_lastRawLen; }
    size_t   LastFftLen()   const { return m_lastFftLen; }

private:
    enum class State {
        WaitMagic,
        ReadHeader,
        ReadRaw,
        ReadFft,
        ReadCrc32
    };

    void HandleWaitMagic(uint8_t b);
    void HandleReadHeader(const uint8_t* data, size_t& i, size_t len);
    void HandleReadRaw(const uint8_t* data, size_t& i, size_t len);
    void HandleReadFft(const uint8_t* data, size_t& i, size_t len);
    void HandleReadCrc(const uint8_t* data, size_t& i, size_t len);
    void FinalizeFrame();
    void ValidateHeaderAndAdvance();

    State          m_state{State::WaitMagic};
    uint32_t       m_magicShift{0};
    FrameHeader    m_hdr{};
    size_t         m_hdrBytesRead{0};
    std::vector<uint8_t> m_rawBuf;      // raw bytes of raw section (uint16 samples)
    std::vector<uint8_t> m_fftBuf;      // raw bytes of fft section (float32)
    size_t         m_rawNeeded{0};
    size_t         m_fftNeeded{0};
    uint8_t        m_crcBuf[4]{};
    size_t         m_crcBytesRead{0};

    FrameCallback  m_cb;

    uint64_t       m_framesOk{0};
    uint64_t       m_framesBadCrc{0};
    uint64_t       m_bytesDropped{0};

    // Diagnostics: last frame seen (ok or bad).
    uint32_t       m_lastCrcCalc{0};
    uint32_t       m_lastCrcRx{0};
    uint32_t       m_lastFrameId{0};
    size_t         m_lastRawLen{0};
    size_t         m_lastFftLen{0};
};
