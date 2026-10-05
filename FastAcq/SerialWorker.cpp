#include "pch.h"
#include "SerialWorker.h"

#include <process.h>
#include <initguid.h>   // must precede devguid.h: defines GUID_DEVCLASS_PORTS in this TU
#include <devguid.h>
#include <setupapi.h>
#pragma comment(lib, "setupapi.lib")

// USB hardware id of the FastAcq Acquisition Device.
// Must match firmware Core/Inc/usbd_desc.h (USBD_VID / USBD_PID_FS).
static constexpr LPCTSTR kFastAcqHwId = _T("VID_0483&PID_5FA1");

SerialWorker::SerialWorker(ChirpStore& store, HWND targetHwnd)
    : m_store(store), m_hwnd(targetHwnd)
{
    ::InitializeCriticalSection(&m_writeCs);

    m_parser.SetCallback([this](ChirpFrame&& f) {
        const FrameHeader& h = f.header;

        // Service frames (PONG / STATUS / ACK) are routed to the UI directly
        // and never enter the ChirpStore -- they carry no capture data.
        const bool isService =
            (h.data_flags & (FRAME_FLAG_IS_STATUS | FRAME_FLAG_IS_ACK)) != 0 ||
            h.frame_id >= FRAME_ID_ACK;
        if (isService) {
            WPARAM type = SVC_FRAME_PONG;
            if ((h.data_flags & FRAME_FLAG_IS_STATUS) || h.frame_id == FRAME_ID_STATUS)
                type = SVC_FRAME_STATUS;
            else if ((h.data_flags & FRAME_FLAG_IS_ACK) || h.frame_id == FRAME_ID_ACK)
                type = SVC_FRAME_ACK;
            if (::IsWindow(m_hwnd)) {
                auto* pf = new ChirpFrame(std::move(f));
                if (!::PostMessage(m_hwnd, WM_APP_SERVICE_FRAME, type, reinterpret_cast<LPARAM>(pf)))
                    delete pf;
            }
            return;
        }

        const uint32_t frameId  = h.frame_id;
        const uint32_t tsMs     = h.timestamp_ms;
        const size_t   nSamples = f.raw.size();
        const size_t   nFft     = f.fft.size();

        const uint64_t seq = m_store.Push(std::move(f));
        if (::IsWindow(m_hwnd))
            ::PostMessage(m_hwnd, WM_APP_FRAME_READY, static_cast<WPARAM>(seq), 0);

        if (m_verbose.load()) {
            CString line;
            line.Format(_T("frame_id=%u  ts=%u ms  raw=%zu  fft=%zu  seq=%llu"),
                        frameId, tsMs, nSamples, nFft, static_cast<unsigned long long>(seq));
            PostCommLog(LOG_RX, line);
        }
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

    m_hPort = ::CreateFile(path, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL, nullptr);
    if (m_hPort == INVALID_HANDLE_VALUE) {
        CString err; err.Format(_T("Cannot open %s (error %lu)"), portName, ::GetLastError());
        PostCommLog(LOG_ERR, err);
        return false;
    }

    DCB dcb{}; dcb.DCBlength = sizeof(dcb);
    if (!::GetCommState(m_hPort, &dcb)) { Close(); return false; }
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
    if (!::SetCommState(m_hPort, &dcb)) { Close(); return false; }

    COMMTIMEOUTS to{};
    // Return from ReadFile after 100ms if no data -- lets us check m_quit.
    to.ReadIntervalTimeout         = MAXDWORD;
    to.ReadTotalTimeoutMultiplier  = MAXDWORD;
    to.ReadTotalTimeoutConstant    = 100;
    to.WriteTotalTimeoutMultiplier = 0;
    to.WriteTotalTimeoutConstant   = 1000;
    ::SetCommTimeouts(m_hPort, &to);

    ::SetupComm(m_hPort, 1 << 18, 1 << 12);
    ::PurgeComm(m_hPort, PURGE_RXCLEAR | PURGE_TXCLEAR);

    m_parser.Reset();
    InterlockedExchange(&m_quit, 0);

    unsigned tid = 0;
    m_hThread = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, ThreadProc, this, 0, &tid));
    if (!m_hThread) { Close(); return false; }

    if (::IsWindow(m_hwnd))
        ::PostMessage(m_hwnd, WM_APP_PORT_STATUS, 1, 0);

    CString openMsg;
    openMsg.Format(_T("Connected: %s @ %lu baud"), portName, baud);
    PostCommLog(LOG_PORT, openMsg);
    return true;
}

