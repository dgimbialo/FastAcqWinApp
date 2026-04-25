#include "pch.h"
#include "ChirpListCtrl.h"
#include "AppMessages.h"

BEGIN_MESSAGE_MAP(ChirpListCtrl, CListCtrl)
    ON_NOTIFY_REFLECT(LVN_ITEMCHANGED, &ChirpListCtrl::OnItemChanged)
END_MESSAGE_MAP()

void ChirpListCtrl::InitColumns()
{
    SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    InsertColumn(0, _T("ID"),       LVCFMT_RIGHT, 70);
    InsertColumn(1, _T("t, ms"),    LVCFMT_RIGHT, 80);
    InsertColumn(2, _T("Peak Hz"),  LVCFMT_RIGHT, 90);
}

void ChirpListCtrl::AddChirp(size_t logicalIndex, uint32_t frameId,
                             uint32_t tsMs, float peakHz)
{
    CString s;
    s.Format(_T("#%u"), frameId);
    int row = InsertItem(static_cast<int>(logicalIndex), s);
    if (row < 0) return;
    SetItemData(row, static_cast<DWORD_PTR>(logicalIndex));
    s.Format(_T("%u"), tsMs);           SetItemText(row, 1, s);
    s.Format(_T("%.0f"), peakHz);       SetItemText(row, 2, s);
    EnsureVisible(row, FALSE);
}

void ChirpListCtrl::ClearAll()
{
    DeleteAllItems();
}

void ChirpListCtrl::OnItemChanged(NMHDR* pNMHDR, LRESULT* pResult)
{
    auto* pn = reinterpret_cast<NMLISTVIEW*>(pNMHDR);
    *pResult = 0;
    if ((pn->uChanged & LVIF_STATE) &&
        (pn->uNewState & LVIS_SELECTED) &&
        !(pn->uOldState & LVIS_SELECTED))
    {
        size_t idx = static_cast<size_t>(GetItemData(pn->iItem));
        CWnd* parent = GetParent();
        if (parent && ::IsWindow(parent->GetSafeHwnd())) {
            parent->PostMessage(WM_APP_FRAME_SELECTED,
                                static_cast<WPARAM>(idx), 0);
        }
    }
}
