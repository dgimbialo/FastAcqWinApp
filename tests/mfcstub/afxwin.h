// ---------------------------------------------------------------------------
// Minimal MFC "look-alike" declarations used ONLY to syntax-check the FastAcq
// UI sources with a non-MSVC compiler (mingw-w64 headers provide Win32).
// Not a real MFC implementation: declarations only, no bodies, no linking.
// Signatures mirror the real MFC ones for the members the project uses.
// ---------------------------------------------------------------------------
#pragma once
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <tchar.h>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cwchar>
#include <string>

#ifndef _TRUNCATE
#define _TRUNCATE ((size_t)-1)
#endif

#define afx_msg
#define TRACE0(s) ((void)0)
#define TRACE(...) ((void)0)
#define VERIFY(f) ((void)(f))
#define ASSERT(f) ((void)0)

typedef const RECT*  LPCRECT;
#define CBRS_BOTTOM     0x2000L
#define CBRS_TOP        0x1000L
struct __POSITION {};
typedef __POSITION* POSITION;

// ---- CString ---------------------------------------------------------------
class CString {
public:
    CString() {}
    CString(const CString&) = default;
    CString(CString&&) = default;
    CString(const wchar_t* s) : m_s(s ? s : L"") {}
    CString(const char* s) { if (s) while (*s) m_s.push_back(static_cast<wchar_t>(*s++)); }
    CString(wchar_t c, int n = 1) : m_s(static_cast<size_t>(n), c) {}
    CString& operator=(const CString&) = default;
    CString& operator=(CString&&) = default;
    CString& operator=(const wchar_t* s) { m_s = s ? s : L""; return *this; }
    operator const wchar_t*() const { return m_s.c_str(); }
    const wchar_t* GetString() const { return m_s.c_str(); }
    int  GetLength() const { return static_cast<int>(m_s.size()); }
    bool IsEmpty() const { return m_s.empty(); }
    void Empty() { m_s.clear(); }
    void Format(const wchar_t*, ...) {}
    void AppendFormat(const wchar_t*, ...) {}
    CString& Trim() { return *this; }
    CString& TrimRight() { return *this; }
    CString& TrimRight(TCHAR ch) { (void)ch; return *this; }
    CString& TrimLeft(TCHAR ch) { (void)ch; return *this; }
    CString& MakeUpper() { return *this; }
    CString& MakeLower() { return *this; }
    int  Replace(wchar_t, wchar_t) { return 0; }
    int  Replace(const wchar_t*, const wchar_t*) { return 0; }
    int  Find(wchar_t) const { return -1; }
    int  Find(const wchar_t*) const { return -1; }
    int  ReverseFind(wchar_t) const { return -1; }
    CString Left(int n) const { return CString(m_s.substr(0, n < 0 ? 0 : n).c_str()); }
    CString Right(int n) const { return CString(m_s.substr(0, n < 0 ? 0 : n).c_str()); }
    CString Mid(int a) const { (void)a; return *this; }
    CString Mid(int a, int b) const { (void)a; (void)b; return *this; }
    CString& operator+=(const CString& o) { m_s += o.m_s; return *this; }
    CString& operator+=(const wchar_t* o) { if (o) m_s += o; return *this; }
    CString& operator+=(wchar_t c) { m_s += c; return *this; }
    wchar_t operator[](int i) const { return m_s[static_cast<size_t>(i)]; }
    int Compare(const wchar_t*) const { return 0; }
    int CompareNoCase(const wchar_t*) const { return 0; }
    friend CString operator+(const CString& a, const CString& b) { CString r(a); r += b; return r; }
    friend CString operator+(const CString& a, const wchar_t* b) { CString r(a); r += b; return r; }
    friend CString operator+(const wchar_t* a, const CString& b) { CString r(a); r += b; return r; }
    friend bool operator==(const CString& a, const CString& b) { return a.m_s == b.m_s; }
    friend bool operator==(const CString& a, const wchar_t* b) { return a.m_s == (b ? b : L""); }
    friend bool operator!=(const CString& a, const CString& b) { return !(a == b); }
    friend bool operator!=(const CString& a, const wchar_t* b) { return !(a == b); }
private:
    std::wstring m_s;
};
typedef CString CStringW;
class CStringA {
public:
    CStringA() {}
    CStringA(const char* s) : m_s(s ? s : "") {}
    CStringA(const CString&) {}
    operator const char*() const { return m_s.c_str(); }
    const char* GetString() const { return m_s.c_str(); }
private:
    std::string m_s;
};

