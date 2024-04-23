#include <Windows.h>
#include <stdio.h>
#include "FrtvConnection.h"
#include "Protocol.h"
#include "FrtvCommunicate.h"
#include "IOCP.h"
#include "Callbacks.h"
#include "ConvertPath.h"

DWORD CreateIOCPThreads(PWORKER_IOCP_PARAMS Context, PHANDLE iocpThreads, DWORD threadCount)
{
	DWORD threadId;

	CallDebugCallback(LOG_LEVEL_DEBUG, "[IOCP] Creating IOCP threads.");
	for (DWORD i = 0; i < threadCount; i++)
	{
		iocpThreads[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)FrtvWorker, Context, 0, &threadId);

		if (iocpThreads[i] == NULL)
		{
			return GetLastError();
		}
	}

	return 0;
}

/// <summary>
/// IOCP 쓰레드로 작업을 수행하는 함수
/// 통신을 진행하는 스레드이기 때문에 에러 발생시 무조건 사용자 프로그램에 로그를 남긴다
/// </summary>
/// <param name="Context">IOCP 쓰레드 함수에 전달하는 파라미터를 담은 구조체</param>
/// <returns></returns>
DWORD FrtvWorker(PWORKER_IOCP_PARAMS Context)
{
	CallDebugCallback(LOG_LEVEL_DEBUG, "[IOCPWorker] IOCP thread started.");

	LPOVERLAPPED pOvlp;
	BOOL result;
	DWORD outSize;
	HRESULT hr = 0;
	ULONG_PTR key;
	PFLT_TO_USER_WRAPPER message;	ZeroMemory(&message, sizeof(message));
	PFLT_TO_USER data;
	WCHAR dosFilePath[FLT_TO_USER_MSG_SIZE];	ZeroMemory(dosFilePath, sizeof(dosFilePath));
	CHAR msgBuffer[4096];						ZeroMemory(msgBuffer, sizeof(msgBuffer));

	// 스택 크기 제한 때문에 일부는 힙에 할당해야함.
	PFLT_TO_USER_WRAPPER recv = new FLT_TO_USER_WRAPPER;
	PFLT_TO_USER_REPLY_WRAPPER recv_reply = new FLT_TO_USER_REPLY_WRAPPER;

	if (recv == nullptr || recv_reply == nullptr)
	{
		sprintf_s(msgBuffer, "[IOCPWorker] 수신 버퍼 메모리 할당 실패.");
		CallDebugCallback(LOG_LEVEL_ERROR, msgBuffer);
		return 1;
	}

	ZeroMemory(recv, sizeof(FLT_TO_USER_WRAPPER));
	ZeroMemory(recv_reply, sizeof(FLT_TO_USER_REPLY_WRAPPER));

	while (true)
	{
		result = GetQueuedCompletionStatus(Context->Completion, &outSize, &key, &pOvlp, INFINITE);

		if (!result)
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
			sprintf_s(msgBuffer, "[IOCPWorker] GetQueuedCompletionStatus() 실패. HRESULT: %d", hr);
			CallDebugCallback(LOG_LEVEL_ERROR, msgBuffer);
			break;
		}

		if (key == IOCP_QUIT_KEY)
		{
			// 이미 연결이 끊어진 경우 로깅을 하기때문에 해당 사항은 디버그 메세지로만 로깅한다.
			CallDebugCallback(LOG_LEVEL_DEBUG, "[IOCPWorker] Thread terminate key received. Thread is stopping.");
			break;
		}

		message = CONTAINING_RECORD(pOvlp, FLT_TO_USER_WRAPPER, Ovl);
		data = &message->data;

		if (data->RtvCode == RTV_BACKUP_ALERT)
		{
			// 커널 경로를 LPCSTR 형태의 Win32 경로로 변환 후 DB 콜백 실행
			GetWin32FileName(data->Msg, dosFilePath);
			sprintf_s(msgBuffer, "%ws", dosFilePath);
			CallDBCallback(msgBuffer, data->Crc32);
			sprintf_s(msgBuffer, "파일 백업 완료: %ws", dosFilePath);
			CallDebugCallback(LOG_LEVEL_NORMAL, msgBuffer);
		}
		else if (data->RtvCode == RTV_DBG_MESSAGE)
		{
			// 디버그 메세지를 LPCSTR로 변환 후 전송
			sprintf_s(msgBuffer, "[frtvkrnl.sys] 커널 드라이버 디버그 메세지: %ws", data->Msg);
			// 커널 메세지를 수신받았기 때문에 접두사를 FrtvKrnl로 한다.
			CallDebugCallback(LOG_LEVEL_DEBUG, msgBuffer);
		}
		else
		{
			// 처리되지 않은 코드의 메세지를 표기한다.
			sprintf_s(msgBuffer, "[frtvkrnl.sys] 알 수 없는 커널 드라이버 메세지 수신. RtvCode: %d", data->RtvCode);
			CallDebugCallback(LOG_LEVEL_DEBUG, msgBuffer);
		}

		memset(&message->Ovl, 0, sizeof(OVERLAPPED));
		hr = FilterGetMessage(Context->Port,
			&message->hdr,
			FIELD_OFFSET(FLT_TO_USER_WRAPPER, Ovl),
			&message->Ovl);

		// IOCP 쓰레드는 FilterGetMessage() 후 ERROR_IO_PENDING을 반환해야함
		if (hr != HRESULT_FROM_WIN32(ERROR_IO_PENDING))
		{
			sprintf_s(msgBuffer, "[IOCPWorker] FilterGetMessage() 실패. HRESULT: %d", hr);
			CallDebugCallback(LOG_LEVEL_ERROR, msgBuffer);
			break;
		}

		// Reply를 사용하지 않을 경우 통신 과정에서 일부 메세지 손실이 발생할 수 있음.
		wcscpy_s(recv_reply->data.Msg, L"Kernel message receive success.");
		recv_reply->hdr.MessageId = message->hdr.MessageId;
		recv_reply->data.RtvResult = 0;

		hr = FilterReplyMessage(
			Context->Port,
			&recv_reply->hdr,
			sizeof(recv_reply->hdr) + sizeof(recv_reply->data)
		);

		if (IS_ERROR(hr))
		{
			if (hr == HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE))
			{
				CallDebugCallback(LOG_LEVEL_ERROR, "[IOCPWorker] FilterReplyMessage() 실패. HRESULT: ERROR_INVALID_HANDLE\n");
				break;
			}
			else
			{
				sprintf_s(msgBuffer, "[IOCPWorker] FilterReplyMessage() 실패. HRESULT: %d", hr);
				CallDebugCallback(LOG_LEVEL_ERROR, msgBuffer);
				break;
			}
		}

		// 메세지 수신 버퍼 초기화
		ZeroMemory(dosFilePath, sizeof(dosFilePath));
		ZeroMemory(msgBuffer, sizeof(msgBuffer));
	}

	if (message != NULL)
		free(message);

	delete recv;
	delete recv_reply;

	return hr;
}