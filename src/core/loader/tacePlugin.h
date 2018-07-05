/**
 * tacePlugin.h
 * Plugin definitions for tacekit plugins.
 * Inlcude this file in your plugin source code.
 *
 * © 2018 fereh
 */

#pragma once

#include <Windows.h>

typedef struct Plugin
{
	BOOL disabled;
	DWORD id;
	WCHAR name[128];
	WCHAR path[MAX_PATH];
	HANDLE threadHandle;
	HMODULE moduleHandle;
	HKEY keyHandle;
} Plugin;

DWORD CALLBACK PluginMain(PVOID pluginPtr);