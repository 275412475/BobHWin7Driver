#pragma once
#include "DeiverDefFun.h"
#include "GlobalVariables.h"
#include "Driverdef.h"


// ���ݽ���ID���ؽ���EPROCESS�ṹ��,ʧ�ܷ���NULL
PEPROCESS LookupProcess(HANDLE Pid)
{
	PEPROCESS eprocess = NULL;
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	Status = PsLookupProcessByProcessId(Pid, &eprocess);
	if (NT_SUCCESS(Status))
		return eprocess;
	return NULL;
}

DWORD GetPidByEnumProcess(STRING processName)
{
	PEPROCESS eproc = NULL;

	DWORD ret = 0;

	for (int temp = 0; temp < 100000; temp += 4)
	{
		eproc = LookupProcess((HANDLE)temp);
		if (eproc != NULL)
		{
			STRING nowProcessnameString = { 0 };
			RtlInitString(&nowProcessnameString, PsGetProcessImageFileName(eproc));

			if (RtlCompareString(&nowProcessnameString, &processName, FALSE) == 0)
			{
				ret = PsGetProcessId(eproc);
				break;
			}

			DbgPrint("������: %s --> ����PID = %d --> ������PPID = %d\r\n", PsGetProcessImageFileName(eproc), PsGetProcessId(eproc),
				PsGetProcessInheritedFromUniqueProcessId(eproc));
			ObDereferenceObject(eproc);
		}
	}
	return ret;
}


