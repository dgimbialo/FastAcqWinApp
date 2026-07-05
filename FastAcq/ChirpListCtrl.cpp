#include "pch.h"
#include "ChirpListCtrl.h"
#include "AppMessages.h"

BEGIN_MESSAGE_MAP(ChirpListCtrl, CListCtrl)
    ON_NOTIFY_REFLECT(LVN_ITEMCHANGED, &ChirpListCtrl::OnItemChanged)
END_MESSAGE_MAP()

void ChirpListCtrl::InitColumns()
{
    // Native list-view look: system colors, standard header.
    SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    InsertColumn(0, _T("ID"),       LVCFMT_RIGHT, 70);
    InsertColumn(1, _T("t, ms"),    LVCFMT_RIGHT, 80);
    InsertColumn(2, _T("Peak Hz"),  LVCFMT_RIGHT, 90);
}

void ChirpListCtrl::AddChirp(size_t logicalIndex, uint32_t frameId,
                             uint32_t tsMs, float peakHz)
{
    CString id, ts, pk;
    id.Format(_T("%u"), frameId);
    ts.Format(_T("%u"), tsMs);
    pk.Format(_T("%.0f"), peakHz);

    int row = InsertItem(GetItemCount(), id);
    SetItemText(row, 1, ts);
    SetItemText(row, 2, pk);
    SetItemData(row, static_cast<DWORD_PTR>(logicalIndex));
    EnsureVisible(row, FALSE);
}

void ChirpListCtrl::ClearAll()
{
    DeleteAllItems();
}

void ChirpListCtrl::OnItemChanged(NMHDR* pNMHDR, LRESULT* pResult)
{
    auto* nm = reinterpret_cast<NMLISTVIEW*>(pNMHDR);
    if ((nm->uNewState & LVIS_SELECTED) && !(nm->uOldState & LVIS_SELECTED)) {
        size_t idx = static_cast<size_t>(GetItemData(nm->iItem));
        if (CWnd* p = GetParent())
            p->PostMessage(WM_APP_FRAME_SELECTED, static_cast<WPARAM>(idx), 0);
    }
    *pResult = 0;
}
