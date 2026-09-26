#pragma once
#pragma once

#define IDR_MAINFRAME        128
#define IDI_APPICON          129

// CommandPanel child controls
#define IDC_CMB_COM          1001
#define IDC_BTN_CONNECT      1002
#define IDC_BTN_START        1003
#define IDC_BTN_STOP         1004
#define IDC_EDT_FREQ         1005
#define IDC_BTN_SET_FREQ     1006
#define IDC_EDT_SAMPLES      1007
#define IDC_BTN_SET_SAMPLES  1008
#define IDC_BTN_PING         1009
#define IDC_BTN_SAVE_FRAME   1010
#define IDC_BTN_CLEAR        1011
#define IDC_CMB_MODE         1012
#define IDC_BTN_APPLY_MODE   1013
#define IDC_CHK_RAW          1014
#define IDC_CHK_FFT          1015
#define IDC_BTN_APPLY_DATA   1016
#define IDC_EDT_INTERVAL     1017
#define IDC_BTN_APPLY_INT    1018
#define IDC_BTN_TRIGGER      1019
#define IDC_BTN_GET_STATUS   1020

// PC-side mode + FFT settings (row 3)
#define IDC_RDO_PC_RAW       1021
#define IDC_RDO_PC_FFT       1022
#define IDC_CMB_FFT_SIZE     1023
#define IDC_CMB_FFT_WIN      1024
#define IDC_CHK_FFT_LOG      1025
#define IDC_BTN_APPLY_FFT    1026
#define IDC_CHK_DOTS         1027

// Chirp amplitude / burst / abort controls (row 2)
#define IDC_EDT_AMPLITUDE    1028
#define IDC_BTN_SET_AMP      1029
#define IDC_EDT_BURST        1030
#define IDC_BTN_SET_BURST    1031
#define IDC_BTN_ABORT        1032

// Chirp ramp geometry (Settings tab) + follow-latest toggle (toolbar)
#define IDC_EDT_RISE         1033
#define IDC_EDT_FALL         1034
#define IDC_BTN_SET_RAMP     1035
#define IDC_CHK_FOLLOW       1036
#define IDC_CHIRP_PREVIEW    1037
// ADC clock calibration (Settings tab, PC processing)
#define IDC_EDT_PPM          1038
#define IDC_BTN_APPLY_PPM    1039

// Trace tab
#define IDC_TRC_CHK_ENABLE   1040
#define IDC_TRC_BTN_SHOT     1041
#define IDC_TRC_BTN_GET      1042
#define IDC_TRC_BTN_CLEAR    1043
#define IDC_TRC_BTN_EXPORT   1044
#define IDC_TRC_LIST         1045

// MainFrame child IDs
#define IDC_CHIRP_LIST       1100
#define IDC_TAB_CTRL         1101
#define IDC_CMD_PANEL        1102
#define IDC_TAB1_WND         1103
#define IDC_TAB2_WND         1104
#define IDC_TAB3_WND         1105
#define IDC_TAB4_WND         1106
#define IDC_TAB5_WND         1107
