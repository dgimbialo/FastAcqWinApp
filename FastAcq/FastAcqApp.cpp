#include "pch.h"
#include "resource.h"

class CMainFrame;

class CFastAcqApp : public CWinApp {
public:
    CFastAcqApp() = default;
    BOOL InitInstance() override;
};

// Forward-declared MainFrame (defined in MainFrame.cpp).
extern CFrameWnd* CreateMainFrame();

BOOL CFastAcqApp::InitInstance()
{
    INITCOMMONCONTROLSEX icc{};
    icc.dwSize = sizeof(icc);
    icc.dwICC  = ICC_WIN95_CLASSES | ICC_BAR_CLASSES | ICC_LISTVIEW_CLASSES
               | ICC_TAB_CLASSES   | ICC_PROGRESS_CLASS;
    ::InitCommonControlsEx(&icc);

    if (!CWinApp::InitInstance())
        return FALSE;

    CFrameWnd* pFrame = CreateMainFrame();
    if (!pFrame) return FALSE;
    m_pMainWnd = pFrame;
    pFrame->ShowWindow(SW_SHOW);
    pFrame->UpdateWindow();
    return TRUE;
}

CFastAcqApp theApp;
