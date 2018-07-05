/**
 * log.h
 * Provides event logging functions.
 *
 * © 2018 fereh
 */

#pragma once

#include <Windows.h>

const enum { LOG_DEBUG, LOG_INFO, LOG_WARNING, LOG_ERROR, LOG_FATAL };

#define WriteDebugLog(...) WriteLog(LOG_DEBUG, __VA_ARGS__)
#define WriteInfoLog(...) WriteLog(LOG_INFO, __VA_ARGS__)
#define WriteWarnLog(...) WriteLog(LOG_WARNING, __VA_ARGS__)
#define WriteErrorLog(...) WriteLog(LOG_ERROR, __VA_ARGS__)
#define WriteFatalLog(...) WriteLog(LOG_FATAL, __VA_ARGS__)

BOOL CreateLog(PCWSTR filePath);
BOOL SetLog(PCWSTR filePath);
BOOL WriteLog(UINT level, PCWSTR format, ...);
//BOOL FlushLog();
void CloseLog(void);

#define OpenLog CreateLog

#ifdef _DEBUG
#define WriteDebugErrorLog(errorCode) \
	WriteDebugLog( \
		L"Error in %S on line %d (0x%x)", \
		strrchr(__FILE__, '\\') + 1, \
		__LINE__, \
		errorCode);
#else
#define WriteDebugErrorLog(errorCode) // do nothing
#endif