#pragma once
//
// AppMessages.h -- inter-thread and inter-window messages.
//

#include "pch.h"

constexpr UINT WM_APP_FRAME_READY  = WM_APP + 1; // wParam = logical store index
constexpr UINT WM_APP_PORT_STATUS  = WM_APP + 2; // wParam = 1 connected / 0 disconnected
constexpr UINT WM_APP_PONG_RX      = WM_APP + 3; // lParam = RTT ms
constexpr UINT WM_APP_PARSE_STATS  = WM_APP + 4; // wParam = framesOk, lParam = badCrc

// Command IDs routed from CommandPanel to MainFrame.
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
constexpr UINT WM_APP_CMD_SET_DATA_MASK = WM_APP + 21; // wParam = bit0=RAW, bit1=FFT
constexpr UINT WM_APP_CMD_SET_INTERVAL  = WM_APP + 22; // wParam = ms
constexpr UINT WM_APP_CMD_TRIGGER       = WM_APP + 23;
constexpr UINT WM_APP_CMD_GET_STATUS    = WM_APP + 24;

// Notification from ChirpListCtrl: user selected a frame.
constexpr UINT WM_APP_FRAME_SELECTED    = WM_APP + 20; // wParam = logical index

// Communication log line.  lParam = new CString* (receiver must delete).
constexpr UINT WM_APP_COMM_LOG          = WM_APP + 30;

// Acquisition mode change (from CommandPanel).  wParam = 0 RAW / 1 FFT.
constexpr UINT WM_APP_ACQ_MODE          = WM_APP + 31;

// FFT settings changed (from FftSettingsPanel). lParam = new FftSettings* (receiver deletes).
constexpr UINT WM_APP_FFT_SETTINGS      = WM_APP + 32;

// Dots display mode (from CommandPanel). wParam = 0 lines / 1 dots.
constexpr UINT WM_APP_DOTS_MODE         = WM_APP + 33;
