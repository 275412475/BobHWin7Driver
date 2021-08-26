#include "Driverdef.h"
#include "DeiverDefFun.h"
#include "Hide.c"
#define DELAY_ONE_MICROSECOND 	(-10)
#define DELAY_ONE_MILLISECOND	(DELAY_ONE_MICROSECOND*1000)

VOID KernelSleep(LONG msec)
{
	LARGE_INTEGER my_interval;
	my_interval.QuadPart = DELAY_ONE_MILLISECOND;
	my_interval.QuadPart *= msec;
	KeDelayExecutionThread(KernelMode, 0, &my_interval);
}

VOID
DelObject(
	_In_ PVOID StartContext
)
{
	PULONG_PTR pZero = NULL;
	KernelSleep(5000);
	ObMakeTemporaryObject(DeviceObject);
	DPRINT("test seh.\n");
	__try {
		*pZero = 0x100;
	}
	__except (1)
	{
		DPRINT("seh success.\n");
	}
}

VOID Reinitialize(
	_In_     PDRIVER_OBJECT        pDriverObject,
	_In_opt_ PVOID                 Context,
	_In_     ULONG                 Count
)
{
	HANDLE hThread = NULL;
	PsCreateSystemThread(&hThread, THREAD_ALL_ACCESS, NULL, NULL, NULL, DelObject, NULL);
	if (*NtBuildNumber < 8000)
		HideDriverWin7(pDriverObject);
	else
		HideDriverWin10(pDriverObject);
}

