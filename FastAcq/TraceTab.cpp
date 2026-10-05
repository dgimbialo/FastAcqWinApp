#include "pch.h"
#include "Lang.h"
#include "TraceTab.h"
#include "AppMessages.h"
#include "Dpi.h"
#include "ProtocolDefs.h"
#include "Theme.h"
#include "resource.h"

#include <cstring>

// ---------------------------------------------------------------------------
// Trace decoding (contract: firmware trace_defs.h)
// ---------------------------------------------------------------------------
LPCTSTR TraceEventName(uint16_t ev)
{
    switch (ev) {
    case TR_CYCLE_START:     return _T("CYCLE_START");
    case TR_CHIRP_PARAMS:    return _T("CHIRP_PARAMS");
    case TR_CHIRP_TABLE:     return _T("CHIRP_TABLE");
    case TR_CHIRP_ARMED:     return _T("CHIRP_ARMED");
    case TR_CAPTURE_START:   return _T("CAPTURE_START");
    case TR_VSYNC_HIGH:      return _T("VSYNC_HIGH");
    case TR_CHIRP_NEXT:      return _T("CHIRP_NEXT");
    case TR_CHIRP_DONE:      return _T("CHIRP_DONE");
    case TR_DMA_CHUNK:       return _T("DMA_CHUNK");
    case TR_MDMA_DONE:       return _T("MDMA_DONE");
    case TR_CAPTURE_DONE:    return _T("CAPTURE_DONE");
    case TR_CAPTURE_TIMEOUT: return _T("CAPTURE_TIMEOUT");
    case TR_CAPTURE_ABORT:   return _T("CAPTURE_ABORT");
    case TR_MEM_STATE:       return _T("MEM_STATE");
    case TR_FFT_START:       return _T("FFT_START");
    case TR_FFT_DONE:        return _T("FFT_DONE");
    case TR_TX_START:        return _T("TX_START");
    case TR_TX_DONE:         return _T("TX_DONE");
    case TR_TX_SKIP:         return _T("TX_SKIP");
    case TR_CYCLE_END:       return _T("CYCLE_END");
    case TR_CMD_RX:          return _T("CMD_RX");
    case TR_ERROR:           return _T("ERROR");
    case TR_PHASE_A:         return _T("PHASE_A");
    case TR_MARK:            return _T("MARK");
    case TR_VSYNC_LOW:       return _T("VSYNC_LOW");
    case TR_FSM_CAPTURING:   return _T("FSM_CAPTURING");
    default:                 return _T("?");
    }
}

static LPCTSTR ModeName(uint32_t m)
{
    return (m == MODE_IDLE) ? _T("IDLE") : (m == MODE_CONTINUOUS) ? _T("CONTINUOUS")
         : (m == MODE_SINGLE) ? _T("SINGLE") : _T("?");
}

static LPCTSTR AckName(uint32_t st)
{
    static LPCTSTR k[] = { _T("OK"), _T("BAD_ARG"), _T("BAD_STATE"), _T("HW_FAIL") };
    return st < 4 ? k[st] : _T("?");
}

