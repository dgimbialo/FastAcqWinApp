#include "pch.h"
#include "ModernTabCtrl.h"

BEGIN_MESSAGE_MAP(ModernTabCtrl, CTabCtrl)
    ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

BOOL ModernTabCtrl::OnEraseBkgnd(CDC* pDC)
{
    CRect rc; GetClientRect(&rc);
    pDC->FillSolidRect(rc, Theme::Bg);
    return TRUE;
}

void ModernTabCtrl::DrawItem(LPDRAWITEMSTRUCT lpDIS)
{
    CDC*  dc = CDC::FromHandle(lpDIS->hDC);
    CRect rc = lpDIS->rcItem;
    const int  idx      = static_cast<int>(lpDIS->itemID);
    const bool selected = (lpDIS->itemState & ODS_SELECTED) != 0;

    COLORREF face = selected ? Theme::Accent : Theme::Panel;
    dc->FillSolidRect(rc, face);

    if (selected) {
        // Bright accent underline for the active tab.
        dc->FillSolidRect(rc.left, rc.bottom - 3, rc.Width(), 3,
                          Theme::Shade(Theme::Accent, 1.3));
    } else {
        dc->FillSolidRect(rc.right - 1, rc.top + 4, 1, rc.Height() - 8, Theme::Border);
    }

    TCHAR buf[64]{};
    TCITEM ti{}; ti.mask = TCIF_TEXT; ti.pszText = buf; ti.cchTextMax = 63;
    GetItem(idx, &ti);

    dc->SetBkMode(TRANSPARENT);
    dc->SetTextColor(selected ? RGB(255, 255, 255) : Theme::TextDim);
    dc->DrawText(buf, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}
