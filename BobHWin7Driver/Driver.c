#include "ntifs.h"
#include "ntddk.h"
#include <windef.h>

#define BOBH_SET CTL_CODE(FILE_DEVICE_UNKNOWN,0x810,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define BOBH_READ CTL_CODE(FILE_DEVICE_UNKNOWN,0x811,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define BOBH_WRITE CTL_CODE(FILE_DEVICE_UNKNOWN,0x812,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define BOBH_PROTECT CTL_CODE(FILE_DEVICE_UNKNOWN,0x813,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define BOBH_UNPROTECT CTL_CODE(FILE_DEVICE_UNKNOWN,0x814,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define BOBH_KILLPROCESS_DIRECT CTL_CODE(FILE_DEVICE_UNKNOWN,0x815,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define BOBH_KILLPROCESS_MEMORY CTL_CODE(FILE_DEVICE_UNKNOWN,0x816,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define PROCESS_TERMINATE         0x0001  
#define PROCESS_VM_OPERATION      0x0008  
#define PROCESS_VM_READ           0x0010  
#define PROCESS_VM_WRITE          0x0020 

UNICODE_STRING myDeviceName = RTL_CONSTANT_STRING(L"\\Device\\BobHWin7Read");//�豸����
UNICODE_STRING symLinkName = RTL_CONSTANT_STRING(L"\\??\\BobHWin7ReadLink");//�豸��������
PDEVICE_OBJECT DeviceObject = NULL;

PEPROCESS Process = NULL;

DWORD protectPID = -1;
PVOID g_pRegiHandle = NULL;
BOOLEAN isProtecting = FALSE;
struct r3Buffer {
	ULONG64 Address;
	ULONG64 Buffer;
	ULONG64 size;
}appBuffer;

typedef struct _LDR_DATA_TABLE_ENTRY
{
	LIST_ENTRY    InLoadOrderLinks;
	LIST_ENTRY    InMemoryOrderLinks;
	LIST_ENTRY    InInitializationOrderLinks;
	PVOID            DllBase;
	PVOID            EntryPoint;
	ULONG            SizeOfImage;
	UNICODE_STRING    FullDllName;
	UNICODE_STRING     BaseDllName;
	ULONG            Flags;
	USHORT            LoadCount;
	USHORT            TlsIndex;
	PVOID            SectionPointer;
	ULONG            CheckSum;
	PVOID            LoadedImports;
	PVOID            EntryPointActivationContext;
	PVOID            PatchInformation;
	LIST_ENTRY    ForwarderLinks;
	LIST_ENTRY    ServiceTagLinks;
	LIST_ENTRY    StaticLinks;
	PVOID            ContextInformation;
	ULONG            OriginalBase;
	LARGE_INTEGER    LoadTime;
} LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY;

VOID Unload(PDRIVER_OBJECT DriverObject) {
	if (isProtecting) {
		ObUnRegisterCallbacks(g_pRegiHandle);
	}
	IoDeleteSymbolicLink(&symLinkName);
	IoDeleteDevice(DeviceObject);
	KdPrint(("[BobHWin7]�ɹ�ж������ \r\n"));
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
		memcpy(&appBuffer,buffer,uInSize);
		KdPrint(("���յ��ĵ�ַ��:%x \r\n",appBuffer.Address));
		tmpbuffer = ExAllocatePool(NonPagedPool, appBuffer.size+1);
		RtlFillMemory(tmpbuffer, appBuffer.size + 1, 0);
		/*KdPrint(("tmpbuffer��ַ��%x ����Ϊ%d \r\n", tmpbuffer, *(DWORD*)tmpbuffer));*/
		KeReadProcessMemory(appBuffer.Address, tmpbuffer, appBuffer.size);
		/*KdPrint(("tmpbuffer��ַ��%x ����Ϊ%d \r\n", tmpbuffer, *(DWORD*)tmpbuffer));*/
		memcpy(&appBuffer.Buffer,tmpbuffer, sizeof(tmpbuffer));
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

		KeWriteProcessMemory(appBuffer.Address,tmpbuffer,appBuffer.size);

		ExFreePool(tmpbuffer);
		status = STATUS_SUCCESS;
		
		break;
	case BOBH_SET: 
	{
		DWORD PID;
		memcpy(&PID,buffer,uInSize);
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

	return status;
}