// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include "../sharedmemory.h"
#include <winternl.h>
#include <detours/detours.h>

typedef CLIENT_ID *PCLIENT_ID;
typedef NTSTATUS (*NtOpenProcessType)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, PCLIENT_ID);

NtOpenProcessType oldNtOpenProcess;
HANDLE pidsHandle, pathHandle;
DWORD* pids;
LPWSTR path;

__declspec(dllexport) __kernel_entry NTSTATUS NTAPI newNtOpenProcess(
    PHANDLE            ProcessHandle,
    ACCESS_MASK        DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes,
    PCLIENT_ID         ClientId
) {
    if ((DWORD)ClientId->UniqueProcess == GetCurrentProcessId()) return oldNtOpenProcess(ProcessHandle, DesiredAccess, ObjectAttributes, ClientId); //允许进程自己访问自己
    for (int i = 0;i < PIDS_COUNT;i++) {
        if ((DWORD)ClientId->UniqueProcess == pids[i]) DesiredAccess &= READ_CONTROL | SYNCHRONIZE;
        break;
    }
    return oldNtOpenProcess(ProcessHandle, DesiredAccess, ObjectAttributes, ClientId);
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        if (DetourIsHelperProcess()) return TRUE;
        DetourRestoreAfterWith();
        HMODULE hNtdll=GetModuleHandle(L"ntdll.dll");
        oldNtOpenProcess = (NtOpenProcessType)GetProcAddress(hNtdll, "NtOpenProcess");
        CloseHandle(hNtdll);
        pidsHandle = OpenFileMapping(FILE_MAP_READ, FALSE, SHARED_MEMORY_PIDS_NAME);
        if (pidsHandle == NULL) return FALSE;
        pids = (DWORD*)MapViewOfFile(pidsHandle, FILE_MAP_READ, 0, 0, 0);
        if (pids == NULL) return FALSE;
        pathHandle = OpenFileMapping(FILE_MAP_READ, FALSE, SHARED_MEMORY_PATH_NAME);
        if (pathHandle == NULL) return FALSE;
        path = (LPWSTR)MapViewOfFile(pathHandle, FILE_MAP_READ, 0, 0, 0);
        if (path == NULL) return FALSE;
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach(&(PVOID&)oldNtOpenProcess, newNtOpenProcess);
        DetourTransactionCommit();
        break;
    }
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH: {
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourDetach(&(PVOID&)oldNtOpenProcess, newNtOpenProcess);
        DetourTransactionCommit();
        UnmapViewOfFile(pids);
        UnmapViewOfFile(path);
        CloseHandle(pidsHandle);
        CloseHandle(pathHandle);
        break;
    }
    }
    return TRUE;
}

