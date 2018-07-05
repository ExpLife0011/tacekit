

#pragma once

#include <Windows.h>

typedef struct Pipe
{
	HANDLE inputReadHandle;
	HANDLE inputWriteHandle;
	HANDLE outputReadHandle;
	HANDLE outputWriteHandle;
} Pipe;

Pipe *OpenPipe(PCWSTR processPath);
BOOL ReadPipe(Pipe *, PBYTE data, DWORD dataSize);
BOOL WritePipe(Pipe *, PBYTE data, DWORD dataSize);
void ClosePipe(Pipe *);