// ---- Geometry --------------------------------------------------------------
class CSize : public SIZE {
public:
    CSize() { cx = cy = 0; }
    CSize(int x, int y) { cx = x; cy = y; }
    CSize(SIZE s) { cx = s.cx; cy = s.cy; }
};
class CPoint : public POINT {
public:
    CPoint() { x = y = 0; }
    CPoint(int X, int Y) { x = X; y = Y; }
    CPoint(POINT p) { x = p.x; y = p.y; }
    CPoint(LPARAM lp) { x = static_cast<short>(LOWORD(lp)); y = static_cast<short>(HIWORD(lp)); }
    void Offset(int dx, int dy) { x += dx; y += dy; }
};
class CRect : public RECT {
public:
    CRect() { left = top = right = bottom = 0; }
    CRect(int l, int t, int r, int b) { left = l; top = t; right = r; bottom = b; }
    CRect(const RECT& rc) { left = rc.left; top = rc.top; right = rc.right; bottom = rc.bottom; }
    CRect(LPCRECT rc) { *this = *rc; }
    CRect(POINT p, SIZE s) { left = p.x; top = p.y; right = p.x + s.cx; bottom = p.y + s.cy; }
    CRect& operator=(const RECT& rc) { left = rc.left; top = rc.top; right = rc.right; bottom = rc.bottom; return *this; }
    int  Width() const { return right - left; }
    int  Height() const { return bottom - top; }
    CSize Size() const { return CSize(Width(), Height()); }
    CPoint TopLeft() const { return CPoint(left, top); }
    CPoint CenterPoint() const { return CPoint((left + right) / 2, (top + bottom) / 2); }
    BOOL PtInRect(POINT p) const { return p.x >= left && p.x < right && p.y >= top && p.y < bottom; }
    BOOL IsRectEmpty() const { return Width() <= 0 || Height() <= 0; }
    void OffsetRect(int dx, int dy) { left += dx; right += dx; top += dy; bottom += dy; }
    void DeflateRect(int dx, int dy) { left += dx; right -= dx; top += dy; bottom -= dy; }
    void InflateRect(int dx, int dy) { DeflateRect(-dx, -dy); }
    void SetRect(int l, int t, int r, int b) { left = l; top = t; right = r; bottom = b; }
    void SetRectEmpty() { SetRect(0, 0, 0, 0); }
    operator LPRECT() { return this; }
    operator LPCRECT() const { return this; }
};

// ---- GDI -------------------------------------------------------------------
class CObject { public: virtual ~CObject() {} };
class CDC;
class CGdiObject : public CObject {
public:
    HGDIOBJ m_hObject{nullptr};
    HGDIOBJ GetSafeHandle() const { return this ? m_hObject : nullptr; }
    operator HGDIOBJ() const { return m_hObject; }
    BOOL DeleteObject();
    BOOL Attach(HGDIOBJ h);
    HGDIOBJ Detach();
};
class CPen : public CGdiObject {
public:
    CPen() {}
    CPen(int style, int width, COLORREF color);
    BOOL CreatePen(int style, int width, COLORREF color);
    operator HPEN() const { return static_cast<HPEN>(m_hObject); }
};
class CBrush : public CGdiObject {
public:
    CBrush() {}
    explicit CBrush(COLORREF color);
    BOOL CreateSolidBrush(COLORREF color);
    operator HBRUSH() const { return static_cast<HBRUSH>(m_hObject); }
};
class CFont : public CGdiObject {
public:
    BOOL CreateFont(int nHeight, int nWidth, int nEscapement, int nOrientation, int nWeight, BYTE bItalic,
                    BYTE bUnderline, BYTE cStrikeOut, BYTE nCharSet, BYTE nOutPrecision, BYTE nClipPrecision,
                    BYTE nQuality, BYTE nPitchAndFamily, LPCTSTR lpszFacename);
    BOOL CreatePointFont(int nPointSize, LPCTSTR lpszFaceName, CDC* pDC = nullptr);
    BOOL CreateFontIndirect(const LOGFONT* lf);
    operator HFONT() const { return static_cast<HFONT>(m_hObject); }
};
class CBitmap : public CGdiObject {
public:
    BOOL CreateCompatibleBitmap(CDC* pDC, int nWidth, int nHeight);
    HBITMAP Detach();
    operator HBITMAP() const { return static_cast<HBITMAP>(m_hObject); }
};
class CRgn : public CGdiObject {
public:
    BOOL CreateRectRgn(int x1, int y1, int x2, int y2);
    BOOL CreateRectRgnIndirect(LPCRECT lpRect);
    operator HRGN() const { return static_cast<HRGN>(m_hObject); }
};

