#include "pch.h"
#include "ModernButton.h"

BEGIN_MESSAGE_MAP(ModernButton, CButton)
    ON_WM_MOUSEMOVE()
    ON_WM_MOUSELEAVE()
END_MESSAGE_MAP()

void ModernButton::OnMouseMove(UINT nFlags, CPoint pt)
{
    if (!m_tracking) {
        TRACKMOUSEEVENT tme{ sizeof(tme) };
        tme.dwFlags   = TME_LEAVE;
        tme.hwndTrack = m_hWnd;
        ::TrackMouseEvent(&tme);
        m_tracking = true;
        if (!m_hot) { m_hot = true; Invalidate(); }
    }
    CButton::OnMouseMove(nFlags, pt);
}

void ModernButton::OnMouseLeave()
{
    m_tracking = false;
    if (m_hot) { m_hot = false; Invalidate(); }
    CButton::OnMouseLeave();
}

void ModernButton::DrawItem(LPDRAWITEMSTRUCT lpDIS)
{
    CDC*  dc = CDC::FromHandle(lpDIS->hDC);
    CRect rc = lpDIS->rcItem;

    const bool disabled = (lpDIS->itemState & ODS_DISABLED) != 0;
    const bool pressed  = (lpDIS->itemState & ODS_SELECTED) != 0;

    // Blend the rounded-corner gaps into the host panel.
    dc->FillSolidRect(rc, m_back);

    COLORREF face;
    if (disabled)      face = Theme::Shade(Theme::Panel, 1.35);
    else if (pressed)  face = Theme::Shade(m_accent, 0.78);
    else if (m_hot)    face = Theme::Shade(m_accent, 1.18);
    else               face = m_accent;

    COLORREF edge = disabled ? Theme::Border : Theme::Shade(face, 1.25);

    CBrush  brFace(face);
    CPen    penEdge(PS_SOLID, 1, edge);
    CBrush* pOldBr = dc->SelectObject(&brFace);
    CPen*   pOldPen = dc->SelectObject(&penEdge);
    dc->SetBkMode(TRANSPARENT);
    dc->RoundRect(rc.left, rc.top, rc.right, rc.bottom, 7, 7);
    dc->SelectObject(pOldBr);
    dc->SelectObject(pOldPen);

    CString text;
    GetWindowText(text);
    if (!text.IsEmpty()) {
        dc->SetTextColor(disabled ? Theme::TextDim : RGB(255, 255, 255));
        if (pressed) rc.OffsetRect(0, 1);   // tiny press nudge
        dc->DrawText(text, rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    if (lpDIS->itemState & ODS_FOCUS) {
        // Soft focus ring, inset.
        CRect fr = lpDIS->rcItem; fr.DeflateRect(2, 2);
        // (No dotted rect: keep the flat modern look.)
    }
}
