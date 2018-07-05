
#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <strsafe.h>
#include "ntreg.h"

#define OBJ_INHERIT				0x00000002L
#define OBJ_PERMANENT			0x00000010L
#define OBJ_EXCLUSIVE			0x00000020L
#define OBJ_CASE_INSENSITIVE	0x00000040L
#define OBJ_OPENIF				0x00000080L
#define OBJ_KERNEL_HANDLE		0x00000200L
#define OBJ_FORCE_ACCESS_CHECK	0x00000400L
#define OBJ_VALID_ATTRIBUTES    0x000007F2L

#define InitializeObjectAttributes(p, n, a, r, s) \
{ \
    (p)->Length = sizeof(OBJECT_ATTRIBUTES); \
    (p)->RootDirectory = r; \
    (p)->Attributes = a; \
    (p)->ObjectName = n; \
    (p)->SecurityDescriptor = s; \
    (p)->SecurityQualityOfService = NULL; \
}

static PCWSTR __inline FindPathFileName(PCWSTR filePath);
static PCWSTR GetBasePath(HKEY baseKey);

NTSTATUS NtRegCreateKey(PHANDLE keyHandle, HKEY baseKey, PCWSTR subKey, DWORD attributes)
{
	NTSTATUS status;
	UNICODE_STRING objectName;
	OBJECT_ATTRIBUTES objectAttributes;
	ULONG dispostion;

	WCHAR keyName[MAX_PATH];

	*keyHandle = NULL;

	if (baseKey != NULL)
	{
		StringCbCopyW(keyName, sizeof(keyName), GetBasePath(baseKey));
		StringCbCatW(keyName, sizeof(keyName), L"\\");
		StringCbCatW(keyName, sizeof(keyName), subKey);
	}
	else
	{
		StringCbCopyW(keyName, sizeof(keyName), subKey);
	}
	// TODO: verify path

	RtlInitUnicodeString(&objectName, keyName);

	if (attributes & KEY_ATTRIBUTE_HIDDEN)
	{ // add extra null terminator
		objectName.MaximumLength = objectName.Length += sizeof(WCHAR);
	}

	InitializeObjectAttributes(
		&objectAttributes,
		&objectName,
		OBJ_CASE_INSENSITIVE,
		NULL,
		NULL);

	status = NtOpenKey(
		keyHandle,
		GENERIC_ALL,
		&objectAttributes);

	if (status != STATUS_SUCCESS)
	{
		if (status == STATUS_OBJECT_NAME_NOT_FOUND)
		{
			status = NtCreateKey(
				keyHandle,
				GENERIC_ALL,
				&objectAttributes,
				0,
				NULL,
				REG_OPTION_NON_VOLATILE,
				&dispostion);

			if (status != STATUS_SUCCESS)
			{
				return status;
			}
		}
		else
		{
			return status;
		}
	}

	// TODO: Revert lastWriteTime

	NtFlushKey(keyHandle);
	return STATUS_SUCCESS;
}

NTSTATUS NtRegGetValue(HANDLE keyHandle, PCWSTR name, DWORD type, PVOID data, DWORD dataSize)
{
	NTSTATUS status;
	UNICODE_STRING usName;

	KEY_VALUE_PARTIAL_INFORMATION *info;
	DWORD infoSize;

	*(PVOID *)data = NULL; // for quick error checks
	RtlInitUnicodeString(&usName, name);

	status = NtQueryValueKey(
		keyHandle,
		&usName,
		KeyValuePartialInformation,
		NULL,
		0,
		&infoSize);

	if (status != STATUS_BUFFER_TOO_SMALL)
	{
		return status;
	}

	info = _alloca(infoSize);

	status = NtQueryValueKey(
		keyHandle,
		&usName,
		KeyValuePartialInformation,
		info,
		infoSize,
		&infoSize);

	if (status != STATUS_SUCCESS)
	{
		return status;
	}

	if (info->Type != type)
	{
		return STATUS_OBJECT_TYPE_MISMATCH;
	}

	if (dataSize < info->DataLength)
	{
		return STATUS_BUFFER_TOO_SMALL;
	}

	StringCbCopyW(data, dataSize, (PWSTR)info->Data);

	return STATUS_SUCCESS;
}

NTSTATUS NtRegSetValue(HANDLE keyHandle, PCWSTR valueName, DWORD type, PVOID data, DWORD dataSize)
{
	NTSTATUS status;
	UNICODE_STRING ValueName;

	RtlInitUnicodeString(&ValueName, valueName);

	status = NtSetValueKey(
		keyHandle,
		&ValueName,
		0,
		type,
		data,
		dataSize);

	NtFlushKey(keyHandle);

	// TODO: Revert lastWriteTime

	return status;
}

