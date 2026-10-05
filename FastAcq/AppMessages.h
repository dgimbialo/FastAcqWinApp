#pragma once
//
// AppMessages.h -- inter-thread and inter-window messages.
//
// Pointer-carrying messages: the receiver owns and deletes the payload.
//

#include "pch.h"

// --- From worker threads to the main frame ----------------------------------
constexpr UINT WM_APP_FRAME_READY    = WM_APP + 1;  // wParam = ChirpStore sequence number
constexpr UINT WM_APP_PORT_STATUS    = WM_APP + 2;  // wParam = 1 connected / 0 disconnected
constexpr UINT WM_APP_LINK_STATS     = WM_APP + 4;  // lParam = new LinkStats* (receiver deletes)
constexpr UINT WM_APP_COMM_LOG       = WM_APP + 30; // wParam = LogKind, lParam = new CString*
constexpr UINT WM_APP_SERVICE_FRAME  = WM_APP + 37; // wParam = ServiceFrameType, lParam = new ChirpFrame*
constexpr UINT WM_APP_RESULT_READY   = WM_APP + 40; // wParam = 1 live / 0 selected, lParam = new std::shared_ptr<const dsp::FrameResult>*

// --- Commands from CommandPanel / SettingsTab to the main frame --------------
constexpr UINT WM_APP_CMD_CONNECT       = WM_APP + 10;
constexpr UINT WM_APP_CMD_DISCONNECT    = WM_APP + 11;
constexpr UINT WM_APP_CMD_START         = WM_APP + 12;
constexpr UINT WM_APP_CMD_STOP          = WM_APP + 13;
constexpr UINT WM_APP_CMD_SET_FREQ      = WM_APP + 14; // wParam = freq Hz
constexpr UINT WM_APP_CMD_SET_SAMPLES   = WM_APP + 15; // wParam = sample count
constexpr UINT WM_APP_CMD_PING          = WM_APP + 16;
constexpr UINT WM_APP_CMD_SAVE_FRAME    = WM_APP + 17;
constexpr UINT WM_APP_CMD_CLEAR         = WM_APP + 18;
constexpr UINT WM_APP_CMD_SET_MODE      = WM_APP + 19; // wParam = MODE_*
constexpr UINT WM_APP_FRAME_SELECTED    = WM_APP + 20; // wParam = sequence number
constexpr UINT WM_APP_CMD_SET_DATA_MASK = WM_APP + 21; // wParam = bit0=RAW, bit1=FFT
constexpr UINT WM_APP_CMD_SET_INTERVAL  = WM_APP + 22; // wParam = ms
constexpr UINT WM_APP_CMD_TRIGGER       = WM_APP + 23;
constexpr UINT WM_APP_CMD_GET_STATUS    = WM_APP + 24;
constexpr UINT WM_APP_CMD_SET_AMPLITUDE = WM_APP + 34; // wParam = DAC counts
constexpr UINT WM_APP_CMD_SET_BURST     = WM_APP + 35; // wParam = chirps per capture
constexpr UINT WM_APP_CMD_ABORT         = WM_APP + 36;
constexpr UINT WM_APP_REFRESH_PORTS     = WM_APP + 38;
constexpr UINT WM_APP_CMD_HOLD          = WM_APP + 41; // wParam = 1 hold / 0 live
constexpr UINT WM_APP_CMD_RECORD        = WM_APP + 42; // toggle recording
constexpr UINT WM_APP_CMD_OPEN_REPLAY   = WM_APP + 43;
constexpr UINT WM_APP_REPLAY_CTRL       = WM_APP + 44; // wParam = ReplayCommand, lParam = arg
constexpr UINT WM_APP_SETTINGS_CHANGED  = WM_APP + 45; // wParam = SettingsSource
constexpr UINT WM_APP_XRANGE_CHANGED    = WM_APP + 46; // from a plot: x-range zoom changed (sync peers)
constexpr UINT WM_APP_RESET_AVERAGES    = WM_APP + 47;

enum ServiceFrameType : WPARAM {
    SVC_FRAME_PONG   = 0,
    SVC_FRAME_STATUS = 1,
    SVC_FRAME_ACK    = 2,
};

enum LogKind : WPARAM {
    LOG_INFO = 0,
    LOG_TX,
    LOG_RX,
    LOG_SVC,
    LOG_ERR,
    LOG_PORT,
};

enum ReplayCommand : WPARAM {
    REPLAY_PLAY = 0,
    REPLAY_PAUSE,
    REPLAY_FIRST,
    REPLAY_PREV,
    REPLAY_NEXT,
    REPLAY_LAST,
    REPLAY_SEEK,     // lParam = index
    REPLAY_SPEED,    // lParam = speed * 100
    REPLAY_CLOSE,
};

enum SettingsSource : WPARAM {
    SETTINGS_FROM_TAB   = 0,  // Settings tab: everything
    SETTINGS_FROM_RADAR = 1,  // Radar tab footer: display/trace
    SETTINGS_FROM_SCOPE = 2,  // Scope tab footer: dots/units
    SETTINGS_FROM_LOG   = 3,  // Communication tab: verbose
};

// Link statistics snapshot posted by SerialWorker (heap allocated).
struct LinkStats {
    uint64_t framesOk{0};
    uint64_t framesBadCrc{0};
    uint64_t framesBadHeader{0};
    uint64_t framesLost{0};
    uint64_t bytesTotal{0};
    uint64_t bytesDropped{0};
    double   bytesPerSec{0.0};
    double   framesPerSec{0.0};
};