CString TraceEventDetails(const TraceRecord& r)
{
    CString s;
    switch (r.ev) {
    case TR_CYCLE_START:
        s.Format(_T("cycle #%u, mode %s, trigger: %s"), r.a, ModeName(r.b),
                 r.c == 1 ? _T("host TRIGGER command") : r.c == 2 ? _T("CONTINUOUS timer") : _T("?"));
        break;
    case TR_CHIRP_PARAMS:
        s.Format(_T("requested %u Hz, actual %u.%03u Hz, %u TIM7 ticks per DAC sample"),
                 r.a, r.b / 1000, r.b % 1000, r.c);
        break;
    case TR_CHIRP_TABLE:
        s.Format(_T("DAC table L=%u samples, rise %u samples, amplitude %u counts"), r.a, r.b, r.c);
        break;
    case TR_CHIRP_ARMED:
        s.Format(_T("chirp armed: capture target %u samples, burst %u, DMA2 NDTR=%u"), r.a, r.b, r.c);
        break;
    case TR_CAPTURE_START:
        s.Format(_T("DCMI/DMA armed: target %u samples, chunk %u, DCMI.CR=0x%08X, VSYNC in 30 ms"), r.a, r.b, r.c);
        break;
    case TR_VSYNC_HIGH:
        s.Format(_T("VSYNC HIGH + TIM7 started (DAC chirp begins): ARR=%u, target %u, DMA2 NDTR=%u"), r.a, r.b, r.c);
        break;
    case TR_CHIRP_NEXT:
        s.Format(_T("burst: %u chirps left, captured so far %u"), r.a, r.b);
        break;
    case TR_CHIRP_DONE:
        s.Format(_T("DAC chirp finished (callback #%u), captured %u, DMA1 NDTR=%u"), r.a, r.b, r.c);
        break;
    case TR_DMA_CHUNK:
        s.Format(_T("DMA chunk #%u -> captured %u%s"), r.a, r.b,
                 r.c ? _T("  [MDMA OVERRUN: chunk skipped]") : _T(""));
        break;
    case TR_MDMA_DONE:
        s.Format(_T("SDRAM write offset %u, MDMA total %u us%s"), r.a, r.b,
                 r.c ? _T(", capture finished -> ready") : _T(""));
        break;
    case TR_CAPTURE_DONE:
        s.Format(_T("capture done: %u samples in %u ms, SDRAM offset %u"), r.a, r.b, r.c);
        break;
    case TR_CAPTURE_TIMEOUT:
        s.Format(_T("TIMEOUT: %u / %u samples, DMA1 NDTR=%u"), r.a, r.b, r.c);
        break;
    case TR_CAPTURE_ABORT:
        s.Format(_T("ABORT: %u / %u samples, SDRAM offset %u"), r.a, r.b, r.c);
        break;
    case TR_MEM_STATE:
        s.Format(_T("DMA chunks %u, MDMA overruns %u, chirp callbacks %u"), r.a, r.b, r.c);
        break;
    case TR_FFT_START:
        s.Format(_T("FFT start: N=%u of %u samples, window %s"), r.a, r.b,
                 r.c == 1 ? _T("Hann") : r.c == 3 ? _T("Blackman") : _T("rectangular"));
        break;
    case TR_FFT_DONE: {
        float mag = 0.0f; std::memcpy(&mag, &r.b, sizeof(mag));
        s.Format(_T("FFT done: peak bin %u, magnitude %.1f, %u us"), r.a, mag, r.c);
        break;
    }
    case TR_TX_START:
        s.Format(_T("USB TX start: raw %u B, fft %u B, frame #%u"), r.a, r.b, r.c);
        break;
    case TR_TX_DONE:
        s.Format(_T("USB TX done: CRC32 0x%08X, %u ms"), r.a, r.b);
        break;
    case TR_TX_SKIP:
        if (r.a == 1) s = _T("frame not sent: no host connected");
        else          s.Format(_T("frame not sent: TX error at stage %u"), r.b);
        break;
    case TR_CYCLE_END:
        s.Format(_T("cycle end: %u us post-capture, frames sent %u, errors %u"), r.a, r.b, r.c);
        break;
    case TR_CMD_RX:
        s.Format(_T("cmd 0x%02X arg1=%u arg2=%u -> %s"), r.a, r.b & 0xFFFF, r.b >> 16, AckName(r.c));
        break;
    case TR_ERROR:
        s.Format(_T("error code %u, detail %u / %u"), r.a, r.b, r.c);
        break;
    case TR_PHASE_A:
        s.Format(_T("Phase A: UP peak bin %u, DOWN peak bin %u, N=%u"), r.a, r.b, r.c);
        break;
    case TR_VSYNC_LOW:
        s.Format(_T("VSYNC LOW (capture window closed): captured %u, chirp_done=%u, finished=%u"), r.a, r.b, r.c);
        break;
    case TR_FSM_CAPTURING:
        s.Format(_T("FSM -> CAPTURING: deadline %u ms, target %u%s"), r.a, r.b,
                 r.c ? _T(" [legacy Phase A path]") : _T(""));
        break;
    default:
        s.Format(_T("a=%u b=%u c=%u"), r.a, r.b, r.c);
        break;
    }
    return s;
}

