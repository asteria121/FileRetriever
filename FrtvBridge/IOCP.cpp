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
/// IOCP ������� �۾��� �����ϴ� �Լ�
/// ����� �����ϴ� �������̱� ������ ���� �߻��� ������ ����� ���α׷��� �α׸� �����
/// </summary>
/// <param name="Context">IOCP ������ �Լ��� �����ϴ� �Ķ���͸� ���� ����ü</param>
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

	// ���� ũ�� ���� ������ �Ϻδ� ���� �Ҵ��ؾ���.
	PFLT_TO_USER_WRAPPER recv = new FLT_TO_USER_WRAPPER;
	PFLT_TO_USER_REPLY_WRAPPER recv_reply = new FLT_TO_USER_REPLY_WRAPPER;

	if (recv == nullptr || recv_reply == nullptr)
	{
		sprintf_s(msgBuffer, "[IOCPWorker] ���� ���� �޸� �Ҵ� ����.");
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
			sprintf_s(msgBuffer, "[IOCPWorker] GetQueuedCompletionStatus() ����. HRESULT: %d", hr);
			CallDebugCallback(LOG_LEVEL_ERROR, msgBuffer);
			break;
		}

		if (key == IOCP_QUIT_KEY)
		{
			// �̹� ������ ������ ��� �α��� �ϱ⶧���� �ش� ������ ����� �޼����θ� �α��Ѵ�.
			CallDebugCallback(LOG_LEVEL_DEBUG, "[IOCPWorker] Thread terminate key received. Thread is stopping.");
			break;
		}

		message = CONTAINING_RECORD(pOvlp, FLT_TO_USER_WRAPPER, Ovl);
		data = &message->data;

		if (data->RtvCode == RTV_BACKUP_ALERT)
		{
			// Ŀ�� ��θ� LPCSTR ������ Win32 ��η� ��ȯ �� DB �ݹ� ����
			GetWin32FileName(data->Msg, dosFilePath);
			sprintf_s(msgBuffer, "%ws", dosFilePath);
			CallDBCallback(msgBuffer, data->Crc32);
			sprintf_s(msgBuffer, "���� ��� �Ϸ�: %ws", dosFilePath);
			CallDebugCallback(LOG_LEVEL_NORMAL, msgBuffer);
		}
		else if (data->RtvCode == RTV_DBG_MESSAGE)
		{
			// ����� �޼����� LPCSTR�� ��ȯ �� ����
			sprintf_s(msgBuffer, "[frtvkrnl.sys] Ŀ�� ����̹� ����� �޼���: %ws", data->Msg);
			// Ŀ�� �޼����� ���Ź޾ұ� ������ ���λ縦 FrtvKrnl�� �Ѵ�.
			CallDebugCallback(LOG_LEVEL_DEBUG, msgBuffer);
		}
		else
		{
			// ó������ ���� �ڵ��� �޼����� ǥ���Ѵ�.
			sprintf_s(msgBuffer, "[frtvkrnl.sys] �� �� ���� Ŀ�� ����̹� �޼��� ����. RtvCode: %d", data->RtvCode);
			CallDebugCallback(LOG_LEVEL_DEBUG, msgBuffer);
		}

		memset(&message->Ovl, 0, sizeof(OVERLAPPED));
		hr = FilterGetMessage(Context->Port,
			&message->hdr,
			FIELD_OFFSET(FLT_TO_USER_WRAPPER, Ovl),
			&message->Ovl);

		// IOCP ������� FilterGetMessage() �� ERROR_IO_PENDING�� ��ȯ�ؾ���
		if (hr != HRESULT_FROM_WIN32(ERROR_IO_PENDING))
		{
			sprintf_s(msgBuffer, "[IOCPWorker] FilterGetMessage() ����. HRESULT: %d", hr);
			CallDebugCallback(LOG_LEVEL_ERROR, msgBuffer);
			break;
		}

		// Reply�� ������� ���� ��� ��� �������� �Ϻ� �޼��� �ս��� �߻��� �� ����.
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
				CallDebugCallback(LOG_LEVEL_ERROR, "[IOCPWorker] FilterReplyMessage() ����. HRESULT: ERROR_INVALID_HANDLE\n");
				break;
			}
			else
			{
				sprintf_s(msgBuffer, "[IOCPWorker] FilterReplyMessage() ����. HRESULT: %d", hr);
				CallDebugCallback(LOG_LEVEL_ERROR, msgBuffer);
				break;
			}
		}

		// �޼��� ���� ���� �ʱ�ȭ
		ZeroMemory(dosFilePath, sizeof(dosFilePath));
		ZeroMemory(msgBuffer, sizeof(msgBuffer));
	}

	if (message != NULL)
		free(message);

	delete recv;
	delete recv_reply;

	return hr;
}