class CWnd;
class CMenu;
class CDC : public CObject {
public:
    HDC m_hDC{nullptr};
    HDC GetSafeHdc() const { return m_hDC; }
    operator HDC() const { return m_hDC; }
    static CDC* FromHandle(HDC hDC);
    BOOL CreateCompatibleDC(CDC* pDC);
    BOOL DeleteDC();
    CPen*    SelectObject(CPen* pPen);
    CBrush*  SelectObject(CBrush* pBrush);
    CFont*   SelectObject(CFont* pFont);
    CBitmap* SelectObject(CBitmap* pBitmap);
    int      SelectObject(CRgn* pRgn);
    CGdiObject* SelectStockObject(int nIndex);
    void FillSolidRect(LPCRECT lpRect, COLORREF clr);
    void FillSolidRect(int x, int y, int cx, int cy, COLORREF clr);
    void FillRect(LPCRECT lpRect, CBrush* pBrush);
    CPoint MoveTo(int x, int y);
    CPoint MoveTo(POINT point);
    BOOL LineTo(int x, int y);
    BOOL LineTo(POINT point);
    BOOL Polyline(const POINT* lpPoints, int nCount);
    BOOL Polygon(const POINT* lpPoints, int nCount);
    BOOL Rectangle(int x1, int y1, int x2, int y2);
    BOOL Rectangle(LPCRECT lpRect);
    BOOL RoundRect(int x1, int y1, int x2, int y2, int x3, int y3);
    BOOL Ellipse(int x1, int y1, int x2, int y2);
    BOOL Ellipse(LPCRECT lpRect);
    BOOL TextOut(int x, int y, LPCTSTR lpszString, int nCount);
    BOOL TextOut(int x, int y, const CString& str);
    int  DrawText(LPCTSTR lpszString, int nCount, LPRECT lpRect, UINT nFormat);
    int  DrawText(const CString& str, LPRECT lpRect, UINT nFormat);
    int  SetBkMode(int nBkMode);
    COLORREF SetBkColor(COLORREF crColor);
    COLORREF SetTextColor(COLORREF crColor);
    CSize GetTextExtent(LPCTSTR lpszString, int nCount) const;
    CSize GetTextExtent(const CString& str) const;
    BOOL BitBlt(int x, int y, int nWidth, int nHeight, CDC* pSrcDC, int xSrc, int ySrc, DWORD dwRop);
    BOOL StretchBlt(int x, int y, int nWidth, int nHeight, CDC* pSrcDC, int xSrc, int ySrc, int nSrcWidth, int nSrcHeight, DWORD dwRop);
    int  SelectClipRgn(CRgn* pRgn);
    int  SetStretchBltMode(int nStretchMode);
    int  GetDeviceCaps(int nIndex) const;
    BOOL SetPixel(int x, int y, COLORREF crColor);
};
class CPaintDC : public CDC { public: explicit CPaintDC(CWnd* pWnd); ~CPaintDC(); PAINTSTRUCT m_ps; };
class CClientDC : public CDC { public: explicit CClientDC(CWnd* pWnd); ~CClientDC(); };
class CWindowDC : public CDC { public: explicit CWindowDC(CWnd* pWnd); ~CWindowDC(); };

// ---- Command targets / windows --------------------------------------------
class CCmdUI {
public:
    UINT m_nID{0};
    void Enable(BOOL bOn = TRUE);
    void SetCheck(int nCheck = 1);
    void SetRadio(BOOL bOn = TRUE);
    void SetText(LPCTSTR lpszText);
};
class CCmdTarget : public CObject {};
class CScrollBar;
class CWinApp;

