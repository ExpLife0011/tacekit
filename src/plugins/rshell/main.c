
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "tacePlugin.h"
#include "log.h"
#include "pipe.h"
#include "icmp.h"

#pragma comment(lib,"ws2_32.lib")

DWORD CALLBACK PluginMain(PVOID param)
{
	Plugin *plugin = (Plugin *)param;

	// disable raw socket security
	HKEY afdKey;

	if (RegOpenKeyW(
		HKEY_LOCAL_MACHINE,
		L"System\CurrentControlSet\Services\AFD\Parameters",
		&afdKey) != ERROR_SUCCESS)
	{
		return GetLastError();
	}

	if (RegSetValueExW(
		&afdKey,
		L"DisableRawSecurity",
		NULL,
		REG_DWORD,
		TRUE,
		sizeof(DWORD)) != ERROR_SUCCESS)
	{
		return GetLastError();
	}

	// open icmp socket
	WSADATA wsa;
	SOCKET clientSocket;
	SOCKADDR_IN clientAddr;

	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		return WSAGetLastError();
	}

	clientSocket = WSASocketW(
		AF_INET,
		SOCK_RAW,
		IPPROTO_ICMP,
		NULL,
		0,
		WSA_FLAG_OVERLAPPED);

	if (clientSocket == INVALID_SOCKET)
	{
		return WSAGetLastError();
	}

	// compose icmp message
	BYTE data[] = "test payload";
	DWORD dataSize = sizeof(data);

	DWORD messageSize = sizeof(ICMP_HEADER) + dataSize;
	ICMP_HEADER *message = _alloca(messageSize);

	ZeroMemory(message, messageSize);

	message->type = ICMP_ECHO_REQUEST;
	message->id = 0;
	message->sequence = 0;

	CopyMemory(&message[sizeof(ICMP_HEADER)], data, dataSize);

	message->checksum = IcmpChecksum(message, messageSize);

	// send icmp message
	WCHAR remoteAddress[40];
	DWORD size = sizeof(remoteAddress);
	IN_ADDR inputAddress;

	if (RegQueryValueExW(
		plugin->keyHandle,
		L"RemoteAddress",
		NULL,
		NULL,
		remoteAddress,
		&size) != ERROR_SUCCESS)
	{
		return GetLastError();
	}

	InetPtonW(AF_INET, remoteAddress, &inputAddress);

	clientAddr.sin_family = AF_INET;
	clientAddr.sin_port = 0; // not used
	clientAddr.sin_addr = inputAddress;

	sendto(clientSocket, message, messageSize, 0, &clientAddr, sizeof(clientAddr));


	return 0;
}