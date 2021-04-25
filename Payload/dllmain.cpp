// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include<windows.h>
#include <winternl.h>
#include "detours.h"

#pragma comment(lib,"detours.lib")

typedef void PROCESS_CREATE_INFO, *PPROCESS_CREATE_INFO;
typedef void PROCESS_ATTRIBUTE_LIST, *PPROCESS_ATTRIBUTE_LIST;

typedef NTSTATUS(WINAPI* realRtlUnicodeStringToAnsiString)(IN PANSI_STRING destinationString, IN PCUNICODE_STRING sourceString, IN BOOLEAN allocateDestinationString);

typedef NTSTATUS(NTAPI* realNtCreateUserProcess)
(
    PHANDLE ProcessHandle,
    PHANDLE ThreadHandle,
    ACCESS_MASK ProcessDesiredAccess,
    ACCESS_MASK ThreadDesiredAccess,
    POBJECT_ATTRIBUTES ProcessObjectAttributes,
    POBJECT_ATTRIBUTES ThreadObjectAttributes,
    ULONG ProcessFlags,
    ULONG ThreadFlags,
    PRTL_USER_PROCESS_PARAMETERS ProcessParameters,
    PPROCESS_CREATE_INFO CreateInfo,
    PPROCESS_ATTRIBUTE_LIST AttributeList
    );

realNtCreateUserProcess originalNtCreateUserProcess = (realNtCreateUserProcess)GetProcAddress(GetModuleHandleA("ntdll.dll"),
    "NtCreateUserProcess");

realRtlUnicodeStringToAnsiString originalRtlUnicodeStringToAnsiString = (realRtlUnicodeStringToAnsiString)GetProcAddress(GetModuleHandleA("ntdll.dll"),
    "RtlUnicodeStringToAnsiString");

DWORD WINAPI createMessageBox(LPCSTR lpParam) {
    MessageBox(NULL, lpParam, "Dll says:", MB_OK);
    return 0;
}

bool compareToFileName(PRTL_USER_PROCESS_PARAMETERS processParameters, const char* filename) {

    ANSI_STRING as;
    UNICODE_STRING EntryName;
    EntryName.MaximumLength = EntryName.Length = (USHORT)processParameters->ImagePathName.Length;
    EntryName.Buffer = &processParameters->ImagePathName.Buffer[0];
    originalRtlUnicodeStringToAnsiString(&as, &EntryName, TRUE);

    if (_strcmpi(as.Buffer, filename) == 0) {
        return true;
    }
    else {
        return false;
    }
}



NTSTATUS WINAPI  _NtCreateUserProcess
(
    PHANDLE ProcessHandle,
    PHANDLE ThreadHandle,
    ACCESS_MASK ProcessDesiredAccess,
    ACCESS_MASK ThreadDesiredAccess,
    POBJECT_ATTRIBUTES ProcessObjectAttributes,
    POBJECT_ATTRIBUTES ThreadObjectAttributes,
    ULONG ProcessFlags,
    ULONG ThreadFlags,
    PRTL_USER_PROCESS_PARAMETERS ProcessParameters,
    PPROCESS_CREATE_INFO CreateInfo,
    PPROCESS_ATTRIBUTE_LIST AttributeList
)
{

    if (compareToFileName(ProcessParameters, "C:\\Windows\\System32\\notepad.exe")) {
        return STATUS_INVALID_HANDLE;
    }
    else {
        return originalNtCreateUserProcess(ProcessHandle, ThreadHandle, ProcessDesiredAccess, ThreadDesiredAccess, ProcessObjectAttributes, ThreadObjectAttributes, ProcessFlags, ThreadFlags, ProcessParameters, CreateInfo, AttributeList);
    }
}

void attachDetour() {

    DetourRestoreAfterWith();
    DetourTransactionBegin();

    DetourUpdateThread(GetCurrentThread());

    DetourAttach((PVOID*)&originalNtCreateUserProcess, _NtCreateUserProcess);

    DetourTransactionCommit();
}

void deAttachDetour() {

    DetourTransactionBegin();

    DetourUpdateThread(GetCurrentThread());

    DetourDetach((PVOID*)&originalNtCreateUserProcess, _NtCreateUserProcess);

    DetourTransactionCommit();
}


BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        attachDetour();
        break;
    case DLL_PROCESS_DETACH:
        deAttachDetour();
        break;
    }
    return TRUE;
}
