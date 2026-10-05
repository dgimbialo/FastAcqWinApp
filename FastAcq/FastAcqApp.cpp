#include "pch.h"
#include "AppSettings.h"
#include "resource.h"

class CFastAcqApp : public CWinApp {
public:
    CFastAcqApp() = default;
    BOOL InitInstance() override;
};

// Defined in MainFrameImpl.cpp.
extern CFrameWnd* CreateMainFrame();

BOOL CFastAcqApp::InitInstance()
{
    INITCOMMONCONTROLSEX icc{};
    icc.dwSize = sizeof(icc);
    icc.dwICC  = ICC_WIN95_CLASSES | ICC_BAR_CLASSES | ICC_LISTVIEW_CLASSES | ICC_TAB_CLASSES | ICC_PROGRESS_CLASS;
    ::InitCommonControlsEx(&icc);

    if (!CWinApp::InitInstance())
        return FALSE;

    // Restore the saved window placement before the frame is shown.
    AppSettings saved;
    const bool haveSaved = saved.Load(AppSettings::DefaultPath());

    CFrameWnd* pFrame = CreateMainFrame();
    if (!pFrame) return FALSE;
    m_pMainWnd = pFrame;

    int showCmd = m_nCmdShow;
    if (haveSaved) {
        const CRect& r = saved.windowRect;
        if (r.Width() > 200 && r.Height() > 200) {
            // Keep the window on a visible monitor.
            HMONITOR mon = ::MonitorFromRect(&r, MONITOR_DEFAULTTONULL);
            if (mon) pFrame->MoveWindow(&r, FALSE);
        }
        if (saved.windowMax) showCmd = SW_SHOWMAXIMIZED;
    }
    pFrame->ShowWindow(showCmd);
    pFrame->UpdateWindow();
    return TRUE;
}

CFastAcqApp theApp;