NTSTATUS DispatchDevCTL(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
	NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;
	PIO_STACK_LOCATION irpsp = IoGetCurrentIrpStackLocation(Irp);//ȡ��IRP ����
	PVOID buffer = Irp->AssociatedIrp.SystemBuffer;//����Ļ�����
	ULONG CTLcode = irpsp->Parameters.DeviceIoControl.IoControlCode;//�õ��Զ���Ŀ�����
	ULONG uInSize = irpsp->Parameters.DeviceIoControl.InputBufferLength;//����ĳ���
	ULONG uOutSize = irpsp->Parameters.DeviceIoControl.OutputBufferLength;//����ĳ���
	PVOID tmpbuffer = NULL;
	switch (CTLcode)
	{
	case BOBH_READ:
		memcpy(&appBuffer, buffer, uInSize);
		KdPrint(("���յ��ĵ�ַ��:%x \r\n", appBuffer.Address));
		tmpbuffer = ExAllocatePool(NonPagedPool, appBuffer.size + 1);
		RtlFillMemory(tmpbuffer, appBuffer.size + 1, 0);
		/*KdPrint(("tmpbuffer��ַ��%x ����Ϊ%d \r\n", tmpbuffer, *(DWORD*)tmpbuffer));*/
		KeReadProcessMemory(appBuffer.Address, tmpbuffer, appBuffer.size);
		/*KdPrint(("tmpbuffer��ַ��%x ����Ϊ%d \r\n", tmpbuffer, *(DWORD*)tmpbuffer));*/
		memcpy(&appBuffer.Buffer, tmpbuffer, sizeof(tmpbuffer));
		memcpy(buffer, &appBuffer, uInSize);
		ExFreePool(tmpbuffer);
		status = STATUS_SUCCESS;
		break;
	case BOBH_WRITE:
		memcpy(&appBuffer, buffer, uInSize);
		KdPrint(("д�յ��ĵ�ַ��:%x \r\n", appBuffer.Address));

		tmpbuffer = ExAllocatePool(NonPagedPool, appBuffer.size + 1);
		RtlFillMemory(tmpbuffer, appBuffer.size + 1, 0);
		KdPrint(("tmpbuffer��ַ��%x ����Ϊ%d \r\n", tmpbuffer, *(DWORD*)tmpbuffer));

		memcpy(tmpbuffer, &appBuffer.Buffer, appBuffer.size);
		KdPrint(("tmpbuffer��ַ��%x ����Ϊ%d \r\n", tmpbuffer, *(DWORD*)tmpbuffer));

		KeWriteProcessMemory(appBuffer.Address, tmpbuffer, appBuffer.size);

		ExFreePool(tmpbuffer);
		status = STATUS_SUCCESS;

		break;
	case BOBH_SET:
	{
		DWORD PID;
		memcpy(&PID, buffer, uInSize);
		SetPID(PID);
		status = STATUS_SUCCESS;
		break;
	}
	case BOBH_PROTECT:
	{
		DWORD PID;
		memcpy(&PID, buffer, uInSize);
		ProtectProcessStart(PID);
		status = STATUS_SUCCESS;
		break;
	}
	case BOBH_UNPROTECT:
	{
		ProtectProcessStop();
		status = STATUS_SUCCESS;
		break;
	}
	case BOBH_KILLPROCESS_DIRECT:
	{
		DWORD PID;
		memcpy(&PID, buffer, uInSize);
		KeKillProcessSimple(PID);
		status = STATUS_SUCCESS;
		break;
	}
	case BOBH_KILLPROCESS_MEMORY:
	{
		DWORD PID;
		memcpy(&PID, buffer, uInSize);
		KeKillProcessZeroMemory(PID);
		status = STATUS_SUCCESS;
		break;
	}
	case BOBH_GETMODULEADDRESS:
	{
		LPModuleBase TempBase = (LPModuleBase)buffer;
		if (TempBase->Pid <= 0)
		{
			status = STATUS_UNSUCCESSFUL;
			break;
		}

		UNICODE_STRING moudlename = { 0 };
		ULONG64 outdllbase = 0;
		RtlInitUnicodeString(&moudlename, TempBase->ModuleName);
		KdPrint(("ҪѰ�ҵ�moudlenameΪ%wZ \r\n", moudlename));

		outdllbase = KeGetMoudleAddress(TempBase->Pid, &moudlename);

		*(PULONG64)buffer = outdllbase;

		status = STATUS_SUCCESS;
		break;
	}
	case BOBH_GETPROCESSID:
	{


		PMYCHAR a = (PMYCHAR)buffer;

		STRING process = { 0 };


		RtlInitString(&process, a->_char);
		KdPrint(("process %s \r\n", process));


		DWORD pid = 0;
		pid = EnumProcess(process);
		*(PDWORD)buffer = pid;

		if (pid > 0)
		{
			status = STATUS_SUCCESS;
		}
		else
		{
			status = STATUS_UNSUCCESSFUL;
		}
		break;
	}
	default:
		status = STATUS_INVALID_PARAMETER;
		break;
	}
	Irp->IoStatus.Information = uOutSize;
	Irp->IoStatus.Status = status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
	NTSTATUS status;
	int i;
	//��������ж���¼�
	DriverObject->DriverUnload = Unload;
	//�����豸����
	status = IoCreateDevice(DriverObject, 0, &myDeviceName, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &DeviceObject);
	if (!NT_SUCCESS(status)) {
		KdPrint(("[BobHWin7]�����豸����ʧ�� \r\n"));
		return status;
	}
	//������������
	status = IoCreateSymbolicLink(&symLinkName, &myDeviceName);
	if (!NT_SUCCESS(status)) {
		KdPrint(("[BobHWin7]������������ʧ�� \r\n"));
		IoDeleteDevice(DeviceObject);
		return status;
	}
	for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++) {
		DriverObject->MajorFunction[i] = DispatchPassThru;
	}
	//Ϊ��дר��ָ��������
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchDevCTL;
	//��ؽ��̶���
	KdPrint(("[BobHWin7]�ɹ�������������ʼLDR \r\n"));
	PLDR_DATA_TABLE_ENTRY ldr;
	ldr = (PLDR_DATA_TABLE_ENTRY)DriverObject->DriverSection;
	ldr->Flags |= 0x20;
	KdPrint(("[BobHWin7]LDR�޸ĳɹ� \r\n"));
	//ProtectProcessStart(1234);
	//ProtectProcessStart(3100);
	// 
	//��������ж������,ż������
		HideDriver(DriverObject);
	////�������� ж������,�����.
	/*IoRegisterDriverReinitialization(DriverObject, Reinitialize, NULL);*/
	
	return status;
}