// ---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(TraceTab, CWnd)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_ERASEBKGND()
    ON_WM_CTLCOLOR()
    ON_BN_CLICKED(IDC_TRC_CHK_ENABLE, &TraceTab::OnTraceToggle)
    ON_BN_CLICKED(IDC_TRC_BTN_SHOT,   &TraceTab::OnSingleShot)
    ON_BN_CLICKED(IDC_TRC_BTN_GET,    &TraceTab::OnGetTrace)
    ON_BN_CLICKED(IDC_TRC_BTN_CLEAR,  &TraceTab::OnClear)
    ON_BN_CLICKED(IDC_TRC_BTN_EXPORT, &TraceTab::OnExport)
END_MESSAGE_MAP()

BOOL TraceTab::CreateTab(CWnd* parent, UINT id)
{
    LPCTSTR cls = AfxRegisterWndClass(0, ::LoadCursor(nullptr, IDC_ARROW), nullptr, nullptr);
    return Create(cls, nullptr, WS_CHILD | WS_CLIPCHILDREN, CRect(0, 0, 10, 10), parent, id);
}

int TraceTab::OnCreate(LPCREATESTRUCT lpcs)
{
    if (CWnd::OnCreate(lpcs) == -1) return -1;
    m_bgBrush.CreateSolidBrush(Theme::Get().bg);
    Dpi::MakeFont(m_font, m_hWnd, 9);

    const DWORD bs = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON;
    CRect rc(0, 0, 100, 22);

    m_chkTrace.Create(TR("Trace enabled (MCU test mode)"),
                      WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, rc, this, IDC_TRC_CHK_ENABLE);
    m_btnShot.Create  (TR("Single shot"),   bs, rc, this, IDC_TRC_BTN_SHOT);
    m_btnGet.Create   (TR("Get trace"),     bs, rc, this, IDC_TRC_BTN_GET);
    m_btnClear.Create (TR("Clear"),         bs, rc, this, IDC_TRC_BTN_CLEAR);
    m_btnExport.Create(TR("Export CSV..."), bs, rc, this, IDC_TRC_BTN_EXPORT);
    m_lblSummary.Create(TR("No trace received. Enable trace, then fire a single shot."),
                        WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE | SS_ENDELLIPSIS, rc, this, IDC_TRC_LBL_SUMMARY);

    m_list.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
                  rc, this, IDC_TRC_LIST);
    m_list.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_GRIDLINES);
    const int S = static_cast<int>(Dpi::Of(m_hWnd));
    auto sc = [&](int px) { return ::MulDiv(px, S, 96); };
    m_list.InsertColumn(0, _T("#"),        LVCFMT_RIGHT, sc(44));
    m_list.InsertColumn(1, TR("t, us"),    LVCFMT_RIGHT, sc(90));
    m_list.InsertColumn(2, TR("dt, us"),   LVCFMT_RIGHT, sc(80));
    m_list.InsertColumn(3, TR("tick, ms"), LVCFMT_RIGHT, sc(80));
    m_list.InsertColumn(4, TR("ctx"),      LVCFMT_LEFT,  sc(46));
    m_list.InsertColumn(5, TR("event"),    LVCFMT_LEFT,  sc(130));
    m_list.InsertColumn(6, TR("details"),  LVCFMT_LEFT,  sc(700));

    CWnd* kids[] = { &m_chkTrace, &m_btnShot, &m_btnGet, &m_btnClear, &m_btnExport, &m_lblSummary, &m_list };
    for (auto* c : kids) c->SetFont(&m_font);

    SetConnected(false);
    ApplyTheme();
    m_tips.Attach(this);
    return 0;
}

void TraceTab::ApplyTheme()
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
    CWnd* pw = GetWindow(GW_CHILD);
    while (pw) { pw->Invalidate(); pw = pw->GetWindow(GW_HWNDNEXT); }
}

BOOL TraceTab::OnEraseBkgnd(CDC* pDC)
{
    CRect rc; GetClientRect(&rc);
    pDC->FillSolidRect(rc, Theme::Get().bg);
    return TRUE;
}