#define DECLARE_MESSAGE_MAP() void __afx_msgmap_check();
#define BEGIN_MESSAGE_MAP(theClass, baseClass) void theClass::__afx_msgmap_check() { using ThisT = theClass; (void)sizeof(baseClass);
#define END_MESSAGE_MAP() }
#define ON_MESSAGE(id, fn)                 (void)static_cast<LRESULT (ThisT::*)(WPARAM, LPARAM)>(fn);
#define ON_REGISTERED_MESSAGE(id, fn)      (void)static_cast<LRESULT (ThisT::*)(WPARAM, LPARAM)>(fn);
#define ON_COMMAND(id, fn)                 (void)static_cast<void (ThisT::*)()>(fn);
#define ON_COMMAND_RANGE(a, b, fn)         (void)static_cast<void (ThisT::*)(UINT)>(fn);
#define ON_CONTROL(code, id, fn)           (void)static_cast<void (ThisT::*)()>(fn);
#define ON_CONTROL_RANGE(code, a, b, fn)   (void)static_cast<void (ThisT::*)(UINT)>(fn);
#define ON_UPDATE_COMMAND_UI(id, fn)       (void)static_cast<void (ThisT::*)(CCmdUI*)>(fn);
#define ON_NOTIFY(code, id, fn)            (void)static_cast<void (ThisT::*)(NMHDR*, LRESULT*)>(fn);
#define ON_NOTIFY_REFLECT(code, fn)        (void)static_cast<void (ThisT::*)(NMHDR*, LRESULT*)>(fn);
#define ON_BN_CLICKED(id, fn)              (void)static_cast<void (ThisT::*)()>(fn);
#define ON_CBN_SELCHANGE(id, fn)           (void)static_cast<void (ThisT::*)()>(fn);
#define ON_CBN_DROPDOWN(id, fn)            (void)static_cast<void (ThisT::*)()>(fn);
#define ON_EN_CHANGE(id, fn)               (void)static_cast<void (ThisT::*)()>(fn);
#define ON_EN_KILLFOCUS(id, fn)            (void)static_cast<void (ThisT::*)()>(fn);
#define ON_WM_CREATE()        (void)static_cast<int (ThisT::*)(LPCREATESTRUCT)>(&ThisT::OnCreate);
#define ON_WM_PAINT()         (void)static_cast<void (ThisT::*)()>(&ThisT::OnPaint);
#define ON_WM_ERASEBKGND()    (void)static_cast<BOOL (ThisT::*)(CDC*)>(&ThisT::OnEraseBkgnd);
#define ON_WM_SIZE()          (void)static_cast<void (ThisT::*)(UINT, int, int)>(&ThisT::OnSize);
#define ON_WM_MOUSEMOVE()     (void)static_cast<void (ThisT::*)(UINT, CPoint)>(&ThisT::OnMouseMove);
#define ON_WM_MOUSELEAVE()    (void)static_cast<void (ThisT::*)()>(&ThisT::OnMouseLeave);
#define ON_WM_LBUTTONDOWN()   (void)static_cast<void (ThisT::*)(UINT, CPoint)>(&ThisT::OnLButtonDown);
#define ON_WM_LBUTTONUP()     (void)static_cast<void (ThisT::*)(UINT, CPoint)>(&ThisT::OnLButtonUp);
#define ON_WM_LBUTTONDBLCLK() (void)static_cast<void (ThisT::*)(UINT, CPoint)>(&ThisT::OnLButtonDblClk);
#define ON_WM_RBUTTONUP()     (void)static_cast<void (ThisT::*)(UINT, CPoint)>(&ThisT::OnRButtonUp);
#define ON_WM_RBUTTONDOWN()   (void)static_cast<void (ThisT::*)(UINT, CPoint)>(&ThisT::OnRButtonDown);
#define ON_WM_MOUSEWHEEL()    (void)static_cast<BOOL (ThisT::*)(UINT, short, CPoint)>(&ThisT::OnMouseWheel);
#define ON_WM_KEYDOWN()       (void)static_cast<void (ThisT::*)(UINT, UINT, UINT)>(&ThisT::OnKeyDown);
#define ON_WM_GETDLGCODE()    (void)static_cast<UINT (ThisT::*)()>(&ThisT::OnGetDlgCode);
#define ON_WM_HSCROLL()       (void)static_cast<void (ThisT::*)(UINT, UINT, CScrollBar*)>(&ThisT::OnHScroll);
#define ON_WM_VSCROLL()       (void)static_cast<void (ThisT::*)(UINT, UINT, CScrollBar*)>(&ThisT::OnVScroll);
#define ON_WM_DESTROY()       (void)static_cast<void (ThisT::*)()>(&ThisT::OnDestroy);
#define ON_WM_CLOSE()         (void)static_cast<void (ThisT::*)()>(&ThisT::OnClose);
#define ON_WM_TIMER()         (void)static_cast<void (ThisT::*)(UINT_PTR)>(&ThisT::OnTimer);
#define ON_WM_CTLCOLOR()      (void)static_cast<HBRUSH (ThisT::*)(CDC*, CWnd*, UINT)>(&ThisT::OnCtlColor);
#define ON_WM_SETCURSOR()     (void)static_cast<BOOL (ThisT::*)(CWnd*, UINT, UINT)>(&ThisT::OnSetCursor);
#define ON_WM_SETFOCUS()      (void)static_cast<void (ThisT::*)(CWnd*)>(&ThisT::OnSetFocus);
#define ON_WM_KILLFOCUS()     (void)static_cast<void (ThisT::*)(CWnd*)>(&ThisT::OnKillFocus);
#define ON_WM_GETMINMAXINFO() (void)static_cast<void (ThisT::*)(MINMAXINFO*)>(&ThisT::OnGetMinMaxInfo);