NTSTATUS NtRegEnumKey(HANDLE keyHandle, void (CALLBACK *callback)(HANDLE))
{
	NTSTATUS status;

	WORD subKeyCount = 0;

	HANDLE subKeyHandle;
	WCHAR subKeyPath[MAX_PATH];
	KEY_BASIC_INFORMATION *subKeyInfo;
	DWORD subKeyInfoSize;

	while (TRUE)
	{
		status = NtEnumerateKey(
			keyHandle,
			subKeyCount,
			KeyBasicInformation,
			NULL,
			0,
			&subKeyInfoSize);

		if (status != STATUS_BUFFER_TOO_SMALL)
		{
			break;
		}
		
		subKeyInfo = _alloca(subKeyInfoSize + sizeof(WCHAR));

		status = NtEnumerateKey(
			keyHandle,
			subKeyCount,
			KeyBasicInformation,
			subKeyInfo,
			subKeyInfoSize,
			&subKeyInfoSize);

		if (status != STATUS_SUCCESS)
		{
			break;
		}

		subKeyInfo->Name[subKeyInfo->NameLength / sizeof(WCHAR)] = L'\0';

		NtRegGetKeyPath(keyHandle, subKeyPath, sizeof(subKeyPath));
		StringCbCatW(subKeyPath, sizeof(subKeyPath), L"\\");
		StringCbCatW(subKeyPath, sizeof(subKeyPath), subKeyInfo->Name);

		status = NtRegCreateKey(
			&subKeyHandle,
			NULL,
			subKeyPath,
			KEY_ATTRIBUTE_NORMAL);

		if (status != STATUS_SUCCESS)
		{
			break;
		}

		callback(subKeyHandle);

		CloseHandle(subKeyHandle);
		subKeyCount++;
	}

	if (status != STATUS_NO_MORE_ENTRIES)
	{
		return status;
	}

	return STATUS_SUCCESS;
}

NTSTATUS NtRegGetKeyPath(HANDLE keyHandle, PWSTR buffer, DWORD bufferSize)
{
	*buffer = (WCHAR)NULL;

	NTSTATUS status;
	KEY_NAME_INFORMATION *keyName;
	DWORD size;

	status = NtQueryKey(
		keyHandle,
		KeyNameInformation,
		NULL,
		0,
		&size);

	if (status != STATUS_BUFFER_TOO_SMALL) // success status
	{
		return status;
	}

	keyName = _alloca(size + sizeof(WCHAR));

	status = NtQueryKey(
		keyHandle,
		KeyNameInformation,
		keyName,
		size,
		&size);

	if (status != STATUS_SUCCESS)
	{
		return status;
	}

	if (keyName->NameLength > bufferSize)
	{
		return STATUS_BUFFER_TOO_SMALL;
	}

	keyName->Name[keyName->NameLength / sizeof(WCHAR)] = L'\0';
	StringCbCopyW(buffer, bufferSize, keyName->Name);

	return STATUS_SUCCESS;
}

NTSTATUS NtRegGetKeyName(HANDLE keyHandle, PWSTR buffer, DWORD bufferSize)
{
	*buffer = (WCHAR)NULL;

	NTSTATUS status;
	WCHAR path[MAX_PATH]; // TODO: this is wasteful, redo later

	status = NtRegGetKeyPath(keyHandle, path, sizeof(path));

	if (status != STATUS_SUCCESS)
	{
		return status;
	}

	StringCbCopyW(buffer, bufferSize, FindPathFileName(path));

	return STATUS_SUCCESS;
}


PCWSTR GetBasePath(HKEY baseKey)
{
	PWSTR basePath;

	if (baseKey == HKEY_LOCAL_MACHINE)
	{
		basePath = L"\\REGISTRY\\MACHINE";
	}
	else if (baseKey == HKEY_CLASSES_ROOT)
	{
		basePath = L"\\REGISTRY\\MACHINE\\SOFTWARE\\Classes";
	}
	else if (baseKey == HKEY_CURRENT_CONFIG)
	{
		basePath = L"\\REGISTRY\\MACHINE\\System\\CurrentControlSet\\Hardware Profiles\\Current";
	}
	else if (baseKey == HKEY_USERS)
	{
		basePath = L"\\REGISTRY\\USER";
	}
	else if (baseKey == HKEY_CURRENT_USER)
	{ // HKEY_CURRENT_USER not supported yet, defaulting
		basePath = L"\\REGISTRY\\USER\\.DEFAULT";
	}
	else
	{ // error
		return NULL;
	}

	return basePath;
}

PCWSTR FindPathFileName(PCWSTR filePath)
{
	return wcsrchr(filePath, L'\\') + 1;
}