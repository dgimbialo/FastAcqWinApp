#pragma once
//
// CommLogWnd -- communication log on a virtual list control (ring buffer of
// lines, kind filter, copy / save / clear, optional per-frame verbose lines).
//

#include "pch.h"
#include "Tips.h"
#include "AppMessages.h"
#include "AppSettings.h"

class CommLogWnd : public CWnd {
public:
    BOOL CreateTab(CWnd* parent, UINT id);

    void AppendLine(LogKind kind, const CString& text);
    void Clear();
    void ApplySettings(const AppSettings& s);
    void ReadFooter(AppSettings& s) const;
    void ApplyTheme();

    BOOL PreTranslateMessage(MSG* pMsg) override;

protected:
    afx_msg int    OnCreate(LPCREATESTRUCT lpcs);
    afx_msg void   OnSize(UINT, int, int);
    afx_msg void   OnTimer(UINT_PTR id);
    afx_msg BOOL   OnEraseBkgnd(CDC* pDC);
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    afx_msg void   OnGetDispInfo(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void   OnFilterChanged();
    afx_msg void   OnVerboseChanged();
    afx_msg void   OnAutoScrollChanged();
    afx_msg void   OnCopy();
    afx_msg void   OnSave();
    afx_msg void   OnClear();
    DECLARE_MESSAGE_MAP()

private:
    struct Line { LogKind kind; CString time; CString text; };

    void RebuildView();
    bool PassesFilter(LogKind k) const;
    static LPCTSTR KindName(LogKind k);
    CString LinesText(bool selectedOnly) const;

    static constexpr size_t kMaxLines = 50000;
    static constexpr int    kTimerId  = 1;
    static constexpr int    kFooterH96 = 30;

    CListCtrl m_list;
    CComboBox m_cmbFilter;
    CButton   m_chkVerbose;
    CButton   m_chkScroll;
    CButton   m_btnCopy, m_btnSave, m_btnClear;
    CFont     m_font;
    CFont     m_monoFont;
    CBrush    m_bgBrush;

    std::deque<Line>    m_lines;      // all lines (oldest first)
    std::vector<size_t> m_view;       // indices into m_lines passing the filter
    size_t   m_dropped{0};            // lines removed from the front of m_lines
    bool     m_pending{false};
    bool     m_autoScroll{true};
    int      m_filter{0};
    FieldTips m_tips;
};
