#include "pch.h"
#include "releaseHelper.h"
#include <cstdio>
#include <string.h>
#include <direct.h>
#include <exception>
#include <atlconv.h>

CReleaseDLL::CReleaseDLL()
{
	this->m_hModule = GetSelfModuleHandle();
	if (m_hModule == NULL)
	{
		throw std::exception("Error:��ȡ��ַʧ��");
	}
	//��ȡĿ¼
	memset(this->m_filePath, 0, MAX_DLL_PATH);
	_getcwd(this->m_filePath, MAX_DLL_PATH);
}

CReleaseDLL::~CReleaseDLL()
{
}

bool CReleaseDLL::FreeResFile(unsigned long m_lResourceID, const char* m_strResourceType, const char* m_strFileName)
{
	//���������ͷ��ļ�·��
	char strFullPath[MAX_DLL_PATH] = { 0 };
	sprintf_s(strFullPath, "%s\\%s", this->m_filePath, m_strFileName);
	USES_CONVERSION;
	//������Դ
	HRSRC hResID = ::FindResource(this->m_hModule, MAKEINTRESOURCE(m_lResourceID), A2W(m_strResourceType));
	//������Դ  
	HGLOBAL hRes = ::LoadResource(this->m_hModule, hResID);
	//������Դ
	LPVOID pRes = ::LockResource(hRes);
	if (pRes == NULL)
	{
		return FALSE;
	}
	//�õ����ͷ���Դ�ļ���С 
	unsigned long dwResSize = ::SizeofResource(this->m_hModule, hResID);
	//�����ļ� 
	HANDLE hResFile = CreateFile(A2W(strFullPath), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (INVALID_HANDLE_VALUE == hResFile)
	{
		return FALSE;
	}

	DWORD dwWritten = 0;//д���ļ��Ĵ�С     
	WriteFile(hResFile, pRes, dwResSize, &dwWritten, NULL);//д���ļ�  
	CloseHandle(hResFile);//�ر��ļ����  

	return (dwResSize == dwWritten);//��д���С�����ļ���С�����سɹ�������ʧ��  

}
HMODULE GetSelfModuleHandle()
{
	MEMORY_BASIC_INFORMATION mbi;

	return ((::VirtualQuery(GetSelfModuleHandle, &mbi, sizeof(mbi)) != 0)
		? (HMODULE)mbi.AllocationBase : NULL);
}
HMODULE GetCurrentModule(BOOL bRef/* = FALSE*/)
{
	HMODULE hModule = NULL;
	if (GetModuleHandleEx(bRef ? GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS : (GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS
		| GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT), (LPCWSTR)GetCurrentModule, &hModule))
	{
		return hModule;
	}

	return NULL;
}

HMODULE CReleaseDLL::GetSelfModuleHandle()
{
	try
	{
		//#ifdef _USER_RELEASEDLL_
				//����ͷŵİ����ඨ����DLL�У�����������ķ�ʽ��ȡ��ַ
				/*MEMORY_BASIC_INFORMATION mbi;
				return ((::VirtualQuery(CReleaseDLL::GetSelfModuleHandle, &mbi, sizeof(mbi)) != 0) ? (HMODULE)mbi.AllocationBase : NULL);*/
				//#else
				//		/*���ֱ�Ӷ�����exe����Ĵ�����*/
				//		return ::GetModuleHandle(NULL);
				//#endif
		return GetCurrentModule(false);
		/*	return ghModule;*/
	}
	catch (...)
	{
		return NULL;
	}

}



