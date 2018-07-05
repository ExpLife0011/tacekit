
#include "socket.h"

static WSADATA gWsaData;

SOCKET OpenSocket()
{
	SOCKET clientSocket;

	if (WSAStartup(MAKEWORD(2, 2), &gWsaData) != 0)
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
}