/**
 * log.c
 *
 * © 2018 fereh
 */

#include <strsafe.h>
#include "log.h"
#include "helper.h"

static HANDLE gFileHandle = INVALID_HANDLE_VALUE;
static PCHAR gFileBuffer = NULL;
static DWORD gFileBufferSize = 4096;

static PCWSTR levelNames[] =
{
	L"DEBUG", L"INFO", L"WARNING", L"ERROR", L"FATAL"
};

static BOOL FormatLog(PWCHAR buffer, DWORD bufferSize, UINT level, PCWSTR format, va_list);
static BOOL FlushLog();

BOOL CreateLog(PCWSTR filePath)
{
	if (gFileBuffer) return FALSE;

	// logs are buffered until a log path is set
	gFileBuffer = HeapAlloc(
		GetProcessHeap(),
		HEAP_ZERO_MEMORY,
		gFileBufferSize);

	if (filePath != NULL)
	{
		if (!SetLog(filePath))
		{
			return FALSE;
		}
	}

	return TRUE;
}

BOOL SetLog(PCWSTR filePath)
{
	if (filePath == NULL) return FALSE;

	HANDLE prevHandle = gFileHandle;

	if (prevHandle != INVALID_HANDLE_VALUE)
	{
		FlushLog();
	}

	/*if (!CreateMissingDirectories(filePath))
	{
		return FALSE;
	}*/

	gFileHandle = CreateFileW(
		filePath,
		FILE_APPEND_DATA,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL |
		FILE_FLAG_WRITE_THROUGH,
		NULL);

	if (gFileHandle == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}

	CloseHandle(prevHandle);
	return TRUE;
}

BOOL FormatLog(PWCHAR buffer, DWORD bufferSize, UINT level, PCWSTR format, va_list args)
{
	HRESULT result;
	WCHAR message[1024];
	SYSTEMTIME time;

	result = StringCbVPrintfW(message, sizeof(message), format, args);

	if (result == STRSAFE_E_INSUFFICIENT_BUFFER)
	{
		WriteWarnLog(L"Next message truncated");
		// TODO: reallocate buffer instead of truncating
	}

	GetLocalTime(&time);

	result = StringCbPrintfW(
		buffer,
		bufferSize,
		L"%d-%02d-%02d %02d:%02d:%02d.%03d \u2014 %s \u2014 %s\r\n",
		time.wYear, time.wMonth, time.wDay,
		time.wHour, time.wMinute, time.wSecond,
		time.wMilliseconds,
		levelNames[level],
		message);

	if (result != S_OK)
	{
		return FALSE;
	}

	return TRUE;
}

BOOL WriteLog(UINT level, PCWSTR format, ...)
{
	if (!gFileBuffer) return FALSE;

	va_list args;

	WCHAR message[1024]; // TODO: dynamic allocation?

	DWORD currentSize = (DWORD)strlen(gFileBuffer); // terminator not included
	DWORD remainingSize = gFileBufferSize - currentSize;
	DWORD requiredSize = 0;

	va_start(args, format);

	if (!FormatLog(message, sizeof(message), level, format, args))
	{
		return FALSE;
	}

	va_end(args);

	if (!Utf16ToUtf8(message, NULL, &requiredSize))
	{
		return FALSE;
	}

	if (currentSize + requiredSize > gFileBufferSize)
	{
		if (!gFileHandle)
		{ // then extend buffer
			gFileBufferSize = currentSize + requiredSize;

			gFileBuffer = HeapReAlloc(
				GetProcessHeap(),
				HEAP_ZERO_MEMORY,
				gFileBuffer,
				gFileBufferSize);
		}
		else
		{
			FlushLog();

			currentSize = 0;
			remainingSize = gFileBufferSize;
		}
	}

	if (!Utf16ToUtf8(message, gFileBuffer + currentSize, &remainingSize))
	{
		return FALSE;
	}

	return TRUE;
}

BOOL FlushLog()
{
	if (!gFileBuffer) return FALSE;

	DWORD currentSize = (DWORD)strlen(gFileBuffer);
	DWORD writtenSize;

	if (!WriteFile(gFileHandle, gFileBuffer, currentSize, &writtenSize, NULL))
	{
		return FALSE;
	}

	if (currentSize != writtenSize)
	{
		return FALSE;
	}

	return TRUE;
}

void CloseLog(void)
{
	FlushLog();
	HeapFree(GetProcessHeap(), 0, gFileBuffer);
	CloseHandle(gFileHandle);
}