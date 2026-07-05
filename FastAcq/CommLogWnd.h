#pragma once
//
// CommLogWnd -- tab page showing real-time communication log.
//

#include "pch.h"

class CommLogWnd : public CWnd {
public:
    BOOL CreateTab(CWnd* parent, UINT id);
    void AppendLine(const CString& line);
    void Clear();

protected:
    afx_msg int  OnCreate(LPCREATESTRUCT);
    afx_msg void OnSize(UINT, int, int);
    afx_msg HBRUSH OnCtlColor(CDC*, CWnd*, UINT);
    DECLARE_MESSAGE_MAP()

private:
    static constexpr int kMaxLines  = 2000;
    static constexpr int kTrimLines = 500;

    CEdit  m_log;
    CFont  m_font;
    CBrush m_bgBrush;
};