class CWnd : public CCmdTarget {
public:
    CMenu* GetMenu() const;
    void DrawMenuBar();
    HWND m_hWnd{nullptr};
    CWnd() {}
    virtual ~CWnd() {}
    operator HWND() const { return m_hWnd; }
    HWND GetSafeHwnd() const { return this ? m_hWnd : nullptr; }
    static CWnd* FromHandle(HWND hWnd);
    virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect,
                        CWnd* pParentWnd, UINT nID, void* pContext = nullptr);
    virtual BOOL CreateEx(DWORD dwExStyle, LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle,
                          const RECT& rect, CWnd* pParentWnd, UINT nID, LPVOID lpParam = nullptr);
    virtual BOOL DestroyWindow();
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
    virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
    LRESULT Default();
    void MoveWindow(int x, int y, int nWidth, int nHeight, BOOL bRepaint = TRUE);
    void MoveWindow(LPCRECT lpRect, BOOL bRepaint = TRUE);
    BOOL SetWindowPos(const CWnd* pWndInsertAfter, int x, int y, int cx, int cy, UINT nFlags);
    BOOL ShowWindow(int nCmdShow);
    BOOL IsWindowVisible() const;
    void Invalidate(BOOL bErase = TRUE);
    void InvalidateRect(LPCRECT lpRect, BOOL bErase = TRUE);
    BOOL RedrawWindow(LPCRECT lpRectUpdate = nullptr, CRgn* prgnUpdate = nullptr,
                      UINT flags = RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
    void UpdateWindow();
    void GetClientRect(LPRECT lpRect) const;
    void GetWindowRect(LPRECT lpRect) const;
    void SetFont(CFont* pFont, BOOL bRedraw = TRUE);
    CFont* GetFont() const;
    CWnd* GetParent() const;
    CWnd* GetWindow(UINT nCmd) const;
    CWnd* GetDlgItem(int nID) const;
    int  GetDlgCtrlID() const;
    BOOL PostMessage(UINT message, WPARAM wParam = 0, LPARAM lParam = 0);
    LRESULT SendMessage(UINT message, WPARAM wParam = 0, LPARAM lParam = 0);
    UINT_PTR SetTimer(UINT_PTR nIDEvent, UINT nElapse, void (CALLBACK* lpfnTimer)(HWND, UINT, UINT_PTR, DWORD));
    BOOL KillTimer(UINT_PTR nIDEvent);
    BOOL EnableWindow(BOOL bEnable = TRUE);
    BOOL IsWindowEnabled() const;
    void SetWindowText(LPCTSTR lpszString);
    int  GetWindowText(CString& rString) const;
    int  GetWindowText(LPTSTR lpszStringBuf, int nMaxCount) const;
    int  GetWindowTextLength() const;
    void ScreenToClient(LPPOINT lpPoint) const;
    void ScreenToClient(LPRECT lpRect) const;
    void ClientToScreen(LPPOINT lpPoint) const;
    void ClientToScreen(LPRECT lpRect) const;
    CWnd* SetCapture();
    static CWnd* GetCapture();
    CWnd* SetFocus();
    static CWnd* GetFocus();
    BOOL EnableScrollBarCtrl(int nBar, BOOL bEnable = TRUE);
    BOOL SetScrollInfo(int nBar, LPSCROLLINFO lpScrollInfo, BOOL bRedraw = TRUE);
    BOOL GetScrollInfo(int nBar, LPSCROLLINFO lpScrollInfo, UINT nMask = SIF_ALL);
    int  SetScrollPos(int nBar, int nPos, BOOL bRedraw = TRUE);
    int  GetScrollPos(int nBar) const;
    void SetRedraw(BOOL bRedraw = TRUE);
    HICON SetIcon(HICON hIcon, BOOL bBigIcon);
    BOOL GetWindowPlacement(WINDOWPLACEMENT* lpwndpl) const;
    BOOL SetWindowPlacement(const WINDOWPLACEMENT* lpwndpl);
    DWORD GetStyle() const;
    DWORD GetExStyle() const;
    BOOL ModifyStyle(DWORD dwRemove, DWORD dwAdd, UINT nFlags = 0);
    CDC* GetDC();
    int  ReleaseDC(CDC* pDC);
    void CenterWindow(CWnd* pAlternateOwner = nullptr);
    BOOL IsChild(const CWnd* pWnd) const;
    BOOL SetForegroundWindow();
    void DragAcceptFiles(BOOL bAccept = TRUE);
protected:
    afx_msg int  OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnMouseLeave();
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
    afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
    afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg UINT OnGetDlgCode();
    afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void OnDestroy();
    afx_msg void OnClose();
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
    afx_msg void OnSetFocus(CWnd* pOldWnd);
    afx_msg void OnKillFocus(CWnd* pNewWnd);
    afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
};

