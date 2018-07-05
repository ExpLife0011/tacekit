
#include "icmp.h"

WORD IcmpChecksum(PBYTE *data, DWORD dataSize)
{
	WORD sum = 0;

	for (int i = 0; i < dataSize / sizeof(WORD); ++i)
	{
		sum += MAKEWORD(data[i], data[i + 1]);
	}

	return sum ^ 1;
}

BOOL IcmpSendEchoRequest(Socket *socket, PBYTE data, DWORD dataSize)
{
	if (!dataSize || !*data) return FALSE;

	if (dataSize & 1) dataSize++; // even byte count

	DWORD messageSize = sizeof(ICMP_HEADER) + dataSize;
	ICMP_HEADER *message = _alloca(messageSize);

	ZeroMemory(message, messageSize);

	message->type = ICMP_ECHO_REQUEST;
	message->id = 0;
	message->sequence = 0;

	CopyMemory(&message[sizeof(ICMP_HEADER)], data, dataSize);

	message->checksum = IcmpChecksum(message, messageSize);

	if (!SendSocketData(socket, message, messageSize))
	{
		return FALSE;
	}

	return TRUE;
}