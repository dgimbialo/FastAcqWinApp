#!/bin/sh
# Syntax-checks the MFC sources with mingw-w64 headers + the MFC stub (tests/mfcstub).
# Usage: tests/syntax-check.sh   (requires: apt install mingw-w64)
cd "$(dirname "$0")/.." || exit 1
CXX=$(command -v x86_64-w64-mingw32-g++-posix || command -v x86_64-w64-mingw32-g++)
rc=0
for f in FastAcq/*.cpp; do
  case "$f" in *pch.cpp) continue;; esac
  if ! $CXX -std=c++17 -fsyntax-only -Wall -Wno-unknown-pragmas -Wno-format -Wno-unused-parameter -Wno-nonnull-compare \
       -DUNICODE -D_UNICODE -D_WIN32_WINNT=0x0A00 -DWINVER=0x0A00 -DWIN32_LEAN_AND_MEAN \
       -isystem tests/mfcstub -I FastAcq "$f" 2>&1 | sed "s|^|$f: |" | grep -E "error|warning" ; then :; fi
  $CXX -std=c++17 -fsyntax-only -Wno-unknown-pragmas -Wno-format -DUNICODE -D_UNICODE -D_WIN32_WINNT=0x0A00 -DWINVER=0x0A00 -DWIN32_LEAN_AND_MEAN \
       -isystem tests/mfcstub -I FastAcq "$f" >/dev/null 2>&1 || rc=1
done
exit $rc
