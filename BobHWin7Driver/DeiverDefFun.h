#pragma once
#include "ntifs.h"
#include "ntddk.h"
#include <windef.h>
#include "ntdef.h"


extern NTKERNELAPI PVOID PsGetProcessWow64Process(_In_ PEPROCESS Process);
extern PVOID PsGetProcessPeb(_In_ PEPROCESS Process);
extern NTKERNELAPI UCHAR* PsGetProcessImageFileName(IN PEPROCESS Process); //δ�����Ľ��е�������
extern NTKERNELAPI HANDLE PsGetProcessInheritedFromUniqueProcessId(IN PEPROCESS Process);//δ�������е��� 


PEPROCESS LookupProcess(HANDLE Pid);
DWORD GetPidByEnumProcess(STRING processName);
VOID Unload(PDRIVER_OBJECT DriverObject);
VOID KeReadProcessMemory(ULONG64 add, PVOID buffer, SIZE_T size);
VOID KeWriteProcessMemory(ULONG64 add, PVOID buffer, SIZE_T size);
NTSTATUS SetPID(DWORD pid);
NTSTATUS DispatchPassThru(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS KeKillProcessSimple(DWORD pid);
BOOLEAN KeKillProcessZeroMemory(DWORD pid);
OB_PREOP_CALLBACK_STATUS MyObjectPreCallback
(
	__in PVOID  RegistrationContext,
	__in POB_PRE_OPERATION_INFORMATION  pOperationInformation
);
NTSTATUS ProtectProcessStart(DWORD pid);
NTSTATUS ProtectProcessStop();
ULONGLONG KeGetMoudleAddress(_In_ ULONG pid, _In_ PUNICODE_STRING name);
NTSTATUS DispatchDevCTL(PDEVICE_OBJECT DeviceObject, PIRP Irp);
//
//�ر��ڴ�д�����Ĵ���
//
KIRQL WPOFFx64();
//
//���ڴ�д�����Ĵ���
//
void WPONx64(
	KIRQL irql);