/**
 * module.h
 * Just some path related helper functions.
 *
 * © 2018 fereh
 */

#pragma once

#include <Windows.h>

HMODULE GetCurrentModuleHandle(void);
BOOL GetCurrentModuleDirectory(DWORD bufferLength, PWSTR buffer);

BOOL RemovePathFileName(PWSTR filePath);
PCWSTR GetPathFileName(PCWSTR filePath);

BOOL CreateMissingDirectories(PCWSTR filePath);

BOOL Utf16ToUtf8(PCWSTR wcStr, PSTR buffer, int *bufferSize);
BOOL Utf8ToUtf16(PCSTR mbStr, PWSTR buffer, int *bufferSize);