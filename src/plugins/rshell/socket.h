/**
 * socket.h
 * 
 *
 */

#pragma once
#include <Windows.h>

SOCKET OpenSocket();
BOOL SendSocketData(SOCKET, PBYTE data, DWORD dataSize);
BOOL RecvSocketData(SOCKET, PBYTE data, DWORD dataSize);
void CloseSocket(SOCKET);