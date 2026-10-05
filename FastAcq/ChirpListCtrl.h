#pragma once
//
// ChirpListCtrl -- list of received frames (ID, time, range, velocity, SNR).
// Rows carry the ChirpStore sequence number; selecting a row posts
// WM_APP_FRAME_SELECTED (wParam = seq) to the parent.
//

#include "pch.h"
#include "Dsp/RadarDsp.h"

class ChirpListCtrl : public CListCtrl {
public:
    ChirpListCtrl() = default;

    void InitColumns();
    void AddFrame(uint64_t seq, uint32_t frameId, uint32_t tsMs, size_t samples);
    void UpdateResult(const dsp::FrameResult& r);
    void ClearAll();
    void SetMaxRows(int n) { m_maxRows = n > 10 ? n : 10; }
    void ApplyTheme();

    // Selection helpers (used by keyboard navigation). Return false if none.
    bool     SelectSeq(uint64_t seq);
    bool     SelectRelative(int delta);
    uint64_t SelectedSeq() const;
    uint64_t LastSeq() const;

protected:
    afx_msg void OnItemChanged(NMHDR* pNMHDR, LRESULT* pResult);
    DECLARE_MESSAGE_MAP()

private:
    int FindRow(uint64_t seq) const;

    int  m_maxRows{200};
    bool m_suppressNotify{false};
};
