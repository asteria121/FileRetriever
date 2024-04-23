#include "Callbacks.h"

DebugCallback* dbgcb = NULL;
DBCallback* dcb = NULL;
ConnectCallback* ccb = NULL;
DisconnectCallback* dccb = NULL;

// ����� �޽��� ���� �ݹ�
void CallDebugCallback(DWORD logLevel, PCSTR message)
{
	if (dbgcb != NULL)
		dbgcb(logLevel, message);
}

// �����ͺ��̽� ��� ��û �ݹ� (���� ��� �Ϸ� ��)
void CallDBCallback(LPCSTR fileName, DWORD crc32)
{
	if (dcb != NULL)
		dcb(fileName, crc32);
}

// ���� ������ ȣ��Ǵ� �ݹ�
void CallConnectCallback()
{
	if (ccb != NULL)
		ccb();
}

// ������ ������ ��� ȣ��Ǵ� �ݹ�
void CallDisconnectCallback()
{
	if (dccb != NULL)
		dccb();
}

// �ݹ��Լ� ��� �Լ�
__declspec(dllexport) void RegisterCallbacks(DebugCallback* debugCallback, DBCallback * dbCallback, ConnectCallback * ccCallback, DisconnectCallback * dcCallback)
{
	dbgcb = debugCallback;
	dcb = dbCallback;
	ccb = ccCallback;
	dccb = dcCallback;
}