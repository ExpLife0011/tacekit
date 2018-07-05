/**
 * main.c
 * Loads injector DLL and verifies impersonation.
 * Poses as victim process for injection.
 *
 * © 2018 fereh
 */

#include <Windows.h>
#include <strsafe.h>

void wmain(int argCount, PCWSTR *args)
{
	HMODULE moduleHandle;

	moduleHandle = LoadLibraryW(args[1]);

	// wait for injection
	Sleep(INFINITE);
	return;
}