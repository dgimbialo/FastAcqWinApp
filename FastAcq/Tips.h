#pragma once
//
// Tips.h -- tooltips for the input controls of a panel. Attach() creates a
// tooltip control and registers every direct child window that has a help
// text in Lang::Help(id); the owning window relays mouse messages through
// Relay() from its PreTranslateMessage.
//

#include "pch.h"
#include "Dpi.h"
#include "Lang.h"

class FieldTips {
public:
    void Attach(CWnd* parent)
    {
        if (!parent || !parent->GetSafeHwnd()) return;
        if (!m_tip.GetSafeHwnd()) {
            m_tip.Create(parent, TTS_ALWAYSTIP | TTS_NOPREFIX);
            m_tip.SetMaxTipWidth(Dpi::Scale(parent->GetSafeHwnd(), 440));
            m_tip.SetDelayTime(TTDT_INITIAL, 400);
            m_tip.SetDelayTime(TTDT_AUTOPOP, 30000);
            m_tip.SetDelayTime(TTDT_RESHOW, 200);
        }
        for (CWnd* c = parent->GetWindow(GW_CHILD); c; c = c->GetWindow(GW_HWNDNEXT)) {
            const CString h = Lang::Help(static_cast<UINT>(c->GetDlgCtrlID()));
            if (!h.IsEmpty()) m_tip.AddTool(c, h);
        }
        m_tip.Activate(TRUE);
    }

    void Relay(MSG* pMsg)
    {
        if (m_tip.GetSafeHwnd() && pMsg &&
            (pMsg->message == WM_MOUSEMOVE || pMsg->message == WM_LBUTTONDOWN || pMsg->message == WM_LBUTTONUP ||
             pMsg->message == WM_RBUTTONDOWN || pMsg->message == WM_RBUTTONUP || pMsg->message == WM_MBUTTONDOWN ||
             pMsg->message == WM_MBUTTONUP))
            m_tip.RelayEvent(pMsg);
    }

private:
    CToolTipCtrl m_tip;
};
