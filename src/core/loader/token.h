/**
 * token.h
 * Token helper functions.
 *
 * © 2018 fereh
 */

#pragma once

#include <Windows.h>

BOOL EnableTokenPrivileges(HANDLE tokenHandle, PCWSTR privNames[], DWORD privSize, BOOL enable);
BOOL GetTokenAccountName(HANDLE tokenHandle, PWCHAR buffer, DWORD bufferSize);
BOOL CheckTokenElevation(HANDLE tokenHandle, PBOOL isElevated);