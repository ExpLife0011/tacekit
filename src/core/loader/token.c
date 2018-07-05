
#include "token.h"
#include <strsafe.h>

#define MAX_NAME 128


BOOL EnableTokenPrivileges(HANDLE tokenHandle, PCWSTR privNames[], DWORD privSize, BOOL enable)
{
	BOOL closeToken = FALSE;
	TOKEN_PRIVILEGES *tokenPrivileges;
	LUID privLuid;
	unsigned privCount = privSize / sizeof(PWSTR);
	unsigned i = 0;

	if (!privSize || privNames == NULL || *privNames == NULL)
	{
		return FALSE;
	}

	if (tokenHandle == NULL)
	{
		if (!OpenProcessToken(
			GetCurrentProcess(),
			TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY | TOKEN_QUERY_SOURCE,
			&tokenHandle))
		{
			return FALSE;
		}
	}

	tokenPrivileges = _alloca(privCount * sizeof(LUID_AND_ATTRIBUTES) + sizeof(DWORD));

	for (WORD i = 0; i < privCount; ++i)
	{
		if (!LookupPrivilegeValueW(NULL, privNames[i], &privLuid))
		{
			if (closeToken) CloseHandle(tokenHandle);
			return FALSE;
		}

		tokenPrivileges->PrivilegeCount = i + 1;
		tokenPrivileges->Privileges[i].Luid = privLuid;
		tokenPrivileges->Privileges[i].Attributes = (enable ? SE_PRIVILEGE_ENABLED : 0);
	}

	AdjustTokenPrivileges(
		tokenHandle,
		FALSE,
		tokenPrivileges,
		0,
		NULL,
		0);

	if (GetLastError() != ERROR_SUCCESS)
	{
		if (closeToken) CloseHandle(tokenHandle);
		return FALSE;
	}

	if (closeToken) CloseHandle(tokenHandle);
	return TRUE;
}

BOOL GetTokenAccountName(HANDLE tokenHandle, PWCHAR buffer, DWORD bufferSize)
{
	BOOL closeToken = FALSE;
	TOKEN_USER *tokenUser;
	SID_NAME_USE sidType;
	DWORD requiredSize = 0;
	DWORD nameSize = MAX_NAME;
	WCHAR domainName[MAX_NAME];
	WCHAR userName[MAX_NAME];

	if (tokenHandle == NULL)
	{
		closeToken = TRUE;

		if (!OpenProcessToken(
			GetCurrentProcess(),
			TOKEN_QUERY | TOKEN_QUERY_SOURCE,
			&tokenHandle))
		{
			return FALSE;
		}
	}

	if (!GetTokenInformation(
		tokenHandle,
		TokenUser,
		NULL,
		0,
		&requiredSize))
	{
		if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
		{
			if (closeToken) CloseHandle(tokenHandle);
			return FALSE;
		}
	}

	tokenUser = _alloca(requiredSize);

	if (!GetTokenInformation(
		tokenHandle,
		TokenUser,
		tokenUser,
		requiredSize,
		&requiredSize))
	{
		if (closeToken) CloseHandle(tokenHandle);
		return FALSE;
	}

	if (!LookupAccountSidW(
		NULL,
		tokenUser->User.Sid,
		userName,
		&nameSize,
		domainName,
		&nameSize,
		&sidType))
	{
		if (GetLastError() == ERROR_NONE_MAPPED)
		{
			StringCbCopyW(userName, sizeof(userName), L"NONE_MAPPED");
		}
		else
		{
			if (closeToken) CloseHandle(tokenHandle);
			return FALSE;
		}
	}

	StringCbCopyW(buffer, bufferSize, domainName);
	StringCbCatW(buffer, bufferSize, L"\\");
	StringCbCatW(buffer, bufferSize, userName);

	if (closeToken) CloseHandle(tokenHandle);
	return TRUE;
}

BOOL CheckTokenElevation(HANDLE tokenHandle, PBOOL isElevated)
{
	BOOL closeToken = FALSE;
	TOKEN_ELEVATION tokenElevation;
	DWORD requiredSize;

	*isElevated = FALSE;

	if (tokenHandle == NULL)
	{
		closeToken = TRUE;

		if (!OpenProcessToken(
			GetCurrentProcess(),
			TOKEN_QUERY | TOKEN_QUERY_SOURCE,
			&tokenHandle))
		{
			return FALSE;
		}
	}
	
	if (!GetTokenInformation(
		tokenHandle,
		TokenElevation,
		&tokenElevation,
		sizeof(TOKEN_ELEVATION),
		&requiredSize))
	{
		if (closeToken) CloseHandle(tokenHandle);
		return FALSE;
	}

	*isElevated = tokenElevation.TokenIsElevated;

	if (closeToken) CloseHandle(tokenHandle);
	return TRUE;
}