#pragma once
//
// TraceTabWnd -- "Trace" tab: MCU test mode. Enables the acquisition trace on
// the device, fires single captures and shows the step-by-step timeline
// (chirp armed -> VSYNC -> DAC -> DMA chunks -> capture done -> FFT -> USB TX)
// decoded from FRAME_ID_TRACE frames. Rows can be exported to CSV.
// Native Win32 look: standard controls, system colors.
//

#include "pch.h"
#include "ChirpStore.h"
#include "TraceDefs.h"
#include <vector>

class TraceTabWnd : public CWnd {
public:
    BOOL CreateTab(CWnd* parent, UINT id);

    // Decode and display a trace frame (header + TraceRecord[] in f.raw bytes).
    void ShowTrace(const ChirpFrame& f);
    void SetConnected(bool c);
    // Reflect the device-reported trace state (from STATUS frames).
    void SetDeviceTraceEnabled(bool on);
    bool IsTraceRequested() const;

protected:
    afx_msg int  OnCreate(LPCREATESTRUCT lpcs);
    afx_msg void OnSize(UINT, int, int);
    afx_msg void OnTraceToggle();
    afx_msg void OnSingleShot();
    afx_msg void OnGetTrace();
    afx_msg void OnClear();
    afx_msg void OnExport();
    DECLARE_MESSAGE_MAP()

private:
    void Relayout();
    void PostToMain(UINT msg, WPARAM wp = 0, LPARAM lp = 0);
    void Rebuild();

    static constexpr int kToolH = 34;

    CButton   m_chkTrace;
    CButton   m_btnShot;
    CButton   m_btnGet;
    CButton   m_btnClear;
    CButton   m_btnExport;
    CStatic   m_lblSummary;
    CListCtrl m_list;
    CFont     m_font;

    std::vector<TraceRecord> m_records;
    FrameHeader              m_hdr{};
    bool                     m_connected{false};
};
