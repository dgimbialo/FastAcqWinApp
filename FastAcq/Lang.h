#pragma once
//
// Lang.h -- user-interface language: English (source strings) or Ukrainian.
// TR("text") returns the translation of an English UI string (or the string
// itself when no translation exists), Lang::Help(id) the tooltip text of an
// input control. The language is chosen on the Settings tab and applied at
// start-up (controls are created with their final captions).
//

#include "pch.h"

namespace Lang {

enum class Id { English = 0, Ukrainian = 1, Count };

void Set(Id id);
Id   Get();
LPCTSTR Name(Id id);          // "English", "Українська"

// Translation of an English UI string; the string itself when unknown.
CString Tr(LPCTSTR english);

// Tooltip (help) text for a control id; empty when none is defined.
CString Help(UINT ctrlId);

// Translate every item caption of a menu (recursively).
void TranslateMenu(CMenu* menu);

} // namespace Lang

#define TR(s) Lang::Tr(_T(s))