void SerialWorker::Close()
{
    InterlockedExchange(&m_quit, 1);

    if (m_hPort != INVALID_HANDLE_VALUE) {
        ::CancelIoEx(m_hPort, nullptr);   // break any blocking ReadFile
    }

    if (m_hThread) {
        // The read loop returns within ~100 ms (ReadFile timeout); wait for it
        // before touching the handle so the thread never uses a closed port.
        ::WaitForSingleObject(m_hThread, INFINITE);
        ::CloseHandle(m_hThread);
        m_hThread = nullptr;
    }

    if (m_hPort != INVALID_HANDLE_VALUE) {
        ::CloseHandle(m_hPort);
        m_hPort = INVALID_HANDLE_VALUE;
        if (::IsWindow(m_hwnd))
            ::PostMessage(m_hwnd, WM_APP_PORT_STATUS, 0, 0);
        PostCommLog(LOG_PORT, _T("Disconnected"));
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
    c.crc8 = Crc8(reinterpret_cast<const uint8_t*>(&c), sizeof(c) - sizeof(c.crc8));

    DWORD written = 0;
    ::EnterCriticalSection(&m_writeCs);
    BOOL ok = ::WriteFile(m_hPort, &c, sizeof(c), &written, nullptr);
    ::LeaveCriticalSection(&m_writeCs);

    CString line;
    if (ok && written == sizeof(c)) {
        line.Format(_T("cmd=0x%02X  arg1=%u (0x%04X)  arg2=%u  arg3=%u"), cmd, a1, a1, a2, a3);
        PostCommLog(LOG_TX, line);
        return true;
    }
    line.Format(_T("WriteFile failed for cmd=0x%02X (error %lu)"), cmd, ::GetLastError());
    PostCommLog(LOG_ERR, line);
    return false;
}

void SerialWorker::PostCommLog(LogKind kind, const CString& line)
{
    if (!::IsWindow(m_hwnd)) return;
    CString* p = new CString(line);
    if (!::PostMessage(m_hwnd, WM_APP_COMM_LOG, kind, reinterpret_cast<LPARAM>(p)))
        delete p;
}

UINT __stdcall SerialWorker::ThreadProc(LPVOID p)
{
    static_cast<SerialWorker*>(p)->ThreadLoop();
    return 0;
}

void SerialWorker::ThreadLoop()
{
    std::vector<uint8_t> buf(1 << 16);
    uint64_t bytesLastPost   = 0;
    uint64_t framesLastPost  = 0;
    uint64_t lastBadCrcLogged = 0;
    uint64_t lastBadHdrLogged = 0;
    uint64_t lastLostLogged   = 0;
    DWORD    lastPost = ::GetTickCount();

    while (!InterlockedCompareExchange(const_cast<LONG*>(&m_quit), 0, 0)) {
        DWORD nRead = 0;
        BOOL ok = ::ReadFile(m_hPort, buf.data(), static_cast<DWORD>(buf.size()), &nRead, nullptr);
        if (!ok) {
            DWORD err = ::GetLastError();
            if (err == ERROR_OPERATION_ABORTED) break;      // CancelIoEx
            if (err == ERROR_ACCESS_DENIED ||
                err == ERROR_DEVICE_NOT_CONNECTED ||
                err == ERROR_BAD_COMMAND) {                 // device unplugged
                PostCommLog(LOG_ERR, _T("Device disconnected (read error)"));
                break;
            }
            ::ClearCommError(m_hPort, nullptr, nullptr);
            ::Sleep(10);
            continue;
        }
        if (nRead > 0) m_parser.Feed(buf.data(), nRead);

        // Periodic link statistics (rates over the last interval).
        DWORD now = ::GetTickCount();
        if (now - lastPost >= 500) {
            const double dt = (now - lastPost) / 1000.0;
            auto* st = new LinkStats();
            st->framesOk        = m_parser.FramesOk();
            st->framesBadCrc    = m_parser.FramesBadCrc();
            st->framesBadHeader = m_parser.FramesBadHeader();
            st->framesLost      = m_parser.FramesLost();
            st->bytesTotal      = m_parser.BytesTotal();
            st->bytesDropped    = m_parser.BytesDropped();
            st->bytesPerSec     = (st->bytesTotal - bytesLastPost) / dt;
            st->framesPerSec    = (st->framesOk - framesLastPost) / dt;
            bytesLastPost  = st->bytesTotal;
            framesLastPost = st->framesOk;
            if (!::IsWindow(m_hwnd) || !::PostMessage(m_hwnd, WM_APP_LINK_STATS, 0, reinterpret_cast<LPARAM>(st)))
                delete st;

            if (m_parser.FramesBadCrc() != lastBadCrcLogged) {
                lastBadCrcLogged = m_parser.FramesBadCrc();
                CString d;
                d.Format(_T("CRC mismatch: frame_id=%u raw=%zuB fft=%zuB calc=0x%08X rx=0x%08X (total %llu)"),
                         m_parser.LastFrameId(), m_parser.LastRawLen(), m_parser.LastFftLen(),
                         m_parser.LastCrcCalc(), m_parser.LastCrcRx(),
                         static_cast<unsigned long long>(lastBadCrcLogged));
                PostCommLog(LOG_ERR, d);
            }
            if (m_parser.FramesBadHeader() != lastBadHdrLogged) {
                lastBadHdrLogged = m_parser.FramesBadHeader();
                CString d; d.Format(_T("Bad frame header (resync), total %llu"), static_cast<unsigned long long>(lastBadHdrLogged));
                PostCommLog(LOG_ERR, d);
            }
            if (m_parser.FramesLost() != lastLostLogged) {
                lastLostLogged = m_parser.FramesLost();
                CString d; d.Format(_T("Frame id gap: %llu frames lost in total"), static_cast<unsigned long long>(lastLostLogged));
                PostCommLog(LOG_ERR, d);
            }
            lastPost = now;
        }
    }

    if (::IsWindow(m_hwnd))
        ::PostMessage(m_hwnd, WM_APP_PORT_STATUS, 0, 0);
}

std::vector<CString> SerialWorker::EnumPorts()
{
    // Enumerate only COM ports that belong to the FastAcq Acquisition Device
    // (matched by USB VID/PID from the device hardware id via SetupAPI).
    std::vector<CString> out;

    HDEVINFO hDevInfo = ::SetupDiGetClassDevs(&GUID_DEVCLASS_PORTS, nullptr, nullptr, DIGCF_PRESENT);
    if (hDevInfo == INVALID_HANDLE_VALUE)
        return out;

    SP_DEVINFO_DATA did{};
    did.cbSize = sizeof(did);
    for (DWORD i = 0; ::SetupDiEnumDeviceInfo(hDevInfo, i, &did); ++i) {
        TCHAR hwid[512]{};
        if (!::SetupDiGetDeviceRegistryProperty(hDevInfo, &did, SPDRP_HARDWAREID, nullptr,
                                                reinterpret_cast<PBYTE>(hwid), sizeof(hwid) - sizeof(TCHAR), nullptr))
            continue;

        CString id(hwid);   // first string of the REG_MULTI_SZ list
        id.MakeUpper();
        if (id.Find(kFastAcqHwId) < 0)
            continue;

        HKEY hKey = ::SetupDiOpenDevRegKey(hDevInfo, &did, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
        if (hKey == INVALID_HANDLE_VALUE)
            continue;
        TCHAR port[64]{};
        DWORD sz = sizeof(port), type = 0;
        if (::RegQueryValueEx(hKey, _T("PortName"), nullptr, &type, reinterpret_cast<LPBYTE>(port), &sz) == ERROR_SUCCESS &&
            type == REG_SZ && _tcsncmp(port, _T("COM"), 3) == 0)
        {
            out.emplace_back(port);
        }
        ::RegCloseKey(hKey);
    }
    ::SetupDiDestroyDeviceInfoList(hDevInfo);
    return out;
}
