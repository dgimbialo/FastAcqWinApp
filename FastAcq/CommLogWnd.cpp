#include "pch.h"
#include "CommLogWnd.h"

BEGIN_MESSAGE_MAP(CommLogWnd, CWnd)
    ON_WM_CREATE()
    ON_WM_SIZE()
END_MESSAGE_MAP()

BOOL CommLogWnd::CreateTab(CWnd* parent, UINT id)
{
    LPCTSTR cls = AfxRegisterWndClass(0, ::LoadCursor(nullptr, IDC_ARROW),
                                      reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1),
                                      nullptr);
    return Create(cls, nullptr, WS_CHILD, CRect(0, 0, 10, 10), parent, id);
}

int CommLogWnd::OnCreate(LPCREATESTRUCT lpcs)
{
    if (CWnd::OnCreate(lpcs) == -1) return -1;

    m_log.Create(WS_CHILD | WS_VISIBLE | WS_VSCROLL |
                 ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
                 CRect(0, 0, 10, 10), this, 3001);

    m_font.CreateFont(-13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                      DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                      DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, _T("Consolas"));
    m_log.SetFont(&m_font);
    return 0;
}

void CommLogWnd::OnSize(UINT t, int cx, int cy)
{
    CWnd::OnSize(t, cx, cy);
    if (m_log.GetSafeHwnd())
        m_log.MoveWindow(0, 0, cx, cy);
}

void CommLogWnd::AppendLine(const CString& line)
{
    // Trim oldest lines when buffer is too large.
    int nLines = m_log.GetLineCount();
    if (nLines > kMaxLines) {
        int pos = m_log.LineIndex(kTrimLines);
        if (pos > 0) {
            m_log.SetSel(0, pos);
            m_log.ReplaceSel(_T(""));
        }
    }

    int len = m_log.GetWindowTextLength();
    m_log.SetSel(len, len);
    m_log.ReplaceSel(line + _T("\r\n"));
}

void CommLogWnd::Clear()
{
    m_log.SetWindowText(_T(""));
}