VOID Unload(PDRIVER_OBJECT DriverObject) {
	if (isProtecting) {
		ObUnRegisterCallbacks(g_pRegiHandle);
	}
	IoDeleteSymbolicLink(&symLinkName);
	IoDeleteDevice(DeviceObject);
	KdPrint(("[BobHWin7]�ɹ�ж������ \r\n"));
}



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

DWORD EnumProcess(STRING processName)
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
			
			if (RtlCompareString(&nowProcessnameString, &processName,FALSE)==0)
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


VOID KeReadProcessMemory(ULONG64 add, PVOID buffer, SIZE_T size){
	KAPC_STATE apc_state;
	KeStackAttachProcess(Process, &apc_state);
	__try
	{
		if (MmIsAddressValid(add))
		{
			memcpy(buffer, (PVOID)add, size);
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
			memcpy((PVOID)add, buffer, size);
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		DbgPrint("��ȡ����:��ַ:%llX", add);
	}
	KeUnstackDetachProcess(&apc_state);
}
VOID SetPID(DWORD pid) {
	NTSTATUS status = PsLookupProcessByProcessId((HANDLE)pid, &Process);
	if (!NT_SUCCESS(status)) {
		KdPrint(("[BobHWin7]����PIDʧ�� \r\n"));
		return;
	}
	KdPrint(("[BobHWin7]����PID: %d �ɹ� \r\n",pid));
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
VOID KeKillProcessSimple(DWORD pid) {
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
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		KdPrint(("[BobHWin7] ��ͨ����ɱ����ʧ��"));
	}
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
				if (i>0x1000000)  //����ô���㹻�ƻ�����������  
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
		if (pOperationInformation->Operation == OB_OPERATION_HANDLE_CREATE){
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


VOID ProtectProcessStart(DWORD pid) {
	if (isProtecting) {
		return;
	}
	protectPID = pid;
	KdPrint(("[BobHWin7] ��ʼ����PID:%d",pid));
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
	}
	else {
		KdPrint(("[BobHWin7]ע��obj�ص�ʧ�� %x\r\n",status));
		isProtecting = FALSE;
	}
}
VOID ProtectProcessStop() {
	if (isProtecting) {
		ObUnRegisterCallbacks(g_pRegiHandle);
		isProtecting = FALSE;
	}
}

ULONGLONG KeGetMoudleAddress(_In_ ULONG pid, _In_ PUNICODE_STRING name)
{
	PEPROCESS p = NULL;
	ULONGLONG ModuleBase = 0;
	NTSTATUS status = STATUS_SUCCESS;
	KAPC_STATE kapc_state = { 0 };
	ULONG64  dllbaseaddr = 0;
	status = PsLookupProcessByProcessId((HANDLE)pid, &p);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("Ѱ��ʧ��"));
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

				if (RtlCompareUnicodeString(&nowMoudlename, name, TRUE) == 0 ) {

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

				do
				{
					pLdrDataEntry = (PLDR_DATA_TABLE_ENTRY)CONTAINING_RECORD(plistEntryStart, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);

					if (RtlCompareUnicodeString(&pLdrDataEntry->BaseDllName, name, TRUE) == 0)
					{
						dllbaseaddr = (ULONG64)pLdrDataEntry->DllBase;
						KdPrint(("�ҵ��ˣ�"));
						KdPrint(("DllBase %x", dllbaseaddr));
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
//
//test hide driver
//
BOOLEAN HideDriver(
	_In_ PDRIVER_OBJECT pDrvObj)
{
	if (pDrvObj->DriverSection != NULL)
	{
		PLIST_ENTRY nextSection = ((PLIST_ENTRY)pDrvObj->DriverSection)->Blink;
		RemoveEntryList((PLIST_ENTRY)pDrvObj->DriverSection);
		pDrvObj->DriverSection = nextSection;
		DbgPrint("���������ɹ�");
		return TRUE;
	}
	DbgPrint("��������ʧ��");
	return FALSE;
}