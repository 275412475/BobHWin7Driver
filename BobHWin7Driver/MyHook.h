#pragma once

#include "ntifs.h"
#include "ntddk.h"
#include <windef.h>
#include "ntdef.h"
#include "DeiverDefFun.h"

VOID StartHOOK(UINT64 HOOK������ַ, UINT64 ��������ַ, USHORT ��д�ĳ���, PVOID *jmpBrigePtr);
VOID RecoveryHOOK(UINT64 HOOK������ַ, USHORT ��д�ĳ���, PVOID  ԭ����);