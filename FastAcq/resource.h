#pragma once

#define IDR_MAINFRAME        128
#define IDI_APPICON          129
#define IDR_MAINMENU         130

// Status bar pane strings
#define IDS_PANE_CONN        201
#define IDS_PANE_MCU         202
#define IDS_PANE_RATE        203
#define IDS_PANE_LINK        204
#define IDS_PANE_DSP         205
#define IDS_PANE_BUF         206

// Menu / accelerator commands
#define ID_FILE_OPEN_REPLAY     32771
#define ID_FILE_CLOSE_REPLAY    32772
#define ID_FILE_RECORD          32773
#define ID_FILE_SAVE_FRAME      32774
#define ID_FILE_EXPORT_TARGETS  32775
#define ID_FILE_EXPORT_WAV      32776
#define ID_FILE_SCREENSHOT      32777
#define ID_FILE_EXIT            32778
#define ID_VIEW_RADAR           32781
#define ID_VIEW_SCOPE           32782
#define ID_VIEW_COMM            32783
#define ID_VIEW_SETTINGS        32784
#define ID_VIEW_TRACE           32785
#define ID_VIEW_DARK            32786
#define ID_ACQ_CONNECT          32791
#define ID_ACQ_START            32792
#define ID_ACQ_STOP             32793
#define ID_ACQ_TRIGGER          32794
#define ID_ACQ_ABORT            32795
#define ID_ACQ_HOLD             32796
#define ID_ACQ_PING             32797
#define ID_ACQ_STATUS           32798
#define ID_ACQ_RESET_AVG        32799
#define ID_HELP_ABOUT           32801
#define ID_HELP_KEYS            32802
// Plot context menu
#define ID_PLOT_RESET_ZOOM      32811
#define ID_PLOT_AUTOSCALE       32812
#define ID_PLOT_CLEAR_MARKERS   32813
#define ID_PLOT_COPY_IMAGE      32814
#define ID_PLOT_CLEAR_HISTORY   32815
#define ID_PLOT_SNAP_PEAKS      32816

// CommandPanel child controls (1001..1099)
#define IDC_CMB_COM          1001
#define IDC_BTN_CONNECT      1002
#define IDC_BTN_START        1003
#define IDC_BTN_STOP         1004
#define IDC_BTN_TRIGGER      1005
#define IDC_BTN_ABORT        1006
#define IDC_BTN_HOLD         1007
#define IDC_BTN_RECORD       1008
#define IDC_BTN_OPEN_REPLAY  1009
#define IDC_BTN_SAVE_FRAME   1010
#define IDC_BTN_CLEAR        1011
#define IDC_BTN_RP_PLAY      1020
#define IDC_BTN_RP_FIRST     1021
#define IDC_BTN_RP_PREV      1022
#define IDC_BTN_RP_NEXT      1023
#define IDC_BTN_RP_LAST      1024
#define IDC_SLD_RP_POS       1025
#define IDC_CMB_RP_SPEED     1026
#define IDC_LBL_RP_INFO      1027
#define IDC_BTN_RP_CLOSE     1028

// MainFrame child IDs (1100..1199)
#define IDC_CHIRP_LIST       1100
#define IDC_TAB_CTRL         1101
#define IDC_CMD_PANEL        1102
#define IDC_TAB_RADAR        1103
#define IDC_TAB_SCOPE        1104
#define IDC_TAB_COMM         1105
#define IDC_TAB_SETTINGS     1106
#define IDC_TAB_TRACE        1107

