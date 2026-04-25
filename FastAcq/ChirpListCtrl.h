#pragma once
//
// ChirpListCtrl -- CListCtrl subclass with columns ID | Time | Peak Hz.
// Posts WM_APP_FRAME_SELECTED to parent when user changes selection.
//

#include "pch.h"

class ChirpListCtrl : public CListCtrl {
public:
    ChirpListCtrl() = default;

    void InitColumns();
    void AddChirp(size_t logicalIndex, uint32_t frameId,
                  uint32_t tsMs, float peakHz);
    void ClearAll();

protected:
    afx_msg void OnItemChanged(NMHDR* pNMHDR, LRESULT* pResult);
    DECLARE_MESSAGE_MAP()
};