class CFrameWnd : public CWnd {
public:
    enum RepositionFlags { reposDefault = 0, reposQuery = 1, reposExtra = 2, reposNoPosLeftOver = 0x8000 };
    virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle = WS_OVERLAPPEDWINDOW,
                        const RECT& rect = CRect(0, 0, 0, 0), CWnd* pParentWnd = nullptr,
                        LPCTSTR lpszMenuName = nullptr, DWORD dwExStyle = 0, void* pContext = nullptr);
    void RepositionBars(UINT nIDFirst, UINT nIDLast, UINT nIDLeftOver, UINT nFlag = reposDefault,
                        LPRECT lpRectParam = nullptr, LPCRECT lpRectClient = nullptr, BOOL bStretch = TRUE);
    virtual void RecalcLayout(BOOL bNotify = TRUE);
    BOOL PreTranslateMessage(MSG* pMsg) override;
    void SetMessageText(LPCTSTR lpszText);
protected:
    afx_msg int  OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnDestroy();
    afx_msg void OnClose();
    afx_msg void OnTimer(UINT_PTR nIDEvent);
};

class CWinApp : public CCmdTarget {
public:
    CWnd* m_pMainWnd{nullptr};
    int   m_nCmdShow{SW_SHOW};
    HINSTANCE m_hInstance{nullptr};
    virtual BOOL InitInstance();
    virtual int  ExitInstance();
    virtual int  Run();
    HICON LoadIcon(UINT nIDResource) const;
    HICON LoadIcon(LPCTSTR lpszResourceName) const;
    HCURSOR LoadCursor(UINT nIDResource) const;
};