HBRUSH TraceTab::OnCtlColor(CDC* pDC, CWnd*, UINT nCtlColor)
{
    if (nCtlColor == CTLCOLOR_STATIC || nCtlColor == CTLCOLOR_BTN) {
        pDC->SetBkMode(TRANSPARENT);
        pDC->SetTextColor(Theme::Get().text);
        return static_cast<HBRUSH>(m_bgBrush.GetSafeHandle());
    }
    // Editable fields: white (light theme) so they stand out from read-only
    // read-outs, which arrive as CTLCOLOR_STATIC and keep the grey background.
    pDC->SetTextColor(Theme::Get().text);
    pDC->SetBkColor(Theme::Get().plot);
    return Theme::FieldBrush();
}

void TraceTab::OnSize(UINT t, int cx, int cy)
{
    CWnd::OnSize(t, cx, cy);
    Relayout();
}

void TraceTab::Relayout()
{
    if (!m_list.GetSafeHwnd()) return;
    CRect rc; GetClientRect(&rc);
    const int S = static_cast<int>(Dpi::Of(m_hWnd));
    auto sc = [&](int px) { return ::MulDiv(px, S, 96); };
    const int pad = sc(6), h = sc(24);
    int x = pad, y = pad;
    auto place = [&](CWnd& w, int ww) { ww = Dpi::FitWidth(w, ww); w.MoveWindow(x, y, ww, h); x += ww + pad; };
    place(m_chkTrace, sc(220));
    place(m_btnShot,  sc(90));
    place(m_btnGet,   sc(90));
    place(m_btnClear, sc(70));
    place(m_btnExport, sc(110));
    m_lblSummary.MoveWindow(x, y, (std::max)(sc(50), rc.Width() - x - pad), h);
    const int top = pad + h + pad;
    m_list.MoveWindow(0, top, rc.Width(), (std::max)(10, rc.Height() - top));
}

void TraceTab::SetConnected(bool c)
{
    m_connected = c;
    m_chkTrace.EnableWindow(c && m_traceAvail);
    m_btnShot.EnableWindow(c);
    m_btnGet.EnableWindow(c && m_traceAvail);
}

void TraceTab::SetDeviceTraceState(bool enabled, bool available)
{
    m_traceAvail = available;
    if (m_chkTrace.GetSafeHwnd()) {
        m_chkTrace.SetCheck(enabled ? BST_CHECKED : BST_UNCHECKED);
        m_chkTrace.SetWindowText(available ? TR("Trace enabled (MCU test mode)")
                                           : TR("Trace not compiled into firmware"));
    }
    SetConnected(m_connected);
}

bool TraceTab::IsTraceRequested() const
{
    return m_chkTrace.GetSafeHwnd() && m_chkTrace.GetCheck() == BST_CHECKED;
}

void TraceTab::PostToMain(UINT msg, WPARAM wp, LPARAM lp)
{
    CWnd* pMain = AfxGetMainWnd();
    if (pMain && ::IsWindow(pMain->GetSafeHwnd()))
        pMain->PostMessage(msg, wp, lp);
}

void TraceTab::OnTraceToggle() { PostToMain(WM_APP_CMD_SET_TRACE, IsTraceRequested() ? 1 : 0); }
void TraceTab::OnSingleShot()  { PostToMain(WM_APP_CMD_SINGLE_SHOT); }
void TraceTab::OnGetTrace()    { PostToMain(WM_APP_CMD_GET_TRACE); }

void TraceTab::OnClear()
{
    m_records.clear();
    m_list.DeleteAllItems();
    m_lblSummary.SetWindowText(TR("Cleared."));
}

// ---------------------------------------------------------------------------
void TraceTab::ShowTrace(const ChirpFrame& f)
{
    m_hdr = f.header;
    m_records.clear();
    // Records travel in the raw section; ChirpFrame::raw is uint16 storage.
    const size_t bytes = f.raw.size() * sizeof(uint16_t);
    const size_t n     = bytes / TRACE_RECORD_SIZE;
    m_records.resize(n);
    if (n) std::memcpy(m_records.data(), f.raw.data(), n * TRACE_RECORD_SIZE);
    Rebuild();
}

