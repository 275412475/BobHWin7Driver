#include "DeiverDefFun.h"
#include "WindowsStructure.h"

PEPROCESS Process;

DWORD protectPID;
PVOID g_pRegiHandle;
BOOLEAN isProtecting;

UNICODE_STRING myDeviceName;//�豸����
UNICODE_STRING symLinkName;//�豸��������
PDEVICE_OBJECT DeviceObject;

