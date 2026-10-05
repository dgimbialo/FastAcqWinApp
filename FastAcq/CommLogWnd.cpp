#include "pch.h"
#include "Lang.h"
#include "CommLogWnd.h"
#include "Dpi.h"
#include "Theme.h"
#include "resource.h"

BEGIN_MESSAGE_MAP(CommLogWnd, CWnd)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_TIMER()
    ON_WM_ERASEBKGND()
    ON_WM_CTLCOLOR()
    ON_NOTIFY(LVN_GETDISPINFO, IDC_LOG_LIST, &CommLogWnd::OnGetDispInfo)
    ON_CBN_SELCHANGE(IDC_CMB_LOG_FILTER, &CommLogWnd::OnFilterChanged)
    ON_BN_CLICKED(IDC_CHK_LOG_VERBOSE,   &CommLogWnd::OnVerboseChanged)
    ON_BN_CLICKED(IDC_CHK_LOG_SCROLL,    &CommLogWnd::OnAutoScrollChanged)
    ON_BN_CLICKED(IDC_BTN_LOG_COPY,      &CommLogWnd::OnCopy)
    ON_BN_CLICKED(IDC_BTN_LOG_SAVE,      &CommLogWnd::OnSave)
    ON_BN_CLICKED(IDC_BTN_LOG_CLEAR,     &CommLogWnd::OnClear)
END_MESSAGE_MAP()

BOOL CommLogWnd::CreateTab(CWnd* parent, UINT id)
{
    LPCTSTR cls = AfxRegisterWndClass(0, ::LoadCursor(nullptr, IDC_ARROW), nullptr, nullptr);
    return Create(cls, nullptr, WS_CHILD | WS_CLIPCHILDREN, CRect(0, 0, 10, 10), parent, id);
}