void TraceTab::Rebuild()
{
    m_list.SetRedraw(FALSE);
    m_list.DeleteAllItems();

    const uint32_t t0 = m_records.empty() ? 0 : m_records.front().t_us;
    uint32_t prev = t0;
    int row = 0;
    CString s;
    for (const auto& r : m_records) {
        s.Format(_T("%d"), row + 1);
        const int it = m_list.InsertItem(row, s);
        s.Format(_T("%u"), r.t_us - t0);        m_list.SetItemText(it, 1, s);
        s.Format(_T("+%u"), r.t_us - prev);     m_list.SetItemText(it, 2, s);
        s.Format(_T("%u"), r.tick_ms);          m_list.SetItemText(it, 3, s);
        m_list.SetItemText(it, 4, r.ctx ? _T("ISR") : _T("main"));
        m_list.SetItemText(it, 5, TraceEventName(r.ev));
        m_list.SetItemText(it, 6, TraceEventDetails(r));
        prev = r.t_us;
        ++row;
    }
    m_list.SetRedraw(TRUE);
    m_list.Invalidate();

    const uint32_t span = m_records.empty() ? 0 : (m_records.back().t_us - t0);
    s.Format(_T("%u records (%u dropped), span %u.%03u ms, capture tick %u ms, chirp %u Hz, ")
             _T("amp %u, burst %u, rise/fall %u/%u us"),
             static_cast<unsigned>(m_records.size()), m_hdr.fft_peak_bin,
             span / 1000, span % 1000, m_hdr.timestamp_ms, m_hdr.chirp_freq_hz,
             HeaderAmplitude(m_hdr), HeaderBurst(m_hdr), HeaderRiseUs(m_hdr), HeaderFallUs(m_hdr));
    m_lblSummary.SetWindowText(s);
}

void TraceTab::OnExport()
{
    if (m_records.empty()) { AfxMessageBox(TR("No trace to export.")); return; }
    CString defName;
    defName.Format(_T("trace_%u.csv"), m_hdr.timestamp_ms);
    CFileDialog dlg(FALSE, _T("csv"), defName, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
                    TR("CSV files (*.csv)|*.csv|All files (*.*)|*.*||"), this);
    if (dlg.DoModal() != IDOK) return;

    CStdioFile fp;
    if (!fp.Open(dlg.GetPathName(), CFile::modeCreate | CFile::modeWrite | CFile::typeText)) {
        AfxMessageBox(TR("Cannot create file."));
        return;
    }
    CString line;
    line.Format(_T("# capture_tick_ms,%u\n# chirp_freq_hz,%u\n# amplitude,%u\n# burst,%u\n# rise_us,%u\n# fall_us,%u\n# dropped,%u\n"),
                m_hdr.timestamp_ms, m_hdr.chirp_freq_hz, HeaderAmplitude(m_hdr), HeaderBurst(m_hdr),
                HeaderRiseUs(m_hdr), HeaderFallUs(m_hdr), m_hdr.fft_peak_bin);
    fp.WriteString(line);
    fp.WriteString(_T("index,t_us,dt_us,tick_ms,ctx,event,a,b,c,details\n"));
    const uint32_t t0 = m_records.front().t_us;
    uint32_t prev = t0;
    int i = 1;
    for (const auto& r : m_records) {
        CString det = TraceEventDetails(r);
        det.Replace(_T("\""), _T("'"));
        line.Format(_T("%d,%u,%u,%u,%s,%s,%u,%u,%u,\"%s\"\n"), i++, r.t_us - t0, r.t_us - prev,
                    r.tick_ms, r.ctx ? _T("ISR") : _T("main"), TraceEventName(r.ev),
                    r.a, r.b, r.c, det.GetString());
        fp.WriteString(line);
        prev = r.t_us;
    }
    fp.Close();
}

BOOL TraceTab::PreTranslateMessage(MSG* pMsg)
{
    m_tips.Relay(pMsg);
    return CWnd::PreTranslateMessage(pMsg);
}
