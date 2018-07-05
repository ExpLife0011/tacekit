/**
 * main.c
 * Loads and manages plugins.
 * This library is injected by the Injector and executed by a remote process.
 *
 * © 2018 fereh
 */

#define WIN32_LEAN_AND_MEAN

#include "helper.h"
#include "log.h"
#include "ntreg.h"
#include "token.h"
#include "plugin.h"
#include <strsafe.h>

#define DEFAULT_LOG_FILE_NAME L"tacekit.log"

static HANDLE gWaitThread;
static HANDLE gStopEvent;

void CALLBACK StopCallback(PVOID param, BOOLEAN timedOut);
void CALLBACK PluginKeyCallback(HANDLE keyHandle);
void CALLBACK PluginThreadCallback(PVOID param, BOOLEAN timedOut);

DWORD CALLBACK WorkerThread(PVOID param)
{
	NTSTATUS status;

	HMODULE moduleHandle = (HMODULE)param;
	HANDLE configKey;

	WCHAR logPath[MAX_PATH];

	PCWSTR privileges[] =
	{
		SE_SECURITY_NAME,
		SE_BACKUP_NAME,
		SE_RESTORE_NAME
	};

	CreateLog(NULL); // logs buffered until path is set

	WriteInfoLog(L"Started");

#ifdef _DEBUG
	WCHAR hostName[MAX_PATH];
	WCHAR accountName[128];

	GetModuleFileNameW(NULL, hostName, sizeof(hostName));
	GetTokenAccountName(NULL, accountName, sizeof(accountName));

	WriteDebugLog(L"Hosted by '%s'", hostName);
	WriteDebugLog(L"Running as '%s' account", accountName);
#endif

	if (!EnableTokenPrivileges(
		NULL,
		privileges,
		sizeof(privileges),
		TRUE))
	{
		WriteDebugErrorLog(GetLastError());
	}

	// init registry
	status = NtRegCreateKey(
		&configKey,
		HKEY_CLASSES_ROOT,
		L"Installer\\Config",
		// TODO: change key attribute in release
		KEY_ATTRIBUTE_NORMAL);

	if (status != STATUS_SUCCESS)
	{
		WriteDebugErrorLog(status);
	}

	// init log
	status = NtRegGetString(
		configKey,
		L"LogPath",
		logPath,
		sizeof(logPath));

	if (status != STATUS_SUCCESS)
	{
		GetCurrentModuleDirectory(sizeof(logPath) / sizeof(WCHAR), logPath);
		StringCbCatW(logPath, sizeof(logPath), L"\\" DEFAULT_LOG_FILE_NAME);
	}

	SetLog(logPath);
	
	// init events
	gStopEvent = CreateEventW(NULL, TRUE, FALSE, NULL);

	// init stop
	if (!RegisterWaitForSingleObject(
		&gWaitThread,
		gStopEvent,
		StopCallback,
		moduleHandle,
		INFINITE,
		WT_EXECUTEONLYONCE))
	{
		WriteDebugErrorLog(GetLastError());

		StopCallback(NULL, FALSE);
		CloseHandle(configKey);
		return 0;
	}
	
	WriteInfoLog(L"Initialized");

	// load plugins
	status = NtRegEnumKey(configKey, PluginKeyCallback);

	if (status != STATUS_SUCCESS)
	{
		WriteDebugErrorLog(status);
		SetEvent(gStopEvent);
	}

	else if (GetPluginCount() == 0)
	{
		WriteFatalLog(L"No plugins found");
		SetEvent(gStopEvent);
	}

	else if (GetActivePluginCount() == 0)
	{
		SetEvent(gStopEvent);
	}

	CloseHandle(configKey);
	return 0;
}

void CALLBACK PluginKeyCallback(HANDLE keyHandle)
{
	Plugin *plugin = OpenPlugin(keyHandle);

	if (plugin == NULL)
	{
		return;
	}

	if (plugin->disabled)
	{
		WriteWarnLog(L"%s is disabled", plugin->name);
		ClosePlugin(plugin);
		return;
	}

	if (!LoadPlugin(plugin))
	{
		WriteInfoLog(L"Failed to load %s", plugin->name);
		ClosePlugin(plugin);
		return;
	}

	WriteInfoLog(L"Loaded %s", plugin->name);

	if (!RegisterWaitForSingleObject(
		&gWaitThread,
		plugin->threadHandle,
		PluginThreadCallback,
		plugin,
		INFINITE,
		WT_EXECUTEONLYONCE))
	{
		WriteDebugErrorLog(GetLastError());
		UnloadPlugin(plugin);
		ClosePlugin(plugin);
		return;
	}

	return;
}

void CALLBACK PluginThreadCallback(PVOID param, BOOLEAN timedOut)
{
	Plugin *plugin = (Plugin *)param;

	UnloadPlugin(plugin);
	WriteInfoLog(L"Unloaded %s", plugin->name);
	ClosePlugin(plugin);

	if (GetActivePluginCount() == 0)
	{
		SetEvent(gStopEvent);
	}

	return;
}

void CALLBACK StopCallback(PVOID param, BOOLEAN timedOut)
{
	HANDLE moduleHandle = (HANDLE)param;

	UnregisterWait(gWaitThread);
	WriteInfoLog(L"Stopped");
	CloseLog();

	return;
}

BOOL CALLBACK DllMain(HMODULE moduleHandle, DWORD reason, PVOID reserved)
{
	WCHAR modulePath[MAX_PATH];

	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(moduleHandle);
		CreateThread(NULL, 0, WorkerThread, moduleHandle, 0, NULL);
		break;

	case DLL_PROCESS_DETACH:
		if (WaitForSingleObject(gStopEvent, 0) != WAIT_OBJECT_0)
		{ // threads are still running
			WriteWarnLog(L"Unexpected detach occured");
			StopCallback(moduleHandle, TRUE);
		}
		break;
	}

	return TRUE;
}