// ---- Controls --------------------------------------------------------------
class CButton : public CWnd {
public:
    BOOL Create(LPCTSTR lpszCaption, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);
    void SetCheck(int nCheck);
    int  GetCheck() const;
    UINT GetState() const;
};
class CStatic : public CWnd {
public:
    BOOL Create(LPCTSTR lpszText, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID = 0xffff);
};
class CEdit : public CWnd {
public:
    BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);
    BOOL SetReadOnly(BOOL bReadOnly = TRUE);
    void SetSel(int nStartChar, int nEndChar, BOOL bNoScroll = FALSE);
    void ReplaceSel(LPCTSTR lpszNewText, BOOL bCanUndo = FALSE);
    int  GetLineCount() const;
    int  LineIndex(int nLine = -1) const;
    void SetLimitText(UINT nMax);
};
class CComboBox : public CWnd {
public:
    BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);
    int  AddString(LPCTSTR lpszString);
    void ResetContent();
    int  SetCurSel(int nSelect);
    int  GetCurSel() const;
    int  GetCount() const;
    int  FindStringExact(int nIndexStart, LPCTSTR lpszFind) const;
    int  GetLBText(int nIndex, CString& rString) const;
};
class CScrollBar : public CWnd {};
class CSliderCtrl : public CWnd {
public:
    BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);
    void SetRange(int nMin, int nMax, BOOL bRedraw = FALSE);
    void SetPos(int nPos);
    int  GetPos() const;
    void SetTicFreq(int nFreq);
};
class CProgressCtrl : public CWnd {};
class CListCtrl : public CWnd {
public:
    BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);
    DWORD SetExtendedStyle(DWORD dwNewStyle);
    int  InsertColumn(int nCol, LPCTSTR lpszColumnHeading, int nFormat = LVCFMT_LEFT, int nWidth = -1, int nSubItem = -1);
    int  InsertItem(int nItem, LPCTSTR lpszItem);
    BOOL SetItemText(int nItem, int nSubItem, LPCTSTR lpszText);
    CString GetItemText(int nItem, int nSubItem) const;
    BOOL DeleteItem(int nItem);
    BOOL DeleteAllItems();
    int  GetItemCount() const;
    BOOL SetItemData(int nItem, DWORD_PTR dwData);
    DWORD_PTR GetItemData(int nItem) const;
    BOOL EnsureVisible(int nItem, BOOL bPartialOK);
    BOOL SetItemState(int nItem, UINT nState, UINT nMask);
    UINT GetItemState(int nItem, UINT nMask) const;
    POSITION GetFirstSelectedItemPosition() const;
    int  GetNextSelectedItem(POSITION& pos) const;
    UINT GetSelectedCount() const;
    BOOL SetBkColor(COLORREF cr);
    BOOL SetTextBkColor(COLORREF cr);
    BOOL SetTextColor(COLORREF cr);
    BOOL SetItemCountEx(int iCount, DWORD dwFlags = LVSICF_NOINVALIDATEALL);
    BOOL RedrawItems(int nFirst, int nLast);
    BOOL SetColumnWidth(int nCol, int cx);
    int  GetTopIndex() const;
    int  GetCountPerPage() const;
};
class CTabCtrl : public CWnd {
public:
    BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);
    LONG InsertItem(int nItem, TCITEM* pTabCtrlItem);
    LONG InsertItem(int nItem, LPCTSTR lpszItem);
    int  SetCurSel(int nItem);
    int  GetCurSel() const;
    void AdjustRect(BOOL bLarger, LPRECT lpRect);
    BOOL GetItem(int nItem, TCITEM* pTabCtrlItem) const;
};
class CControlBar : public CWnd {};
class CStatusBar : public CControlBar {
public:
    BOOL Create(CWnd* pParentWnd, DWORD dwStyle = WS_CHILD | WS_VISIBLE | CBRS_BOTTOM, UINT nID = 0xE801);
    BOOL SetIndicators(const UINT* lpIDArray, int nIDCount);
    void SetPaneInfo(int nIndex, UINT nID, UINT nStyle, int cxWidth);
    BOOL SetPaneText(int nIndex, LPCTSTR lpszNewText, BOOL bUpdate = TRUE);
    void SetPaneStyle(int nIndex, UINT nStyle);
};
class CMenu {
public:
    HMENU m_hMenu{nullptr};
    HMENU GetSafeHmenu() const { return m_hMenu; }
    UINT  GetMenuItemCount() const;
    UINT  GetMenuItemID(int nPos) const;
    int   GetMenuString(UINT nIDItem, CString& rString, UINT nFlags) const;
    CMenu* GetSubMenu(int nPos) const;
    BOOL  ModifyMenu(UINT nPosition, UINT nFlags, UINT_PTR nIDNewItem = 0, LPCTSTR lpszNewItem = nullptr);
    BOOL CreatePopupMenu();
    BOOL CreateMenu();
    BOOL AppendMenu(UINT nFlags, UINT_PTR nIDNewItem = 0, LPCTSTR lpszNewItem = nullptr);
    BOOL CheckMenuItem(UINT nIDCheckItem, UINT nCheck);
    BOOL TrackPopupMenu(UINT nFlags, int x, int y, CWnd* pWnd, LPCRECT lpRect = nullptr);
    BOOL DestroyMenu();
};

