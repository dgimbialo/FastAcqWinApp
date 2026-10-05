#include "pch.h"
#include "Screenshot.h"

#include <objidl.h>
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

namespace {

bool GetPngEncoderClsid(CLSID& clsid)
{
    UINT num = 0, size = 0;
    Gdiplus::GetImageEncodersSize(&num, &size);
    if (size == 0) return false;
    std::vector<BYTE> buf(size);
    auto* codecs = reinterpret_cast<Gdiplus::ImageCodecInfo*>(buf.data());
    Gdiplus::GetImageEncoders(num, size, codecs);
    for (UINT i = 0; i < num; ++i) {
        if (wcscmp(codecs[i].MimeType, L"image/png") == 0) { clsid = codecs[i].Clsid; return true; }
    }
    return false;
}

struct GdiplusSession {
    ULONG_PTR token{0};
    bool ok{false};
    GdiplusSession() {
        Gdiplus::GdiplusStartupInput in;
        ok = Gdiplus::GdiplusStartup(&token, &in, nullptr) == Gdiplus::Ok;
    }
    ~GdiplusSession() { if (ok) Gdiplus::GdiplusShutdown(token); }
};

} // namespace

bool SaveWindowPng(HWND hwnd, const CString& path)
{
    if (!::IsWindow(hwnd)) return false;
    RECT rc{};
    ::GetClientRect(hwnd, &rc);
    const int w = rc.right - rc.left, h = rc.bottom - rc.top;
    if (w <= 0 || h <= 0) return false;

    HDC hdcWnd = ::GetDC(hwnd);
    HDC hdcMem = ::CreateCompatibleDC(hdcWnd);
    HBITMAP hbm = ::CreateCompatibleBitmap(hdcWnd, w, h);
    HGDIOBJ old = ::SelectObject(hdcMem, hbm);
    // PrintWindow renders child windows too (PW_CLIENTONLY = 1).
    if (!::PrintWindow(hwnd, hdcMem, 1))
        ::BitBlt(hdcMem, 0, 0, w, h, hdcWnd, 0, 0, SRCCOPY);
    ::SelectObject(hdcMem, old);
    ::DeleteDC(hdcMem);
    ::ReleaseDC(hwnd, hdcWnd);

    bool ok = false;
    {
        GdiplusSession gp;
        if (gp.ok) {
            CLSID png;
            if (GetPngEncoderClsid(png)) {
                Gdiplus::Bitmap bmp(hbm, nullptr);
                ok = bmp.Save(CStringW(path), &png, nullptr) == Gdiplus::Ok;
            }
        }
    }
    ::DeleteObject(hbm);
    return ok;
}
