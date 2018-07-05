/**
 * plugin.c
 * Plugin management functions.
 *
 * © 2018 fereh
 */

#include "plugin.h"
#include "log.h"
#include "helper.h"
#include "ntreg.h"
#include <strsafe.h>

#define MAX_PLUGINS 64 // based on max wait threads
#define DEFAULT_PLUGIN_PROCNAME L"PluginMain"

static unsigned gPluginCount = 0;
static unsigned gActivePluginCount = 0;

Plugin *OpenPlugin(HANDLE pluginKey)
{
	NTSTATUS status;
	Plugin *plugin;
	
	if (gPluginCount >= MAX_PLUGINS)
	{
		return NULL;
	}

	plugin = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(Plugin));
	plugin->id = gPluginCount;

	NtRegGetDword(pluginKey, L"Disabled", &plugin->disabled);

	status = NtRegGetString(
		pluginKey,
		L"DisplayName",
		plugin->name,
		sizeof(plugin->name));

	if (status != STATUS_SUCCESS)
	{
		NtRegGetKeyName(pluginKey, plugin->name, sizeof(plugin->name));
	}

	status = NtRegGetExpandString(
		pluginKey,
		L"ImagePath",
		plugin->path,
		sizeof(plugin->path));

	if (status != STATUS_SUCCESS)
	{
		ClosePlugin(plugin);
		return NULL;
	}

	if (!DuplicateHandle(
		GetCurrentProcess(),
		pluginKey,
		GetCurrentProcess(),
		&plugin->keyHandle,
		0,
		FALSE,
		DUPLICATE_SAME_ACCESS))
	{
		ClosePlugin(plugin);
		return NULL;
	}

	gPluginCount++;

	return plugin;
}

BOOL LoadPlugin(Plugin *plugin)
{
	NTSTATUS status;

	PTHREAD_START_ROUTINE PluginProc;
	WCHAR procName[256];

	PBYTE procSymbolName;
	DWORD procSymbolNameSize = 0;

	if (!plugin || plugin->disabled)
	{
		return FALSE;
	}

	plugin->moduleHandle = LoadLibraryW(plugin->path);

	if (!plugin->moduleHandle)
	{
		if (GetLastError() == ERROR_MOD_NOT_FOUND)
		{
			WriteErrorLog(L"Module '%s' not found", GetPathFileName(plugin->path));
		}

		else WriteDebugErrorLog(GetLastError());

		return FALSE;
	}

	status = NtRegGetString(
		plugin->keyHandle,
		L"PluginMain",
		procName,
		sizeof(procName));

	if (status != STATUS_SUCCESS)
	{
		StringCbCopyW(procName, sizeof(procName), DEFAULT_PLUGIN_PROCNAME);
	}

	if (!Utf16ToUtf8(procName, NULL, &procSymbolNameSize))
	{
		WriteDebugErrorLog(GetLastError());
		return FALSE;
	}

	procSymbolName = _alloca(procSymbolNameSize);

	if (!Utf16ToUtf8(procName, procSymbolName, &procSymbolNameSize))
	{
		WriteDebugErrorLog(GetLastError());
		return FALSE;
	}

	PluginProc = (PTHREAD_START_ROUTINE)GetProcAddress(
		plugin->moduleHandle,
		procSymbolName);

	if (!PluginProc)
	{
		if (GetLastError() == ERROR_PROC_NOT_FOUND)
		{
			WriteErrorLog(L"Symbol '%s' not found", procName);
		}

		else WriteDebugErrorLog(GetLastError());
			
		return FALSE;
	}

	plugin->threadHandle = CreateThread(NULL, 0, PluginProc, plugin, 0, NULL);

	if (!plugin->threadHandle)
	{
		WriteDebugErrorLog(GetLastError());
		return FALSE;
	}

	gActivePluginCount++;

	return TRUE;
}

void UnloadPlugin(Plugin *plugin)
{
	if (!plugin || plugin->disabled) return;

	CloseHandle(plugin->threadHandle);
	CloseHandle(plugin->keyHandle);

	if (plugin->moduleHandle)
	{
		FreeLibrary(plugin->moduleHandle);
	}

	if (gActivePluginCount != 0)
	{
		gActivePluginCount--;
	}

	return;
}

void ClosePlugin(Plugin *plugin)
{
	HeapFree(GetProcessHeap(), 0, plugin);
	return;
}

unsigned GetPluginCount(void)
{
	return gPluginCount;
}

unsigned GetActivePluginCount(void)
{
	return gActivePluginCount;
}