/**
 * plugin.h
 * Plugin management functions.
 *
 * © 2018 fereh
 */

#pragma once

#include <Windows.h>
#include "tacePlugin.h"

Plugin *OpenPlugin(HANDLE pluginKey);
BOOL LoadPlugin(Plugin *);
void UnloadPlugin(Plugin *);
void ClosePlugin(Plugin *);

unsigned GetPluginCount(void);
unsigned GetActivePluginCount(void);