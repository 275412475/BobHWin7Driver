#include "MyHook.h"


//nt!NtOpenProcess:
//fffff800`041582ec 4883ec38        sub     rsp, 38h
//fffff800`041582f0 65488b042588010000 mov   rax, qword ptr gs : [188h]
//fffff800`041582f9 448a90f6010000  mov     r10b, byte ptr[rax + 1F6h]
//fffff800`04158300 4488542428      mov     byte ptr[rsp + 28h], r10b
//fffff800`04158305 4488542420      mov     byte ptr[rsp + 20h], r10b
//fffff800`0415830a e851fcffff      call    nt!PsOpenProcess(fffff800`04157f60)

VOID StartHOOK(UINT64 HOOK������ַ, UINT64 ��������ַ, USHORT ��д�ĳ���, PVOID *ԭ����)
{
	///*48 B8 88 77 66 55 44 33 22 11 FF E0*/
	/*�������ֵ12����תָ����ܳ���
	{



		//BYTE Hookcode[] = { 0x48 ,0xBA ,0x88, 0x77 ,0x66, 0x55, 0x44, 0x33, 0x22 ,0x11, 0xFF, 0xE2 };


		//*ԭ���� = ExAllocatePool(NonPagedPool, ��д�ĳ��� + (USHORT)100);


		////����ͷ
		//RtlZeroMemory(*ԭ����, 0x1000);

		//RtlCopyMemory(*ԭ����, (PVOID)HOOK������ַ, ��д�ĳ���);


		////дjmp

		//ULONG_PTR tempjmp = ((ULONG_PTR)HOOK������ַ + ��д�ĳ���);

		//*((PULONG_PTR)(Hookcode + 2)) = tempjmp;

		//RtlCopyMemory(((PUCHAR)(*ԭ����) + ��д�ĳ���), Hookcode, sizeof(Hookcode));




		////ȡ���ǵ�ַ
		//*((PULONG_PTR)(Hookcode + 2)) = (ULONG_PTR)��������ַ;


		////дjmp
		//KIRQL irql = WPOFFx64();

		//RtlCopyMemory(HOOK������ַ, Hookcode, sizeof(Hookcode));

		//WPONx64(irql);


	}
	*/
	{

		UCHAR ����������[] = "\xFF\x25\x00\x00\x00\x00\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF";
		UCHAR ����ԭ����[] = "\xFF\x25\x00\x00\x00\x00\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF";
		memcpy(���������� + 6, &��������ַ, 8);

		/*
		�������ֵ14����תָ����ܳ��� �����ָ���ַΪ0x410000
		0x410000 jmp qword ptr [0x410006]
		0x410006 xxxxxxxx
		����0x410006�д���������ĵ�ַ
		*/
		UINT64 ����ԭ�����ĵ�ַ = HOOK������ַ + ��д�ĳ���;
		memcpy(����ԭ���� + 6, &����ԭ�����ĵ�ַ, 8);
		*ԭ���� = ExAllocatePool(NonPagedPool, ��д�ĳ��� + 14);
		RtlFillMemory(*ԭ����, ��д�ĳ��� + 14, 0x90);

		KIRQL irql = WPOFFx64();
		memcpy(*ԭ����, (PVOID)HOOK������ַ, ��д�ĳ���);
		memcpy((PCHAR)(*ԭ����) + ��д�ĳ���, ����ԭ����, 14);

		KIRQL dpc_irql = KeRaiseIrqlToDpcLevel();
		RtlFillMemory((void*)HOOK������ַ, ��д�ĳ���, 0x90);
		memcpy((PVOID)HOOK������ַ, &����������, 14);
		KeLowerIrql(dpc_irql);
		WPONx64(irql);
	}

	
}



VOID RecoveryHOOK(UINT64 HOOK������ַ, USHORT ��д�ĳ���, PVOID ԭ����)
{
	KIRQL irql = WPOFFx64();
	memcpy((PVOID)HOOK������ַ, ԭ����, ��д�ĳ���);
	WPONx64(irql);
	ExFreePool(ԭ����);
}