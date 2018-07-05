/**
 * helper.c
 * Just some popular helper functions.
 *
 * © 2018 fereh
 */

#include "helper.h"
#include <strsafe.h>

HMODULE GetCurrentModuleHandle(void)
{
	HMODULE handle = NULL;

	GetModuleHandleExW(
		GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
		(PWSTR)GetCurrentModuleHandle,
		&handle);

	return handle;
}

BOOL GetCurrentModuleDirectory(DWORD bufferLength, PWSTR buffer)
{
	GetModuleFileNameW(
		GetCurrentModuleHandle(),
		buffer,
		bufferLength * sizeof(WCHAR));

	RemovePathFileName(buffer);

	return TRUE;
}

BOOL CreateMissingDirectories(PCWSTR filePath)
{
	WCHAR path[MAX_PATH];
	WCHAR *end;

	end = wcschr(filePath, L'\\');
	end = wcschr(++end, L'\\'); // skip drive

	while (end != NULL)
	{
		StringCchCopyW(path, (end - filePath) + 1, filePath);

		if (!CreateDirectoryW(path, NULL))
		{
			if (GetLastError() != ERROR_ALREADY_EXISTS)
			{
				return FALSE;
			}
		}

		end = wcschr(++end, L'\\');
	}

	return TRUE;
}

PCWSTR GetPathFileName(PCWSTR filePath)
{
	if (wcsrchr(filePath, L'\\'))
	{
		filePath = wcsrchr(filePath, L'\\') + 1;
	}

	return filePath;
}

BOOL RemovePathFileName(PWSTR filePath)
{
	if (!filePath) return FALSE;
	WCHAR *end = wcsrchr(filePath, L'\\');

	StringCchCopyW(filePath, (end - filePath) + 1, filePath);

	return TRUE;
}

BOOL Utf16ToUtf8(PCWSTR wcStr, PSTR buffer, int *bufferSize)
{
	*bufferSize = WideCharToMultiByte(
		CP_UTF8, 0,
		wcStr, -1,
		buffer, *bufferSize,
		NULL, NULL);

	if (*bufferSize == 0)
	{
		return FALSE;
	}

	return TRUE;
}

BOOL Utf8ToUtf16(PCSTR mbStr, PWSTR buffer, int *bufferSize)
{
	*bufferSize = MultiByteToWideChar(
		CP_UTF8, 0,
		mbStr, -1,
		buffer, *bufferSize);

	if (*bufferSize == 0)
	{
		return FALSE;
	}

	return TRUE;
}