VOID KeReadProcessMemory(ULONG64 add, PVOID buffer, SIZE_T size) {
	KAPC_STATE apc_state;
	KeStackAttachProcess(Process, &apc_state);
	__try
	{
		if (MmIsAddressValid(add))
		{
			RtlCopyMemory(buffer, (PVOID)add, size);
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		DbgPrint("��ȡ����:��ַ:%llX", add);
	}
	KeUnstackDetachProcess(&apc_state);
}
VOID KeWriteProcessMemory(ULONG64 add, PVOID buffer, SIZE_T size) {
	KAPC_STATE apc_state;
	KeStackAttachProcess(Process, &apc_state);
	__try
	{
		if (MmIsAddressValid(add))
		{
			RtlCopyMemory((PVOID)add, buffer, size);
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		__try
		{
			KIRQL irql = WPOFFx64();


			if (MmIsAddressValid(add))
			{
				RtlCopyMemory((PVOID)add, buffer, size);
			}

			WPONx64(irql);

		}

		__except (1)
		{
			DbgPrint("��ȡ����:��ַ:%llX", add);
		}
	}
	KeUnstackDetachProcess(&apc_state);
}
NTSTATUS SetPID(DWORD pid) {
	NTSTATUS status = PsLookupProcessByProcessId((HANDLE)pid, &Process);
	if (!NT_SUCCESS(status)) {
		KdPrint(("[BobHWin7]����PIDʧ�� \r\n"));
		return status;
	}
	KdPrint(("[BobHWin7]����PID: %d �ɹ� \r\n", pid));
	return status;
}
NTSTATUS DispatchPassThru(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	//�õ�irp��ջ��ַ
	PIO_STACK_LOCATION irpsp = IoGetCurrentIrpStackLocation(Irp);
	NTSTATUS status = STATUS_SUCCESS;
	//���IRP����
	Irp->IoStatus.Information = 0;
	Irp->IoStatus.Status = status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}
NTSTATUS KeKillProcessSimple(DWORD pid) {
	NTSTATUS ntStatus = STATUS_SUCCESS;
	__try {
		HANDLE hProcess = NULL;
		CLIENT_ID ClientId = { 0 };
		OBJECT_ATTRIBUTES oa = { 0 };
		ClientId.UniqueProcess = (HANDLE)pid;
		ClientId.UniqueThread = 0;
		oa.Length = sizeof(oa);
		oa.RootDirectory = 0;
		oa.ObjectName = 0;
		oa.Attributes = 0;
		oa.SecurityDescriptor = 0;
		oa.SecurityQualityOfService = 0;
		ZwOpenProcess(&hProcess, 1, &oa, &ClientId);
		if (hProcess)
		{
			ZwTerminateProcess(hProcess, 0);
			ZwClose(hProcess);
		}
		KdPrint(("[BobHWin7] ɱ���̳ɹ�"));
		ntStatus = STATUS_SUCCESS;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		KdPrint(("[BobHWin7] ��ͨ����ɱ����ʧ��"));
		ntStatus = STATUS_UNSUCCESSFUL;
	}
	return ntStatus;
}
BOOLEAN KeKillProcessZeroMemory(DWORD pid) {
	NTSTATUS ntStatus = STATUS_SUCCESS;
	int i = 0;
	PVOID handle;
	PEPROCESS Eprocess;
	ntStatus = PsLookupProcessByProcessId(pid, &Eprocess);
	if (NT_SUCCESS(ntStatus))
	{
		PKAPC_STATE pKs = (PKAPC_STATE)ExAllocatePool(NonPagedPool, sizeof(PKAPC_STATE));
		KeStackAttachProcess(Eprocess, pKs);//Attach��������ռ�
		for (i = 0; i <= 0x7fffffff; i += 0x1000)
		{
			if (MmIsAddressValid((PVOID)i))
			{
				_try
				{
					ProbeForWrite((PVOID)i,0x1000,sizeof(ULONG));
					memset((PVOID)i,0xcc,0x1000);
				}
				_except(1) { continue; }
			}
			else {
				if (i > 0x1000000)  //����ô���㹻�ƻ�����������  
					break;
			}
		}
		KeUnstackDetachProcess(pKs);
		if (ObOpenObjectByPointer((PVOID)Eprocess, 0, NULL, 0, NULL, KernelMode, &handle) != STATUS_SUCCESS)
			return FALSE;
		ZwTerminateProcess((HANDLE)handle, STATUS_SUCCESS);
		ZwClose((HANDLE)handle);
		return TRUE;
	}
	return FALSE;

}
OB_PREOP_CALLBACK_STATUS MyObjectPreCallback
(
	__in PVOID  RegistrationContext,
	__in POB_PRE_OPERATION_INFORMATION  pOperationInformation
)
{
	//KdPrint(("[BobHWin7]�����ˣ����� \r\n"));
	if (pOperationInformation->KernelHandle)
		return OB_PREOP_SUCCESS;
	HANDLE pid = PsGetProcessId((PEPROCESS)pOperationInformation->Object);
	if (pid == protectPID) {
		//KdPrint(("[BobHWin7]�й�PIDִ�в���"));
		if (pOperationInformation->Operation == OB_OPERATION_HANDLE_CREATE) {
			if ((pOperationInformation->Parameters->CreateHandleInformation.OriginalDesiredAccess & PROCESS_TERMINATE) == PROCESS_TERMINATE)
			{
				pOperationInformation->Parameters->CreateHandleInformation.DesiredAccess &= ~PROCESS_TERMINATE;
				//pOperationInformation->Parameters->CreateHandleInformation.DesiredAccess = 0;
			}
			if ((pOperationInformation->Parameters->CreateHandleInformation.OriginalDesiredAccess & PROCESS_VM_OPERATION) == PROCESS_VM_OPERATION)//openprocess
			{
				pOperationInformation->Parameters->CreateHandleInformation.DesiredAccess &= ~PROCESS_VM_OPERATION;
				//pOperationInformation->Parameters->CreateHandleInformation.DesiredAccess = 0;
			}
			if ((pOperationInformation->Parameters->CreateHandleInformation.OriginalDesiredAccess & PROCESS_VM_READ) == PROCESS_VM_READ)//�ڴ��
			{
				pOperationInformation->Parameters->CreateHandleInformation.DesiredAccess &= ~PROCESS_VM_READ;
				//pOperationInformation->Parameters->CreateHandleInformation.DesiredAccess = 0;
			}
			if ((pOperationInformation->Parameters->CreateHandleInformation.OriginalDesiredAccess & PROCESS_VM_WRITE) == PROCESS_VM_WRITE)//�ڴ�д
			{
				pOperationInformation->Parameters->CreateHandleInformation.DesiredAccess &= ~PROCESS_VM_WRITE;
				//pOperationInformation->Parameters->CreateHandleInformation.DesiredAccess = 0;
			}
		}
	}
	return OB_PREOP_SUCCESS;
}


NTSTATUS ProtectProcessStart(DWORD pid) {
	if (isProtecting) {
		return;
	}
	protectPID = pid;
	KdPrint(("[BobHWin7] ��ʼ����PID:%d", pid));
	OB_OPERATION_REGISTRATION oor;
	OB_CALLBACK_REGISTRATION ob;
	oor.ObjectType = PsProcessType;
	oor.Operations = OB_OPERATION_HANDLE_CREATE;//Ӧ�ò�򿪾����֪ͨ�ص�,Ҳ�����OB_OPERATION_HANDLE_DUPLICATE  ��ʾ��ֵ���
	oor.PreOperation = MyObjectPreCallback;
	oor.PostOperation = NULL;
	ob.Version = OB_FLT_REGISTRATION_VERSION;
	ob.OperationRegistrationCount = 1;
	ob.OperationRegistration = &oor;
	RtlInitUnicodeString(&ob.Altitude, L"321000");
	ob.RegistrationContext = NULL;

	NTSTATUS status = ObRegisterCallbacks(&ob, &g_pRegiHandle);
	if (NT_SUCCESS(status)) {
		KdPrint(("[BobHWin7]ע��obj�ص��ɹ� \r\n"));
		isProtecting = TRUE;
		return status;
	}
	else {
		KdPrint(("[BobHWin7]ע��obj�ص�ʧ�� %x\r\n", status));
		isProtecting = FALSE;
		return status;
	}
}
NTSTATUS ProtectProcessStop() {
	NTSTATUS status = STATUS_SUCCESS;
	if (isProtecting) {
		ObUnRegisterCallbacks(g_pRegiHandle);
		isProtecting = FALSE;
	}
	return status;
}

ULONGLONG KeGetMoudleAddress(_In_ ULONG pid, _In_ PUNICODE_STRING name)
{
	PEPROCESS p = NULL;
	ULONGLONG ModuleBase = 0;
	NTSTATUS status = STATUS_SUCCESS;
	KAPC_STATE kapc_state = { 0 };
	ULONGLONG  dllbaseaddr = 0;
	status = PsLookupProcessByProcessId((HANDLE)pid, &p);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("δ�ҵ����̣�Ѱ��ʧ��"));
		return 0;
	}
	KeStackAttachProcess(p, &kapc_state);


	PPEB32 peb32 = NULL;

	peb32 = (PPEB32)PsGetProcessWow64Process(p);

	if (peb32 != NULL)
	{

		KdPrint(("wow64����"));

		PPEB_LDR_DATA32 pPebLdrData32 = NULL;
		PLDR_DATA_TABLE_ENTRY32 pLdrDataEntry32 = NULL; //LDR�������;
		PLIST_ENTRY32 pListEntryStart32 = NULL; //����ͷ�ڵ㡢β�ڵ�;
		PLIST_ENTRY32 pListEntryEnd32 = NULL;
		__try
		{

			pPebLdrData32 = (PPEB_LDR_DATA32)peb32->Ldr;
			/*	KdPrint(("pPebLdrData32 %x", pPebLdrData32));*/

			pListEntryStart32 = pListEntryEnd32 = pPebLdrData32->InLoadOrderModuleList.Flink;
			/*KdPrint(("pListEntryStart32 %x", pListEntryStart32));*/
			do {//���DLLȫ·��;


				pLdrDataEntry32 = (PLDR_DATA_TABLE_ENTRY32)CONTAINING_RECORD(pListEntryStart32, LDR_DATA_TABLE_ENTRY32, InMemoryOrderLinks);

				/*KdPrint(("pLdrDataEntry32 %x", pLdrDataEntry32));*/

				WCHAR* a = (DWORD)pLdrDataEntry32->BaseDllName;

				/*KdPrint(("BaseDllName %x", pLdrDataEntry32->BaseDllName));*/

				UNICODE_STRING nowMoudlename = { 0 };
				RtlInitUnicodeString(&nowMoudlename, a);

				if (RtlCompareUnicodeString(&nowMoudlename, name, TRUE) == 0) {

					dllbaseaddr = pLdrDataEntry32->DllBase;

					KdPrint(("�ҵ��ˣ�"));
					KdPrint(("DllBase %x", dllbaseaddr));
					break;
				}

				pListEntryStart32 = (PLIST_ENTRY32)pListEntryStart32->Flink;
			} while (pListEntryStart32 != pListEntryEnd32);

		}
		__except (1)
		{
			KdPrint(("�ڴ�����쳣"));

			dllbaseaddr = 0;
		}
	}
	else
	{
		PPEB peb = NULL;
		peb = (PPEB)PsGetProcessPeb(p);
		if (peb == NULL)
		{
			ObDereferenceObject(p);
			KdPrint(("Ѱ��ʧ��"));
			KeUnstackDetachProcess(&kapc_state);
			return 0;
		}
		else
		{
			KdPrint(("��wow64����"));

			PPEB_LDR_DATA pPebLdrData = (PPEB_LDR_DATA)peb->Ldr;

			PLIST_ENTRY plistEntryStart = NULL, plistEntryEnd = NULL;

			PLDR_DATA_TABLE_ENTRY pLdrDataEntry = NULL;

			plistEntryStart = plistEntryEnd = pPebLdrData->InMemoryOrderModuleList.Blink;

			__try
			{
				/*KdPrint(("pPebLdrData %llx", pPebLdrData));
				KdPrint(("plistEntryStart %llx", plistEntryStart));*/


				do
				{
					pLdrDataEntry = (PLDR_DATA_TABLE_ENTRY)CONTAINING_RECORD(plistEntryStart, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);

					if (RtlCompareUnicodeString(&pLdrDataEntry->BaseDllName, name, TRUE) == 0)
					{
						dllbaseaddr = (ULONGLONG)(pLdrDataEntry->DllBase);
						KdPrint(("�ҵ��ˣ�"));
						/*KdPrint(("pLdrDataEntry %llx", (ULONGLONG)(pLdrDataEntry->DllBase)));*/
						KdPrint(("DllBase %llx", dllbaseaddr));
						break;
					}
					/*KdPrint(("pLdrDataEntry  0x%x", pLdrDataEntry));*/

					plistEntryStart = plistEntryStart->Blink;

				} while (plistEntryStart != plistEntryEnd);
			}
			__except (1)
			{
				KdPrint(("�ڴ�����쳣"));
				dllbaseaddr = 0;
			}
		}
	}

	ObDereferenceObject(p);
	KeUnstackDetachProcess(&kapc_state);
	return dllbaseaddr;
}




//
//�ر��ڴ�д�����Ĵ���
//
KIRQL WPOFFx64()
{
	KIRQL irql = KeRaiseIrqlToDpcLevel();
	UINT64 cr0 = __readcr0();
	cr0 &= 0xfffffffffffeffff;
	__writecr0(cr0);
	_disable();
	return irql;
}

//
//���ڴ�д�����Ĵ���
//
void WPONx64(
	KIRQL irql)
{
	UINT64 cr0 = __readcr0();
	cr0 |= 0x10000;
	_enable();
	__writecr0(cr0);
	KeLowerIrql(irql);
}