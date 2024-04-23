#include "Callbacks.h"

DebugCallback* dbgcb = NULL;
DBCallback* dcb = NULL;
ConnectCallback* ccb = NULL;
DisconnectCallback* dccb = NULL;

// 디버그 메시지 전송 콜백
void CallDebugCallback(DWORD logLevel, PCSTR message)
{
	if (dbgcb != NULL)
		dbgcb(logLevel, message);
}

// 데이터베이스 등록 요청 콜백 (파일 백업 완료 후)
void CallDBCallback(LPCSTR fileName, DWORD crc32)
{
	if (dcb != NULL)
		dcb(fileName, crc32);
}

// 연결 성공시 호출되는 콜백
void CallConnectCallback()
{
	if (ccb != NULL)
		ccb();
}

// 연결이 해제될 경우 호출되는 콜백
void CallDisconnectCallback()
{
	if (dccb != NULL)
		dccb();
}

// 콜백함수 등록 함수
__declspec(dllexport) void RegisterCallbacks(DebugCallback* debugCallback, DBCallback * dbCallback, ConnectCallback * ccCallback, DisconnectCallback * dcCallback)
{
	dbgcb = debugCallback;
	dcb = dbCallback;
	ccb = ccCallback;
	dccb = dcCallback;
}