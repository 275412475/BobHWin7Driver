#include <iostream>
#include "windows.h"

#pragma comment(lib, "BobHWinDriverDll.lib")                                   
extern "C" __declspec(dllimport) bool InitDriver();

extern "C" __declspec(dllimport) void ReadMemoryDWORD(DWORD pid, ULONG64 addre, DWORD * ret);
extern "C" __declspec(dllimport) void WriteMemoryDWORD(DWORD pid, ULONG64 addre, DWORD ret);

extern "C" __declspec(dllimport) void ReadMemoryDWORD64(DWORD pid, ULONG64 addre, DWORD64 * ret);
extern "C" __declspec(dllimport) void WriteMemoryDWORD64(DWORD pid, ULONG64 addre, DWORD64 ret);

extern "C" __declspec(dllexport) void ReadMemoryBytes(DWORD pid, ULONG64 addre, BYTE * *ret, DWORD sizes);
extern "C" __declspec(dllexport) void WriteMemoryBytes(DWORD pid, ULONG64 addre, BYTE * ret, DWORD sizes);

extern "C" __declspec(dllexport) void ReadMemoryFloat(DWORD pid, ULONG64 addre, float* ret);
extern "C" __declspec(dllexport) void WriteMemoryFloat(DWORD pid, ULONG64 addre, float ret);

extern "C" __declspec(dllexport) void ReadMemoryDouble(DWORD pid, ULONG64 addre, double* ret);
extern "C" __declspec(dllexport) void WriteMemoryDouble(DWORD pid, ULONG64 addre, double ret);

int main()
{
	if (!InitDriver())
	{
		printf("���豸ʧ��\n");
	}
	printf("���豸�ɹ�\n");



	/*printf("\n\n\n");
	printf("��ʼ��ȡ�����ڴ��ַ����(����):\n");*/
	/*{

		DWORD Pid, data;
		ULONG64 address = 0;

		printf("���������ö�д��PID��\n");
		scanf_s("%d", &Pid);



		printf("��������ĵ�ַΪ��\n");
		scanf_s("%llx", &address);


		ReadMemoryDWORD(Pid, address, &data);

		printf("����������Ϊ = %d\n", data);


		system("pause");

		printf("��ʼд�ڴ�\n");

		printf("������д��ַ��\n");
		scanf_s("%llx", &address);



		printf("������д�Ķ��٣�\n");
		scanf_s("%d", &data);

		WriteMemoryDWORD(Pid, address, data);

		system("pause");

	}*/

	/*printf("��ʼ��дȡ����\n");*/
	/*{
		DWORD Pid, data;
		ULONG64 address = 0;

		BYTE buffer[8];
		BYTE* bufferPtr = buffer;

		printf("���������ö�д��PID��\n");
		scanf_s("%d", &Pid);


		printf("��������ĵ�ַΪ��\n");
		scanf_s("%llx", &address);

		ReadMemoryBytes(Pid, address, &bufferPtr, sizeof(buffer));

		for (int i = 0; i < 8; i++)
		{
			printf("0x%x  ", buffer[i]);
		}

		printf("\n��ʼд����\n");
		system("pause");


		BYTE writebuff[8] = { 0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88 };

		WriteMemoryBytes(Pid, address, writebuff, sizeof(writebuff));





	}*/

	/*printf("��ʼ��ȡ������\n");*/
	/*{

		DWORD Pid;
		ULONG64 address = 0;
		float dataf;
		double datad;

		printf("���������ö�д��PID��\n");
		scanf_s("%d", &Pid);

		printf("���������float��ַΪ��\n");
		scanf_s("%llx", &address);

		ReadMemoryFloat(Pid, address, &dataf);

		printf("dataf = %f", dataf);

		system("pause");

		printf("���������double��ַΪ��\n");
		scanf_s("%llx", &address);

		ReadMemoryDouble(Pid, address, &datad);

		printf("datad = %lf", datad);
	}*/

	/*printf("��ʼдȡ������\n");*/
	/*{
		DWORD Pid;
		ULONG64 address = 0;
		float dataf;
		double datad;

		printf("���������ö�д��PID��\n");
		scanf_s("%d", &Pid);

		printf("������д��float��ַΪ��\n");
		scanf_s("%llx", &address);

		dataf = 520.1314;

		WriteMemoryFloat(Pid, address, dataf);

		system("pause");

		printf("������д��double��ַΪ��\n");
		scanf_s("%llx", &address);

		datad = 521.1314;
		WriteMemoryDouble(Pid, address, datad);


	}*/

	system("pause");
}