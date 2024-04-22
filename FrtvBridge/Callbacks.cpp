#include "Callbacks.h"

DebugCallback* dbgcb = NULL;
DBCallback* dcb = NULL;
ConnectCallback* ccb = NULL;
DisconnectCallback* dccb = NULL;

void CallDebugCallback(LPCSTR message)
{
	if (dbgcb != NULL)
		dbgcb(message);
}

void CallDBCallback(LPCSTR fileName, DWORD crc32)
{
	if (dcb != NULL)
		dcb(fileName, crc32);
}

void CallConnectCallback()
{
	if (ccb != NULL)
		ccb();
}

void CallDisconnectCallback()
{
	if (dccb != NULL)
		dccb();
}

__declspec(dllexport) void RegisterCallbacks(DebugCallback* debugCallback, DBCallback * dbCallback, ConnectCallback * ccCallback, DisconnectCallback * dcCallback)
{
	dbgcb = debugCallback;
	dcb = dbCallback;
	ccb = ccCallback;
	dccb = dcCallback;
}