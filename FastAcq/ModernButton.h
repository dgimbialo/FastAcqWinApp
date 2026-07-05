#pragma once
//
// ModernButton -- flat, rounded, accent-colored owner-drawn push button
//                 with a subtle hover highlight. Create with BS_OWNERDRAW.
//

#include "pch.h"
#include "Theme.h"

class ModernButton : public CButton {
public:
    // Base accent color of the button face (default: Theme::Accent).
    void SetAccent(COLORREF c)   { m_accent = c;  if (GetSafeHwnd()) Invalidate(); }
    // Background behind the rounded corners (should match the host panel).
    void SetBackColor(COLORREF c){ m_back   = c;  if (GetSafeHwnd()) Invalidate(); }

    void DrawItem(LPDRAWITEMSTRUCT lpDIS) override;

protected:
    afx_msg void OnMouseMove(UINT nFlags, CPoint pt);
    afx_msg void OnMouseLeave();
    DECLARE_MESSAGE_MAP()

private:
    COLORREF m_accent{ Theme::Accent };
    COLORREF m_back{ Theme::Panel };
    bool     m_hot{ false };
    bool     m_tracking{ false };
};
