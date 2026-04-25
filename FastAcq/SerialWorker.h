#pragma once
//
// SerialWorker.h -- background reader thread for COM port.
//
// Owns the HANDLE, runs a ReadFile loop, feeds bytes to ProtocolParser.
// Posts WM_APP_FRAME_READY / WM_APP_PORT_STATUS to the supplied target HWND.
//

#include "pch.h"
#include "ProtocolParser.h"
#include "ChirpStore.h"

class SerialWorker {
public:
    SerialWorker(ChirpStore& store, HWND targetHwnd);
    ~SerialWorker();

    SerialWorker(const SerialWorker&)            = delete;
    SerialWorker& operator=(const SerialWorker&) = delete;

    // Opens the port and spawns the reader thread. Returns false on failure.
    // portName example: L"COM14"; baud is nominal (USB CDC ignores it).
    bool Open(LPCTSTR portName, DWORD baud = 921600);

    // Stop the thread and close the port.
    void Close();

    bool IsOpen() const { return m_hPort != INVALID_HANDLE_VALUE; }

    // Thread-safe: send an 8-byte host command.
    bool SendCommand(uint8_t cmd, uint16_t arg1 = 0,
                     uint16_t arg2 = 0, uint16_t arg3 = 0);

    // Diagnostics
    uint64_t FramesOk()     const { return m_parser.FramesOk(); }
    uint64_t FramesBadCrc() const { return m_parser.FramesBadCrc(); }

    // Enumerate available COM ports ("COM1", "COM14", ...).
    static std::vector<CString> EnumPorts();

private:
    static UINT __stdcall ThreadProc(LPVOID p);
    void ThreadLoop();
    void PostCommLog(const CString& line);

    ChirpStore&     m_store;
    HWND            m_hwnd;
    HANDLE          m_hPort{INVALID_HANDLE_VALUE};
    HANDLE          m_hThread{nullptr};
    volatile LONG   m_quit{0};
    CRITICAL_SECTION m_writeCs;
    ProtocolParser  m_parser;
};
