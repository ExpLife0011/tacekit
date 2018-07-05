/**
 * ntreg.h
 * Registry manipulation using the NTAPI for more control.
 *
 * © 2018 fereh
 */

// TODO: rewrite this library, needs more control

#pragma once

#include <Windows.h>
#include <SubAuth.h>

#pragma comment (lib, "ntdll.lib")

typedef enum _KEY_INFORMATION_CLASS {
	KeyBasicInformation,
	KeyNodeInformation,
	KeyFullInformation,
	KeyNameInformation,
	KeyCachedInformation,
	KeyFlagsInformation,
	KeyVirtualizationInformation,
	KeyHandleTagsInformation,
	KeyTrustInformation,
	KeyLayerInformation,
	MaxKeyInfoClass
} KEY_INFORMATION_CLASS;

typedef enum _KEY_VALUE_INFORMATION_CLASS {
	KeyValueBasicInformation,
	KeyValueFullInformation,
	KeyValuePartialInformation,
	KeyValueFullInformationAlign64,
	KeyValuePartialInformationAlign64,
	KeyValueLayerInformation,
	MaxKeyValueInfoClass
} KEY_VALUE_INFORMATION_CLASS;

typedef struct _KEY_VALUE_PARTIAL_INFORMATION {
	ULONG TitleIndex;
	ULONG Type;
	ULONG DataLength;
	UCHAR Data[1];
} KEY_VALUE_PARTIAL_INFORMATION, *PKEY_VALUE_PARTIAL_INFORMATION;

typedef struct _KEY_FULL_INFORMATION {
	LARGE_INTEGER LastWriteTime;
	ULONG TitleIndex;
	ULONG ClassOffset;
	ULONG ClassLength;
	ULONG SubKeys;
	ULONG MaxNameLen;
	ULONG MaxClassLen;
	ULONG Values;
	ULONG MaxValueNameLen;
	ULONG MaxValueDataLen;
	WCHAR Class[1];
} KEY_FULL_INFORMATION, *PKEY_FULL_INFORMATION;

typedef struct _KEY_BASIC_INFORMATION {
	LARGE_INTEGER LastWriteTime;
	ULONG TitleIndex;
	ULONG NameLength;
	WCHAR Name[1];
} KEY_BASIC_INFORMATION, *PKEY_BASIC_INFORMATION;

typedef struct _KEY_NAME_INFORMATION {
	ULONG NameLength;
	WCHAR Name[1];
} KEY_NAME_INFORMATION, *PKEY_NAME_INFORMATION;

typedef struct _OBJECT_ATTRIBUTES {
	ULONG Length;
	HANDLE RootDirectory;
	PUNICODE_STRING ObjectName;
	ULONG Attributes;
	PVOID SecurityDescriptor;
	PVOID SecurityQualityOfService;
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

NTSYSAPI NTSTATUS NtCreateKey(
	PHANDLE KeyHandle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes,
	ULONG TitleIndex,
	PUNICODE_STRING Class,
	ULONG CreateOptions,
	PULONG Disposition);

NTSYSAPI NTSTATUS NtOpenKey(
	PHANDLE KeyHandle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes);

NTSYSAPI NTSTATUS NtQueryKey(
	HANDLE KeyHandle,
	KEY_INFORMATION_CLASS KeyInformationClass,
	PVOID KeyInformation,
	ULONG Length,
	PULONG ResultLength);

NTSYSAPI NTSTATUS NtEnumerateKey(
	HANDLE KeyHandle,
	ULONG Index,
	KEY_INFORMATION_CLASS KeyInformationClass,
	PVOID KeyInformation,
	ULONG Length,
	PULONG ResultLength);

NTSYSAPI NTSTATUS NtQueryValueKey(
	HANDLE KeyHandle,
	PUNICODE_STRING ValueName,
	KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
	PVOID KeyValueInformation,
	ULONG Length,
	PULONG ResultLength);

NTSYSAPI NTSTATUS NtFlushKey(
	HANDLE KeyHandle);

NTSYSAPI NTSTATUS NtSetValueKey(
	HANDLE KeyHandle,
	PUNICODE_STRING ValueName,
	ULONG TitleIndex,
	ULONG Type,
	PVOID Data,
	ULONG DataSize);

NTSYSAPI NTSTATUS NtDeleteKey(
	HANDLE KeyHandle);

NTSYSAPI VOID RtlInitUnicodeString(
	PUNICODE_STRING DestinationString,
	PCWSTR SourceString);

#define KEY_ATTRIBUTE_NORMAL	0x00
#define KEY_ATTRIBUTE_HIDDEN	0x10

NTSTATUS NtRegCreateKey(
	PHANDLE keyHandle,
	HKEY baseKey,
	PCWSTR subKey,
	DWORD attributes);

NTSTATUS NtRegGetValue(
	HANDLE keyHandle,
	PCWSTR valueName,
	DWORD type,
	PVOID data,
	DWORD cbData);

NTSTATUS NtRegSetValue(
	HANDLE keyHandle,
	PCWSTR valueName,
	DWORD type,
	PVOID data,
	DWORD cbData);

NTSTATUS NtRegGetKeyPath(
	HANDLE keyHandle,
	PWSTR buffer,
	DWORD bufferSize);

NTSTATUS NtRegGetKeyName(
	HANDLE keyHandle,
	PWSTR buffer,
	DWORD bufferSize);

NTSTATUS NtRegEnumKey(
	HANDLE keyHandle,
	void (CALLBACK *callback)(HANDLE));

#define NtRegOpenKey NtRegCreateKey
#define NtRegCloseKey CloseHandle
#define NtRegDeleteKey NtDeleteKey

// just some shortcuts
#define NtRegGetString(key, valueName, wsz, wszSize) \
	NtRegGetValue(key, valueName, REG_SZ, wsz, wszSize)
#define NtRegGetExpandString(key, valueName, wsz, wszSize) \
	NtRegGetValue(key, valueName, REG_EXPAND_SZ, wsz, wszSize)
#define NtRegGetMultiString(key, valueName, pwsz, pwszSize) \
	NtRegGetValue(key, valueName, REG_MULTI_SZ, pwsz, pwszSize)
#define NtRegGetDword(key, valueName, pdw) \
	NtRegGetValue(key, valueName, REG_DWORD, pdw, sizeof(DWORD))
#define NtRegGetQword(key, valueName, pqw) \
	NtRegGetValue(key, valueName, REG_QWORD, pqw, sizeof(unsigned __int64))
#define NtRegGetBinary(key, valueName, pb, pbSize) \
	NtRegGetValue(key, valueName, REG_BINARY, pb, pbSize)

#define NtRegSetString(key, valueName, wsz, wszSize) \
	NtRegSetValue(key, valueName, REG_SZ, wsz, wszSize)
#define NtRegSetExpandString(key, valueName, wsz, wszSize) \
	NtRegSetValue(key, valueName, REG_EXPAND_SZ, wsz, wszSize)
#define NtRegSetMultiString(key, valueName, pwsz, pwszSize) \
	NtRegSetValue(key, valueName, REG_MULTI_SZ, pwsz, pwszSize)
#define NtRegSetDword(key, valueName, pdw) \
	NtRegSetValue(key, valueName, REG_DWORD, pdw, sizeof(DWORD))
#define NtRegSetQword(key, valueName, pqw) \
	NtRegSetValue(key, valueName, REG_QWORD, pqw, sizeof(unsigned __int64))
#define NtRegSetBinary(key, valueName, pb, pbSize) \
	NtRegSetValue(key, valueName, REG_BINARY, pb, pbSize)