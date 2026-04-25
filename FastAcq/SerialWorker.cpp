#include "pch.h"
#include "SerialWorker.h"
#include "AppMessages.h"

#include <process.h>

// ---------------------------------------------------------------------------
static CString TimeStampNow()
{
    SYSTEMTIME st;
    ::GetLocalTime(&st);
    CString s;
    s.Format(_T("%02d:%02d:%02d.%03d"), st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    return s;
}

SerialWorker::SerialWorker(ChirpStore& store, HWND targetHwnd)
    : m_store(store), m_hwnd(targetHwnd)
{
    ::InitializeCriticalSection(&m_writeCs);

    m_parser.SetCallback([this](ChirpFrame&& f) {
        // Capture header info before the frame is moved into the store.
        const uint32_t frameId  = f.header.frame_id;
        const uint32_t tsMs     = f.header.timestamp_ms;
        const uint16_t peakBin  = f.header.fft_peak_bin;
        const size_t   nSamples = f.raw.size();

        size_t idx = m_store.Push(std::move(f));
        if (::IsWindow(m_hwnd))
            ::PostMessage(m_hwnd, WM_APP_FRAME_READY, static_cast<WPARAM>(idx), 0);

        CString line;
        line.Format(_T("[RX] %s  frame_id=%-5u  ts=%u ms  peak_bin=%-4u  samples=%zu"),
                    TimeStampNow().GetString(), frameId, tsMs, peakBin, nSamples);
        PostCommLog(line);
    });
}

SerialWorker::~SerialWorker()
{
    Close();
    ::DeleteCriticalSection(&m_writeCs);
}

bool SerialWorker::Open(LPCTSTR portName, DWORD baud)
{
    Close();

    // Use \\.\COMxx form to handle COM10+.
    CString path;
    path.Format(_T("\\\\.\\%s"), portName);

    m_hPort = ::CreateFile(path,
                           GENERIC_READ | GENERIC_WRITE,
                           0, nullptr, OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL, nullptr);
    if (m_hPort == INVALID_HANDLE_VALUE)
        return false;

    DCB dcb{}; dcb.DCBlength = sizeof(dcb);
    if (!::GetCommState(m_hPort, &dcb)) {
        Close(); return false;
    }
    dcb.BaudRate = baud;
    dcb.ByteSize = 8;
    dcb.Parity   = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.fBinary  = TRUE;
    dcb.fParity  = FALSE;
    dcb.fOutxCtsFlow = FALSE;
    dcb.fOutxDsrFlow = FALSE;
    dcb.fDtrControl  = DTR_CONTROL_ENABLE;
    dcb.fRtsControl  = RTS_CONTROL_ENABLE;
    dcb.fInX = dcb.fOutX = FALSE;
    if (!::SetCommState(m_hPort, &dcb)) {
        Close(); return false;
    }

    COMMTIMEOUTS to{};
    // Return from ReadFile after 100ms if no data -- lets us check m_quit.
    to.ReadIntervalTimeout         = MAXDWORD;
    to.ReadTotalTimeoutMultiplier  = MAXDWORD;
    to.ReadTotalTimeoutConstant    = 100;
    to.WriteTotalTimeoutMultiplier = 0;
    to.WriteTotalTimeoutConstant   = 1000;
    ::SetCommTimeouts(m_hPort, &to);

    ::SetupComm(m_hPort, 1 << 16, 1 << 12);
    ::PurgeComm(m_hPort, PURGE_RXCLEAR | PURGE_TXCLEAR);

    m_parser.Reset();
    InterlockedExchange(&m_quit, 0);

    unsigned tid = 0;
    m_hThread = reinterpret_cast<HANDLE>(
        _beginthreadex(nullptr, 0, ThreadProc, this, 0, &tid));
    if (!m_hThread) {
        Close(); return false;
    }

    if (::IsWindow(m_hwnd))
        ::PostMessage(m_hwnd, WM_APP_PORT_STATUS, 1, 0);

    CString openMsg;
    openMsg.Format(_T("[PORT] %s  Connected: %s @ %lu baud"),
                   TimeStampNow().GetString(), portName, baud);
    PostCommLog(openMsg);
    return true;
}

void SerialWorker::Close()
{
    InterlockedExchange(&m_quit, 1);

    if (m_hPort != INVALID_HANDLE_VALUE) {
        // Break any blocking ReadFile.
        ::CancelIoEx(m_hPort, nullptr);
    }

    if (m_hThread) {
        ::WaitForSingleObject(m_hThread, 2000);
        ::CloseHandle(m_hThread);
        m_hThread = nullptr;
    }

    if (m_hPort != INVALID_HANDLE_VALUE) {
        ::CloseHandle(m_hPort);
        m_hPort = INVALID_HANDLE_VALUE;
        if (::IsWindow(m_hwnd))
            ::PostMessage(m_hwnd, WM_APP_PORT_STATUS, 0, 0);
        CString closeMsg;
        closeMsg.Format(_T("[PORT] %s  Disconnected"), TimeStampNow().GetString());
        PostCommLog(closeMsg);
    }
}

bool SerialWorker::SendCommand(uint8_t cmd, uint16_t a1, uint16_t a2, uint16_t a3)
{
    if (m_hPort == INVALID_HANDLE_VALUE) return false;

    ProtocolCmd c{};
    c.cmd  = cmd;
    c.arg1 = a1;
    c.arg2 = a2;
    c.arg3 = a3;
    c.crc8 = Crc8(reinterpret_cast<const uint8_t*>(&c),
                  sizeof(c) - sizeof(c.crc8));

    DWORD written = 0;
    ::EnterCriticalSection(&m_writeCs);
    BOOL ok = ::WriteFile(m_hPort, &c, sizeof(c), &written, nullptr);
    ::LeaveCriticalSection(&m_writeCs);

    if (ok && written == sizeof(c)) {
        CString line;
        line.Format(_T("[TX]  %s  cmd=0x%02X  arg1=0x%04X  arg2=0x%04X  arg3=0x%04X"),
                    TimeStampNow().GetString(), cmd, a1, a2, a3);
        PostCommLog(line);
        return true;
    }
    return false;
}

void SerialWorker::PostCommLog(const CString& line)
{
    if (!::IsWindow(m_hwnd)) return;
    // Heap-allocate; receiver (OnCommLog) must delete.
    CString* p = new CString(line);
    if (!::PostMessage(m_hwnd, WM_APP_COMM_LOG, 0, reinterpret_cast<LPARAM>(p)))
        delete p;
}

UINT __stdcall SerialWorker::ThreadProc(LPVOID p)
{
    static_cast<SerialWorker*>(p)->ThreadLoop();
    return 0;
}

void SerialWorker::ThreadLoop()
{
    uint8_t buf[4096];
    uint64_t bytesTotal   = 0;
    uint64_t bytesLastPost = 0;
    DWORD    lastPost      = ::GetTickCount();
    uint64_t lastBadCrcLogged = 0;

    while (!InterlockedCompareExchange(const_cast<LONG*>(&m_quit), 0, 0)) {
        DWORD nRead = 0;
        BOOL ok = ::ReadFile(m_hPort, buf, sizeof(buf), &nRead, nullptr);
        if (!ok) {
            DWORD err = ::GetLastError();
            if (err == ERROR_OPERATION_ABORTED) break;      // CancelIoEx
            if (err == ERROR_ACCESS_DENIED ||
                err == ERROR_DEVICE_NOT_CONNECTED ||
                err == ERROR_BAD_COMMAND) break;            // device unplugged
            ::ClearCommError(m_hPort, nullptr, nullptr);
            ::Sleep(10);
            continue;
        }
        if (nRead > 0) {
            bytesTotal += nRead;
            m_parser.Feed(buf, nRead);
        }

        // Periodically publish parse stats + byte counters so the user sees
        // whether any bytes arrive at all, even if the parser hasn't
        // assembled a complete frame yet.
        DWORD now = ::GetTickCount();
        if (now - lastPost > 500) {
            if (::IsWindow(m_hwnd)) {
                ::PostMessage(m_hwnd, WM_APP_PARSE_STATS,
                    static_cast<WPARAM>(m_parser.FramesOk()),
                    static_cast<LPARAM>(m_parser.FramesBadCrc()));
            }
            if (bytesTotal != bytesLastPost) {
                CString line;
                line.Format(_T("[RX bytes] %s  total=%llu  delta=%llu  parsed_ok=%llu  bad_crc=%llu  dropped=%llu"),
                            TimeStampNow().GetString(),
                            static_cast<unsigned long long>(bytesTotal),
                            static_cast<unsigned long long>(bytesTotal - bytesLastPost),
                            static_cast<unsigned long long>(m_parser.FramesOk()),
                            static_cast<unsigned long long>(m_parser.FramesBadCrc()),
                            static_cast<unsigned long long>(m_parser.BytesDropped()));
                PostCommLog(line);
                if (m_parser.FramesBadCrc() != lastBadCrcLogged) {
                    lastBadCrcLogged = m_parser.FramesBadCrc();
                    CString d;
                    d.Format(_T("[CRC-DIAG] frame_id=%u  raw=%zuB  fft=%zuB  calc=0x%08X  rx=0x%08X"),
                             m_parser.LastFrameId(),
                             m_parser.LastRawLen(),
                             m_parser.LastFftLen(),
                             m_parser.LastCrcCalc(),
                             m_parser.LastCrcRx());
                    PostCommLog(d);
                }
                bytesLastPost = bytesTotal;
            }
            lastPost = now;
        }
    }

    if (::IsWindow(m_hwnd))
        ::PostMessage(m_hwnd, WM_APP_PORT_STATUS, 0, 0);
}

std::vector<CString> SerialWorker::EnumPorts()
{
    std::vector<CString> out;
    HKEY hKey = nullptr;
    if (::RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                       _T("HARDWARE\\DEVICEMAP\\SERIALCOMM"),
                       0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return out;

    for (DWORD i = 0;; ++i) {
        TCHAR name[256]; DWORD nameLen = 256;
        TCHAR value[256]; DWORD valueLen = sizeof(value);
        DWORD type = 0;
        LONG r = ::RegEnumValue(hKey, i, name, &nameLen, nullptr,
                                &type, reinterpret_cast<LPBYTE>(value), &valueLen);
        if (r == ERROR_NO_MORE_ITEMS) break;
        if (r == ERROR_SUCCESS && type == REG_SZ)
            out.emplace_back(value);
    }
    ::RegCloseKey(hKey);
    return out;
}
