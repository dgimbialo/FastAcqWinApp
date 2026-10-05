#include "pch.h"
#include "ChirpListCtrl.h"
#include "AppMessages.h"
#include "Dpi.h"
#include "Theme.h"

BEGIN_MESSAGE_MAP(ChirpListCtrl, CListCtrl)
    ON_NOTIFY_REFLECT(LVN_ITEMCHANGED, &ChirpListCtrl::OnItemChanged)
END_MESSAGE_MAP()

void ChirpListCtrl::InitColumns()
{
    SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    InsertColumn(0, _T("ID"),      LVCFMT_RIGHT, Dpi::Scale(m_hWnd, 52));
    InsertColumn(1, _T("t, ms"),   LVCFMT_RIGHT, Dpi::Scale(m_hWnd, 64));
    InsertColumn(2, _T("R, m"),    LVCFMT_RIGHT, Dpi::Scale(m_hWnd, 58));
    InsertColumn(3, _T("v, m/s"),  LVCFMT_RIGHT, Dpi::Scale(m_hWnd, 56));
    InsertColumn(4, _T("SNR"),     LVCFMT_RIGHT, Dpi::Scale(m_hWnd, 44));
    ApplyTheme();
}

void ChirpListCtrl::ApplyTheme()
{
    const Theme::Palette& th = Theme::Get();
    SetBkColor(th.plot);
    SetTextBkColor(th.plot);
    SetTextColor(th.text);
    if (GetSafeHwnd()) Invalidate();
}

void ChirpListCtrl::AddFrame(uint64_t seq, uint32_t frameId, uint32_t tsMs, size_t)
{
    CString id, ts;
    id.Format(_T("%u"), frameId);
    ts.Format(_T("%u"), tsMs);
    m_suppressNotify = true;
    while (GetItemCount() >= m_maxRows) DeleteItem(0);
    int row = InsertItem(GetItemCount(), id);
    SetItemText(row, 1, ts);
    SetItemText(row, 2, _T(""));
    SetItemText(row, 3, _T(""));
    SetItemText(row, 4, _T(""));
    SetItemData(row, static_cast<DWORD_PTR>(seq));
    m_suppressNotify = false;
    // Keep the newest row visible unless the user is inspecting an older one.
    if (GetSelectedCount() == 0) EnsureVisible(row, FALSE);
}

int ChirpListCtrl::FindRow(uint64_t seq) const
{
    for (int i = GetItemCount() - 1; i >= 0; --i)
        if (static_cast<uint64_t>(GetItemData(i)) == seq) return i;
    return -1;
}

void ChirpListCtrl::UpdateResult(const dsp::FrameResult& r)
{
    const int row = FindRow(r.seq);
    if (row < 0) return;
    CString s;
    if (!r.targets.empty()) {
        const dsp::Target& t = r.targets[0];
        if (r.rangePerHz > 0.0) s.Format(_T("%.2f"), t.rangeM); else s = _T("-");
        SetItemText(row, 2, s);
        if (t.paired) s.Format(_T("%+.2f"), t.velocityMps); else s = _T("-");
        SetItemText(row, 3, s);
        s.Format(_T("%.0f"), t.snrDb);
        SetItemText(row, 4, s);
    } else {
        SetItemText(row, 2, _T("-"));
        SetItemText(row, 3, _T("-"));
        SetItemText(row, 4, _T("-"));
    }
}

void ChirpListCtrl::ClearAll()
{
    m_suppressNotify = true;
    DeleteAllItems();
    m_suppressNotify = false;
}

bool ChirpListCtrl::SelectSeq(uint64_t seq)
{
    const int row = FindRow(seq);
    if (row < 0) return false;
    SetItemState(row, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    EnsureVisible(row, FALSE);
    return true;
}

uint64_t ChirpListCtrl::SelectedSeq() const
{
    POSITION pos = GetFirstSelectedItemPosition();
    if (!pos) return ~0ull;
    int row = GetNextSelectedItem(pos);
    return static_cast<uint64_t>(GetItemData(row));
}

uint64_t ChirpListCtrl::LastSeq() const
{
    const int n = GetItemCount();
    return n > 0 ? static_cast<uint64_t>(GetItemData(n - 1)) : ~0ull;
}

bool ChirpListCtrl::SelectRelative(int delta)
{
    const int n = GetItemCount();
    if (n == 0) return false;
    int row;
    POSITION pos = GetFirstSelectedItemPosition();
    if (!pos) row = (delta < 0) ? n - 1 : 0;
    else      row = GetNextSelectedItem(pos) + delta;
    if (row < 0) row = 0;
    if (row >= n) row = n - 1;
    SetItemState(row, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    EnsureVisible(row, FALSE);
    return true;
}

void ChirpListCtrl::OnItemChanged(NMHDR* pNMHDR, LRESULT* pResult)
{
    auto* nm = reinterpret_cast<NMLISTVIEW*>(pNMHDR);
    if (!m_suppressNotify && (nm->uNewState & LVIS_SELECTED) && !(nm->uOldState & LVIS_SELECTED)) {
        uint64_t seq = static_cast<uint64_t>(GetItemData(nm->iItem));
        if (CWnd* p = GetParent())
            p->PostMessage(WM_APP_FRAME_SELECTED, static_cast<WPARAM>(seq), 0);
    }
    *pResult = 0;
}
