#pragma once
//
// SerialWorker.h -- background reader thread for COM port.
//
// Owns the HANDLE, runs a ReadFile loop, feeds bytes to ProtocolParser,
// pushes frames into the ChirpStore and posts WM_APP_FRAME_READY /
// WM_APP_PORT_STATUS / WM_APP_LINK_STATS / WM_APP_COMM_LOG to the target HWND.
//

#include "pch.h"
#include "AppMessages.h"
#include "ChirpStore.h"
#include "ProtocolParser.h"

#include <atomic>

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
    bool SendCommand(uint8_t cmd, uint16_t arg1 = 0, uint16_t arg2 = 0, uint16_t arg3 = 0);

    // Per-frame "[RX] ..." lines in the communication log.
    void SetVerbose(bool v) { m_verbose.store(v); }

    // Connection generation: WM_APP_PORT_STATUS carries it in lParam so the
    // frame can ignore status messages left over from a previous session.
    uint32_t Generation() const { return m_gen.load(); }

    // Enumerate COM ports of FastAcq devices ("COM1", "COM14", ...).
    static std::vector<CString> EnumPorts();

private:
    static UINT __stdcall ThreadProc(LPVOID p);
    void ThreadLoop();
    void PostCommLog(LogKind kind, const CString& line);
    void CloseHandleQuiet();   // failure path of Open(): no status / log traffic

    ChirpStore&       m_store;
    HWND              m_hwnd;
    HANDLE            m_hPort{INVALID_HANDLE_VALUE};
    HANDLE            m_hThread{nullptr};
    volatile LONG     m_quit{0};
    CRITICAL_SECTION  m_writeCs;
    ProtocolParser    m_parser;
    std::atomic<bool> m_verbose{false};
    std::atomic<uint32_t> m_gen{0};
};
