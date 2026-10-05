#pragma once
//
// Screenshot.h -- save a window's client area as PNG (GDI+, part of Windows).
//

#include "pch.h"

bool SaveWindowPng(HWND hwnd, const CString& path);