class CToolTipCtrl : public CWnd {
public:
    BOOL Create(CWnd* pParentWnd, DWORD dwStyle = 0);
    BOOL AddTool(CWnd* pWnd, LPCTSTR lpszText = nullptr, LPCRECT lpRectTool = nullptr, UINT_PTR nIDTool = 0);
    int  SetMaxTipWidth(int iWidth);
    void SetDelayTime(DWORD dwDuration, int iTime);
    void Activate(BOOL bActivate);
    void RelayEvent(LPMSG lpMsg);
};
#ifndef TTS_ALWAYSTIP
#define TTS_ALWAYSTIP 0x01
#define TTS_NOPREFIX  0x02
#define TTDT_AUTOPOP  2
#define TTDT_INITIAL  3
#define TTDT_RESHOW   1
#endif

// ---- Files / dialogs -------------------------------------------------------
class CFileException : public CObject { public: int m_cause{0}; };
class CFile : public CObject {
public:
    enum OpenFlags {
        modeRead = 0x0000, modeWrite = 0x0001, modeReadWrite = 0x0002, shareDenyWrite = 0x0020,
        modeCreate = 0x1000, modeNoTruncate = 0x2000, typeText = 0x4000, typeBinary = 0x8000, typeUnicode = 0x400000
    };
    virtual BOOL Open(LPCTSTR lpszFileName, UINT nOpenFlags, CFileException* pError = nullptr);
    virtual void Close();
    virtual void Write(const void* lpBuf, UINT nCount);
    virtual UINT Read(void* lpBuf, UINT nCount);
};
class CStdioFile : public CFile {
public:
    virtual void WriteString(LPCTSTR lpsz);
    virtual BOOL ReadString(CString& rString);
};
class CDialog : public CWnd { public: virtual INT_PTR DoModal(); };
class CCommonDialog : public CDialog {};
class CFileDialog : public CCommonDialog {
public:
    OPENFILENAME m_ofn{};
    CFileDialog(BOOL bOpenFileDialog, LPCTSTR lpszDefExt = nullptr, LPCTSTR lpszFileName = nullptr,
                DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, LPCTSTR lpszFilter = nullptr,
                CWnd* pParentWnd = nullptr, DWORD dwSize = 0, BOOL bVistaStyle = TRUE);
    INT_PTR DoModal() override;
    CString GetPathName() const;
    CString GetFileName() const;
    CString GetFileExt() const;
};
class CWaitCursor { public: CWaitCursor(); ~CWaitCursor(); void Restore(); };

// ---- Globals ---------------------------------------------------------------
LPCTSTR AfxRegisterWndClass(UINT nClassStyle, HCURSOR hCursor = nullptr, HBRUSH hbrBackground = nullptr, HICON hIcon = nullptr);
CWnd*   AfxGetMainWnd();
CWinApp* AfxGetApp();
HINSTANCE AfxGetInstanceHandle();
HINSTANCE AfxGetResourceHandle();
int AfxMessageBox(LPCTSTR lpszText, UINT nType = MB_OK, UINT nIDHelp = 0);
int AfxMessageBox(UINT nIDPrompt, UINT nType = MB_OK, UINT nIDHelp = (UINT)-1);

#define ID_SEPARATOR               (-1)
#define AFX_IDW_CONTROLBAR_FIRST   0xE800
#define AFX_IDW_CONTROLBAR_LAST    0xE8FF
#define AFX_IDW_STATUS_BAR         0xE801
#define SBPS_NORMAL     0x0000
#define SBPS_NOBORDERS  0x0100
#define SBPS_POPOUT     0x0200
#define SBPS_DISABLED   0x0400
#define SBPS_STRETCH    0x0800