// SettingsTab controls (1200..1299)
#define IDC_CMB_MODE         1200
#define IDC_BTN_APPLY_MODE   1201
#define IDC_EDT_FREQ         1202
#define IDC_BTN_SET_FREQ     1203
#define IDC_EDT_SAMPLES      1204
#define IDC_BTN_SET_SAMPLES  1205
#define IDC_EDT_INTERVAL     1206
#define IDC_BTN_APPLY_INT    1207
#define IDC_EDT_AMPLITUDE    1208
#define IDC_BTN_SET_AMP      1209
#define IDC_EDT_BURST        1210
#define IDC_BTN_SET_BURST    1211
#define IDC_CHK_RAW          1212
#define IDC_CHK_FFT          1213
#define IDC_BTN_APPLY_DATA   1214
#define IDC_BTN_PING         1215
#define IDC_BTN_GET_STATUS   1216
#define IDC_BTN_SEND_ALL     1217
#define IDC_EDT_F0           1220
#define IDC_EDT_BW           1221
#define IDC_EDT_TRAMP        1222
#define IDC_EDT_ROFFSET      1223
#define IDC_CMB_SHAPE        1224
#define IDC_CHK_CHIRPS_AUTO  1225
#define IDC_EDT_CHIRPS       1226
#define IDC_EDT_PAIR_V       1227
#define IDC_EDT_GUARD        1230
#define IDC_CHK_DETREND      1231
#define IDC_CMB_DECIM        1232
#define IDC_EDT_MAXRANGE     1233
#define IDC_CMB_WINDOW       1234
#define IDC_EDT_KAISER       1235
#define IDC_CMB_ZEROPAD      1236
#define IDC_CMB_RANGEGAIN    1237
#define IDC_CMB_DETECTOR     1238
#define IDC_EDT_THRESH       1239
#define IDC_CMB_PFA          1240
#define IDC_EDT_CFAR_GUARD   1241
#define IDC_EDT_CFAR_TRAIN   1242
#define IDC_CMB_INTERP       1243
#define IDC_EDT_MAXPEAKS     1244
#define IDC_CHK_MTI          1245
#define IDC_RDO_SRC_RAW      1246
#define IDC_RDO_SRC_MCU      1247
#define IDC_CHK_TRACK        1248
#define IDC_EDT_DBTOP        1250
#define IDC_EDT_DBBOTTOM     1251
#define IDC_CMB_PALETTE      1252
#define IDC_EDT_WF_ROWS      1253
#define IDC_CHK_DARK         1254
#define IDC_EDT_ADCBITS      1255
#define IDC_EDT_VREF         1256
#define IDC_CHK_VOLTS        1257
#define IDC_EDT_FSCAL        1258
#define IDC_CHK_VERBOSE      1259
#define IDC_CHK_AUTOCONNECT  1260
#define IDC_BTN_APPLY_PROC   1261
#define IDC_LBL_DERIVED      1262
#define IDC_BTN_DEFAULTS     1263
// (1270+ are outside the auto-apply ON_CONTROL_RANGE spans)
#define IDC_EDT_RISE         1270
#define IDC_EDT_FALL         1271
#define IDC_BTN_SET_RAMP     1272
#define IDC_EDT_PPM          1273
#define IDC_BTN_APPLY_PPM    1274
#define IDC_CHIRP_PREVIEW    1275
#define IDC_CHK_FW_GEOM      1276
#define IDC_CHK_TONE         1277
#define IDC_CHK_VCO          1278
#define IDC_EDT_VTUNE_LO     1279
#define IDC_EDT_VTUNE_HI     1280
#define IDC_EDT_VCO_CURVE    1281
#define IDC_EDT_OFFSET       1282
#define IDC_BTN_SET_OFFSET   1283

// Radar tab (1300..1399)
#define IDC_RP_VIEW          1300
#define IDC_WF_VIEW          1301
#define IDC_RD_VIEW          1302
#define IDC_TARGET_LIST      1303
#define IDC_CMB_TRACE        1304
#define IDC_CMB_WF_PALETTE   1305
#define IDC_BTN_RESET_AVG    1306
#define IDC_BTN_AUTOSCALE    1307
#define IDC_BTN_CLEAR_WF     1308
#define IDC_CHK_SHOW_UP      1309
#define IDC_CHK_SHOW_DN      1310
#define IDC_CHK_SHOW_THR     1311
#define IDC_CHK_SHOW_NOISE   1312
#define IDC_CHK_SHOW_RD      1313
#define IDC_LBL_PHASE        1314
#define IDC_PROC_PANEL       1315
// ProcPanel: rejection section (1320..1329)
#define IDC_CHK_HARMONICS    1320
#define IDC_EDT_HARM_DROP    1321
#define IDC_EDT_MIN_SNR      1322
#define IDC_EDT_CONFIRM      1323
#define IDC_EDT_SPURS        1324
#define IDC_BTN_LEARN_SPURS  1325
#define IDC_BTN_CLEAR_SPURS  1326

// Scope tab (1400..1499)
#define IDC_WAVE_FRAME       1400
#define IDC_WAVE_RAMP        1401
#define IDC_CHK_DOTS         1402
#define IDC_RDO_VOLTS        1403
#define IDC_RDO_CODES        1404
#define IDC_CMB_RAMP_SEL     1405
#define IDC_LBL_SCOPE_INFO   1406

// Comm log tab (1500..1599)
#define IDC_LOG_LIST         1500
#define IDC_CMB_LOG_FILTER   1501
#define IDC_CHK_LOG_VERBOSE  1502
#define IDC_BTN_LOG_COPY     1503
#define IDC_BTN_LOG_SAVE     1504
#define IDC_BTN_LOG_CLEAR    1505
#define IDC_CHK_LOG_SCROLL   1506

// Trace tab (1700..1799)
#define IDC_TRC_CHK_ENABLE   1700
#define IDC_TRC_BTN_SHOT     1701
#define IDC_TRC_BTN_GET      1702
#define IDC_TRC_BTN_CLEAR    1703
#define IDC_TRC_BTN_EXPORT   1704
#define IDC_TRC_LIST         1705
#define IDC_TRC_LBL_SUMMARY  1706

// WaveformView children (1600..1699)
#define IDC_WV_XM            1600
#define IDC_WV_XP            1601
#define IDC_WV_YM            1602
#define IDC_WV_YP            1603
#define IDC_WV_RST           1604
