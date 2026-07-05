#pragma once
//
// ModernTabCtrl -- owner-drawn CTabCtrl with a flat dark look and an
//                  accent-highlighted active tab. Create with TCS_OWNERDRAWFIXED.
//

#include "pch.h"
#include "Theme.h"

class ModernTabCtrl : public CTabCtrl {
public:
    void DrawItem(LPDRAWITEMSTRUCT lpDIS) override;

protected:
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    DECLARE_MESSAGE_MAP()
};
