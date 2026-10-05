#include "pch.h"
#include "TargetListCtrl.h"
#include "Dpi.h"
#include "Theme.h"

void TargetListCtrl::Init()
{
    SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_GRIDLINES);
    InsertColumn(0, _T("#"),        LVCFMT_RIGHT, Dpi::Scale(m_hWnd, 32));
    InsertColumn(1, _T("ID"),       LVCFMT_RIGHT, Dpi::Scale(m_hWnd, 40));
    InsertColumn(2, _T("R, m"),     LVCFMT_RIGHT, Dpi::Scale(m_hWnd, 78));
    InsertColumn(3, _T("v, m/s"),   LVCFMT_RIGHT, Dpi::Scale(m_hWnd, 70));
    InsertColumn(4, _T("f_up, kHz"),LVCFMT_RIGHT, Dpi::Scale(m_hWnd, 84));
    InsertColumn(5, _T("f_dn, kHz"),LVCFMT_RIGHT, Dpi::Scale(m_hWnd, 84));
    InsertColumn(6, _T("A, dBFS"),  LVCFMT_RIGHT, Dpi::Scale(m_hWnd, 70));
    InsertColumn(7, _T("SNR, dB"),  LVCFMT_RIGHT, Dpi::Scale(m_hWnd, 70));
    InsertColumn(8, _T("pair"),     LVCFMT_CENTER,Dpi::Scale(m_hWnd, 44));
    ApplyTheme();
}

void TargetListCtrl::ApplyTheme()
{
    const Theme::Palette& th = Theme::Get();
    SetBkColor(th.plot);
    SetTextBkColor(th.plot);
    SetTextColor(th.text);
    if (GetSafeHwnd()) Invalidate();
}

void TargetListCtrl::SetTargets(const std::vector<dsp::Target>& t, bool haveRange, bool triangle)
{
    m_targets = t;
    SetRedraw(FALSE);
    const int n = static_cast<int>(t.size());
    while (GetItemCount() > n) DeleteItem(GetItemCount() - 1);
    for (int i = 0; i < n; ++i) {
        CString s;
        s.Format(_T("%d"), i + 1);
        if (i >= GetItemCount()) InsertItem(i, s); else SetItemText(i, 0, s);
        const dsp::Target& tg = t[static_cast<size_t>(i)];
        s.Format(_T("%d"), tg.id);                              SetItemText(i, 1, s);
        if (haveRange) s.Format(_T("%.3f"), tg.rangeM); else s = _T("-"); SetItemText(i, 2, s);
        if (tg.paired) s.Format(_T("%+.2f"), tg.velocityMps); else s = _T("-"); SetItemText(i, 3, s);
        if (tg.fUpHz > 0.0) s.Format(_T("%.3f"), tg.fUpHz / 1e3); else s = _T("-"); SetItemText(i, 4, s);
        if (tg.fDnHz > 0.0) s.Format(_T("%.3f"), tg.fDnHz / 1e3); else s = _T("-"); SetItemText(i, 5, s);
        s.Format(_T("%.1f"), tg.ampDb);                         SetItemText(i, 6, s);
        s.Format(_T("%.1f"), tg.snrDb);                         SetItemText(i, 7, s);
        s = triangle ? (tg.paired ? _T("yes") : _T("no")) : _T("-"); SetItemText(i, 8, s);
    }
    SetRedraw(TRUE);
    Invalidate(FALSE);
}

CString TargetListCtrl::CsvHeader()
{
    return _T("frame_id,timestamp_ms,target_id,range_m,velocity_mps,f_up_hz,f_dn_hz,amp_dbfs,snr_db,paired\n");
}

CString TargetListCtrl::CsvRow(uint32_t frameId, uint32_t tsMs, const dsp::Target& t)
{
    CString s;
    s.Format(_T("%u,%u,%d,%.4f,%.4f,%.3f,%.3f,%.2f,%.2f,%d\n"),
             frameId, tsMs, t.id, t.rangeM, t.velocityMps, t.fUpHz, t.fDnHz, t.ampDb, t.snrDb, t.paired ? 1 : 0);
    return s;
}

CString TargetListCtrl::ToCsv() const
{
    CString out = CsvHeader();
    for (const auto& t : m_targets) out += CsvRow(0, 0, t);
    return out;
}
