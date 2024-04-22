#include <windows.h>
#include <fltUser.h>
#include <stdio.h>
#include "IOCP.h"
#include "Protocol.h"
#include "Callbacks.h"
#include "FrtvConnection.h"

/// <summary>
/// 드라이버와 연결에 성공할 때 까지 3초마다 재연결을 시도하는 함수
/// 연결에 성공할 경우 IOCP 포트와 Worker 스레드를 생성한다
/// </summary>
/// <param name="Context">IOCP Worker 함수 인자</param>
/// <param name="threads">스레드 핸들 변수 주소</param>
/// <param name="threadCount">스레드 갯수</param>
/// <returns></returns>
VOID ConnectMinifltPort(PWORKER_IOCP_PARAMS Context, PHANDLE threads, DWORD threadCount)
{
	int count = 1;
	HRESULT hr = 0;
	DWORD dwResult = 0;
	CHAR msgBuffer[4096];	ZeroMemory(msgBuffer, sizeof(msgBuffer));

	while (1)
	{
		// 필터 드라이버와 연결 시도
		hr = FilterConnectCommunicationPort(
			TEXT(MINIFLT_EXAMPLE_PORT_NAME),
			0,
			nullptr,
			0,
			nullptr,
			&Context->Port
		);

		if (IS_ERROR(hr))
		{
			sprintf_s(msgBuffer, "[FrtvBridge]-[ConnectMinifltProt] FilterConnectCommunicationPort() failed. HRESULT: %d, Reconnecting attempt: %d", hr, count);
			CallDebugCallback(msgBuffer);
		}
		else
		{
			CallDebugCallback("[FrtvBridge]-[ConnectMinifltProt] FilterConnectCommunicationPort() success.");

			// IOCP 포트 생성
			Context->Completion = CreateIoCompletionPort(Context->Port, NULL, 0, threadCount);
			// IOCP 스레드를 생성
			dwResult = CreateIOCPThreads(Context, threads, threadCount);
			if (dwResult != 0)
			{
				// TODO: IOCP 실패시 어떻게 할지 고민
				sprintf_s(msgBuffer, "[FrtvBridge]-[ConnectMinifltProt] CreateIOCPThreads failed. GetLastError(): %d", dwResult);
				CallDebugCallback(msgBuffer);
			}
			
			break;
		}
		count++;
		Sleep(3000);
	}
}

/// <summary>
/// 드라이버와 5초마다 Heartbeat 메세지를 전송해 FilterSendMessage 함수 호출에 실패했다면 연결이 끊어졌다 판단한다
/// </summary>
/// <param name="params">IOCP Worker 함수 인자, 스레드 갯수, 콜백함수 주소</param>
VOID SendMinifltHeartbeat(PSEND_HB_PARAMS params)
{
	USER_TO_FLT sent;							ZeroMemory(&sent, sizeof(sent));
	USER_TO_FLT_REPLY reply;					ZeroMemory(&reply, sizeof(reply));
	CHAR msgBuffer[USER_TO_FLT_REPLY_MSG_SIZE]; ZeroMemory(msgBuffer, sizeof(msgBuffer));
	HRESULT hr = 0;
	DWORD returnedBytes = 0;

	while (true)
	{
		// 5초마다 전송한다
		Sleep(5000);
		sent.RtvCode = RTV_HEARTBEAT;
		wcscpy_s(sent.Msg, ARRAYSIZE(sent.Msg), L"FrtvKrnl is you alive?");
		hr = FilterSendMessage(params->Context->Port, &sent, sizeof(sent), &reply, sizeof(reply), &returnedBytes);
		
		if (IS_ERROR(hr))
		{
			// Heartbeat 실패 시 IOCP 쓰레드를 모두 종료한다.
			sprintf_s(msgBuffer, "[FrtvBridge] Heartbeat failed. HRESULT: %d", hr);
			CallDebugCallback(msgBuffer);
			// 유저모드 프로그램에 커널 드라이버와 연결이 해제되었음을 알리는 함수를 호출한다.
			CallDisconnectCallback();
			for (int i = 0; i < params->ThreadCount; i++)
			{
				// IOCP ThreadWorker에게 특정 key를 전송해 스레드 종료
				PostQueuedCompletionStatus(params->Context->Completion, 0, IOCP_QUIT_KEY, NULL);
			}
			break;
		}
		else
		{
			if (returnedBytes > 0)
			{
				sprintf_s(msgBuffer, "[FrvBridge] Heartbeat message received: %ws", reply.Msg);
				CallDebugCallback(msgBuffer);
				ZeroMemory(msgBuffer, sizeof(msgBuffer));
			}
			else
			{
				CallDebugCallback("[FrtvBridge] Heartbeat success but no message recieved.");
			}
		}
	}
}