int CommLogWnd::OnCreate(LPCREATESTRUCT lpcs)
{
    if (CWnd::OnCreate(lpcs) == -1) return -1;
    m_bgBrush.CreateSolidBrush(Theme::Get().bg);
    Dpi::MakeFont(m_font, m_hWnd, 9);
    Dpi::MakeMonoFont(m_monoFont, m_hWnd, 9);

    m_list.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT | LVS_OWNERDATA | LVS_NOSORTHEADER | LVS_SHOWSELALWAYS,
                  CRect(0, 0, 10, 10), this, IDC_LOG_LIST);
    m_list.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    m_list.SetFont(&m_monoFont);
    m_list.InsertColumn(0, TR("Time"),    LVCFMT_LEFT, Dpi::Scale(m_hWnd, 96));
    m_list.InsertColumn(1, TR("Kind"),    LVCFMT_LEFT, Dpi::Scale(m_hWnd, 50));
    m_list.InsertColumn(2, TR("Message"), LVCFMT_LEFT, Dpi::Scale(m_hWnd, 1400));

    CRect rc(0, 0, 10, 10);
    m_cmbFilter.Create(WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, rc, this, IDC_CMB_LOG_FILTER);
    m_cmbFilter.AddString(TR("All"));
    m_cmbFilter.AddString(_T("TX"));
    m_cmbFilter.AddString(_T("RX"));
    m_cmbFilter.AddString(TR("Service (ACK/STATUS/PONG)"));
    m_cmbFilter.AddString(TR("Errors"));
    m_cmbFilter.AddString(TR("Port / info"));
    m_cmbFilter.SetCurSel(0);
    m_chkVerbose.Create(TR("Per-frame RX lines"), WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, rc, this, IDC_CHK_LOG_VERBOSE);
    m_chkScroll.Create(TR("Auto-scroll"), WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, rc, this, IDC_CHK_LOG_SCROLL);
    m_chkScroll.SetCheck(BST_CHECKED);
    m_btnCopy.Create(TR("Copy"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, rc, this, IDC_BTN_LOG_COPY);
    m_btnSave.Create(TR("Save..."), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, rc, this, IDC_BTN_LOG_SAVE);
    m_btnClear.Create(TR("Clear"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, rc, this, IDC_BTN_LOG_CLEAR);
    CWnd* kids[] = { &m_cmbFilter, &m_chkVerbose, &m_chkScroll, &m_btnCopy, &m_btnSave, &m_btnClear };
    for (auto* k : kids) k->SetFont(&m_font);

    ApplyTheme();
    SetTimer(kTimerId, 100, nullptr);
    m_tips.Attach(this);
    return 0;
}

void CommLogWnd::ApplyTheme()
{
    const Theme::Palette& th = Theme::Get();
    if (m_bgBrush.GetSafeHandle()) m_bgBrush.DeleteObject();
    m_bgBrush.CreateSolidBrush(th.bg);
    if (m_list.GetSafeHwnd()) {
        m_list.SetBkColor(th.plot);
        m_list.SetTextBkColor(th.plot);
        m_list.SetTextColor(th.text);
        m_list.Invalidate();
    }
    Invalidate();
}

void CommLogWnd::ApplySettings(const AppSettings& s)
{
    m_chkVerbose.SetCheck(s.verboseLog ? BST_CHECKED : BST_UNCHECKED);
    m_chkScroll.SetCheck(s.logAutoScroll ? BST_CHECKED : BST_UNCHECKED);
    m_autoScroll = s.logAutoScroll;
}

void CommLogWnd::ReadFooter(AppSettings& s) const
{
    s.verboseLog    = m_chkVerbose.GetCheck() == BST_CHECKED;
    s.logAutoScroll = m_chkScroll.GetCheck() == BST_CHECKED;
}

LPCTSTR CommLogWnd::KindName(LogKind k)
{
    switch (k) {
    case LOG_TX:   return _T("TX");
    case LOG_RX:   return _T("RX");
    case LOG_SVC:  return _T("SVC");
    case LOG_ERR:  return _T("ERR");
    case LOG_PORT: return _T("PORT");
    default:       return _T("INFO");
    }
}

bool CommLogWnd::PassesFilter(LogKind k) const
{
    switch (m_filter) {
    case 1: return k == LOG_TX;
    case 2: return k == LOG_RX;
    case 3: return k == LOG_SVC;
    case 4: return k == LOG_ERR;
    case 5: return k == LOG_PORT || k == LOG_INFO;
    default: return true;
    }
}

void CommLogWnd::AppendLine(LogKind kind, const CString& text)
{
    SYSTEMTIME st; ::GetLocalTime(&st);
    Line l;
    l.kind = kind;
    l.time.Format(_T("%02d:%02d:%02d.%03d"), st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    l.text = text;
    if (m_lines.size() >= kMaxLines) {
        m_lines.pop_front();
        m_dropped++;
        // Indices in m_view shift by one; rebuild lazily on the next timer tick.
        m_view.clear();
        m_pending = true;
        m_lines.push_back(std::move(l));
        return;
    }
    m_lines.push_back(std::move(l));
    if (!m_view.empty() || m_lines.size() == 1) {
        if (PassesFilter(kind)) m_view.push_back(m_lines.size() - 1);
    }
    m_pending = true;
}

void CommLogWnd::RebuildView()
{
    m_view.clear();
    m_view.reserve(m_lines.size());
    for (size_t i = 0; i < m_lines.size(); ++i)
        if (PassesFilter(m_lines[i].kind)) m_view.push_back(i);
}

void CommLogWnd::OnTimer(UINT_PTR id)
{
    if (id != kTimerId) { CWnd::OnTimer(id); return; }
    if (!m_pending) return;
    m_pending = false;
    if (m_view.empty() && !m_lines.empty()) RebuildView();
    const int n = static_cast<int>(m_view.size());
    m_list.SetItemCountEx(n, LVSICF_NOINVALIDATEALL | LVSICF_NOSCROLL);
    if (n > 0) {
        m_list.RedrawItems((std::max)(0, n - 50), n - 1);
        if (m_autoScroll) m_list.EnsureVisible(n - 1, FALSE);
    }
}

void CommLogWnd::OnGetDispInfo(NMHDR* pNMHDR, LRESULT* pResult)
{
    auto* di = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
    LVITEM& it = di->item;
    *pResult = 0;
    if (!(it.mask & LVIF_TEXT)) return;
    if (it.iItem < 0 || static_cast<size_t>(it.iItem) >= m_view.size()) { it.pszText[0] = 0; return; }
    const Line& l = m_lines[m_view[static_cast<size_t>(it.iItem)]];
    LPCTSTR src = (it.iSubItem == 0) ? l.time.GetString() : (it.iSubItem == 1) ? KindName(l.kind) : l.text.GetString();
    _tcsncpy_s(it.pszText, it.cchTextMax, src, _TRUNCATE);
}

void CommLogWnd::OnFilterChanged()
{
    m_filter = m_cmbFilter.GetCurSel();
    RebuildView();
    m_list.SetItemCountEx(static_cast<int>(m_view.size()), 0);
    m_list.Invalidate();
    if (m_autoScroll && !m_view.empty()) m_list.EnsureVisible(static_cast<int>(m_view.size()) - 1, FALSE);
}

void CommLogWnd::OnVerboseChanged()
{
    CWnd* main = AfxGetMainWnd();
    if (main && ::IsWindow(main->GetSafeHwnd()))
        main->PostMessage(WM_APP_SETTINGS_CHANGED, SETTINGS_FROM_LOG, 0);
}

void CommLogWnd::OnAutoScrollChanged()
{
    m_autoScroll = m_chkScroll.GetCheck() == BST_CHECKED;
    OnVerboseChanged();
}

CString CommLogWnd::LinesText(bool selectedOnly) const
{
    CString out;
    if (selectedOnly) {
        POSITION pos = m_list.GetFirstSelectedItemPosition();
        while (pos) {
            int row = m_list.GetNextSelectedItem(pos);
            if (row >= 0 && static_cast<size_t>(row) < m_view.size()) {
                const Line& l = m_lines[m_view[static_cast<size_t>(row)]];
                out += l.time + _T("  ") + KindName(l.kind) + _T("  ") + l.text + _T("\r\n");
            }
        }
    } else {
        for (size_t i : m_view) {
            const Line& l = m_lines[i];
            out += l.time + _T("  ") + KindName(l.kind) + _T("  ") + l.text + _T("\r\n");
        }
    }
    return out;
}

void CommLogWnd::OnCopy()
{
    CString text = LinesText(m_list.GetSelectedCount() > 0);
    if (text.IsEmpty()) return;
    if (!::OpenClipboard(m_hWnd)) return;
    ::EmptyClipboard();
    const size_t bytes = (static_cast<size_t>(text.GetLength()) + 1) * sizeof(TCHAR);
    HGLOBAL h = ::GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (h) {
        void* p = ::GlobalLock(h);
        if (p) { std::memcpy(p, text.GetString(), bytes); ::GlobalUnlock(h); }
        ::SetClipboardData(CF_UNICODETEXT, h);
    }
    ::CloseClipboard();
}

void CommLogWnd::OnSave()
{
    CFileDialog dlg(FALSE, _T("txt"), _T("fastacq_log.txt"), OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
                    TR("Text files (*.txt)|*.txt|All files (*.*)|*.*||"), this);
    if (dlg.DoModal() != IDOK) return;
    CString text = LinesText(false);
    text.Replace(_T("\r\n"), _T("\n"));   // typeText re-expands LF to CRLF
    CStdioFile f;
    if (!f.Open(dlg.GetPathName(), CFile::modeCreate | CFile::modeWrite | CFile::typeText)) {
        AfxMessageBox(TR("Cannot write the log file."));
        return;
    }
    f.WriteString(text);
    f.Close();
}

void CommLogWnd::OnClear() { Clear(); }

void CommLogWnd::Clear()
{
    m_lines.clear();
    m_view.clear();
    m_dropped = 0;
    m_pending = false;
    if (m_list.GetSafeHwnd()) { m_list.SetItemCountEx(0, 0); m_list.Invalidate(); }
}

void CommLogWnd::OnSize(UINT, int cx, int cy)
{
    if (!m_list.GetSafeHwnd()) return;
    const int S = static_cast<int>(Dpi::Of(m_hWnd));
    auto sc = [&](int px) { return ::MulDiv(px, S, 96); };
    const int footerH = sc(kFooterH96);
    if (cy < footerH + 20) return;
    m_list.MoveWindow(0, 0, cx, cy - footerH);
    const int fy = cy - footerH + (footerH - sc(22)) / 2;
    const int h = sc(22);
    int x = sc(6);
    auto place = [&](CWnd& w, int wpx, int extraH = 0) {
        const int ww = Dpi::FitWidth(w, sc(wpx));
        if (w.GetSafeHwnd()) w.MoveWindow(x, fy, ww, extraH ? extraH : h);
        x += ww + sc(4);
    };
    place(m_cmbFilter, 190, sc(200));
    x += sc(6);
    place(m_chkVerbose, 150);
    place(m_chkScroll, 90);
    x += sc(6);
    place(m_btnCopy, 60);
    place(m_btnSave, 70);
    place(m_btnClear, 60);
}

BOOL CommLogWnd::OnEraseBkgnd(CDC* pDC)
{
    CRect rc; GetClientRect(&rc);
    pDC->FillSolidRect(rc, Theme::Get().bg);
    return TRUE;
}

HBRUSH CommLogWnd::OnCtlColor(CDC* pDC, CWnd*, UINT nCtlColor)
{
    if (nCtlColor == CTLCOLOR_STATIC || nCtlColor == CTLCOLOR_BTN) {
        pDC->SetBkMode(TRANSPARENT);
        pDC->SetTextColor(Theme::Get().text);
        return static_cast<HBRUSH>(m_bgBrush.GetSafeHandle());
    }
    // Edit fields, list boxes and combo drop-downs: keep text readable in the dark theme.
    // Editable fields: white (light theme) so they stand out from read-only
    // read-outs, which arrive as CTLCOLOR_STATIC and keep the grey background.
    pDC->SetTextColor(Theme::Get().text);
    pDC->SetBkColor(Theme::Get().plot);
    return Theme::FieldBrush();
}

BOOL CommLogWnd::PreTranslateMessage(MSG* pMsg)
{
    m_tips.Relay(pMsg);
    return CWnd::PreTranslateMessage(pMsg);
}
