
#include "pipe.h"

Pipe *OpenPipe(PCWSTR processPath)
{
	Pipe *pipe = HeapAlloc(GetProcessHeap(), 0, sizeof(Pipe));

	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = NULL;

	if (!CreatePipe(
		&pipe->inputReadHandle,
		&pipe->inputWriteHandle,
		&sa,
		0))
	{
		ClosePipe(pipe);
		return NULL;
	}

	if (!SetHandleInformation(
		pipe->inputWriteHandle,
		HANDLE_FLAG_INHERIT,
		0))
	{
		return NULL;
	}

	if (!CreatePipe(
		&pipe->outputReadHandle,
		&pipe->outputWriteHandle,
		&sa,
		0))
	{
		ClosePipe(pipe);
		return NULL;
	}

	if (!SetHandleInformation(
		pipe->outputReadHandle,
		HANDLE_FLAG_INHERIT,
		0))
	{
		return NULL;
	}

	// create child process
	PROCESS_INFORMATION pi;
	STARTUPINFOW si;

	ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
	ZeroMemory(&si, sizeof(STARTUPINFOW));

	si.cb = sizeof(STARTUPINFOW);
	si.hStdError = pipe->outputWriteHandle;
	si.hStdOutput = pipe->outputWriteHandle;
	si.hStdInput = pipe->inputReadHandle;
	si.dwFlags |= STARTF_USESTDHANDLES;

	if (!CreateProcessW(
		processPath,
		NULL,
		NULL, NULL,
		TRUE,
		0,
		NULL, NULL,
		&si,
		&pi))
	{
		ClosePipe(pipe);
		return NULL;
	}

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return pipe;
}

BOOL ReadPipe(Pipe *pipe, PBYTE data, DWORD dataSize)
{
	DWORD bytesRead;
	DWORD bytesReady, bytesLeft;
	DWORD totalBytesRead = 0;

	ZeroMemory(data, dataSize);

	while (TRUE)
	{
		if (!ReadFile(
			pipe->outputReadHandle,
			data + totalBytesRead,
			dataSize - totalBytesRead,
			&bytesRead,
			NULL))
		{
			return FALSE;
		}

		totalBytesRead += bytesRead;

		if (!PeekNamedPipe(
			pipe->outputReadHandle,
			NULL,
			0,
			NULL,
			&bytesReady,
			&bytesLeft))
		{
			return FALSE;
		}

		if (bytesReady == 0 && bytesLeft == 0)
		{
			break;
		}
	}

	return TRUE;
}

BOOL WritePipe(Pipe *pipe, PBYTE data, DWORD dataSize)
{
	DWORD bytesWritten;

	while (TRUE)
	{
		if (!WriteFile(pipe->inputWriteHandle, data, dataSize, &bytesWritten, NULL))
		{
			return FALSE;
		}
	}

	return TRUE;
}

void ClosePipe(Pipe *pipe)
{
	CloseHandle(pipe->inputReadHandle);
	CloseHandle(pipe->inputWriteHandle);
	CloseHandle(pipe->outputReadHandle);
	CloseHandle(pipe->outputWriteHandle);

	HeapFree(GetProcessHeap(), 0, pipe);
	return;
}