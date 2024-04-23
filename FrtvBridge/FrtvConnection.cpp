#include <windows.h>
#include <fltUser.h>
#include <stdio.h>
#include "IOCP.h"
#include "Protocol.h"
#include "Callbacks.h"
#include "FrtvConnection.h"

/// <summary>
/// ����̹��� ���ῡ ������ �� ���� 3�ʸ��� �翬���� �õ��ϴ� �Լ�
/// ���ῡ ������ ��� IOCP ��Ʈ�� Worker �����带 �����Ѵ�
/// </summary>
/// <param name="Context">IOCP Worker �Լ� ����</param>
/// <param name="threads">������ �ڵ� ���� �ּ�</param>
/// <param name="threadCount">������ ����</param>
/// <returns>���н� CreateThread() �� GetLastError() �ڵ�</returns>
DWORD ConnectMinifltPort(PWORKER_IOCP_PARAMS Context, PHANDLE threads, DWORD threadCount)
{
	int count = 1;
	HRESULT hr = 0;
	DWORD dwResult = 0;
	CHAR msgBuffer[4096];	ZeroMemory(msgBuffer, sizeof(msgBuffer));

	while (1)
	{
		// ���� ����̹��� ���� �õ�
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
			sprintf_s(msgBuffer, "[frtvkrnl.sys] ����̹��� ���� ����. �翬�� �õ� Ƚ��: %d, FilterConnectCommunicationPort() HRESULT: %d", count, hr);
			CallDebugCallback(LOG_LEVEL_ERROR, msgBuffer);
		}
		else
		{
			// IOCP ��Ʈ ����
			Context->Completion = CreateIoCompletionPort(Context->Port, NULL, 0, threadCount);
			// IOCP �����带 ����
			dwResult = CreateIOCPThreads(Context, threads, threadCount);
			if (dwResult != 0)
			{
				sprintf_s(msgBuffer, "[IOCP] ������ ���� ����. GetLastError(): %d", dwResult);
				CallDebugCallback(LOG_LEVEL_ERROR, msgBuffer);
			}
			
			break;
		}
		count++;
		Sleep(3000);
	}

	return dwResult;
}

/// <summary>
/// ����̹��� 5�ʸ��� Heartbeat �޼����� ������ FilterSendMessage �Լ� ȣ�⿡ �����ߴٸ� ������ �������� �Ǵ��Ѵ�
/// </summary>
/// <param name="params">IOCP Worker �Լ� ����, ������ ����, �ݹ��Լ� �ּ�</param>
VOID SendMinifltHeartbeat(PSEND_HB_PARAMS params)
{
	USER_TO_FLT sent;							ZeroMemory(&sent, sizeof(sent));
	USER_TO_FLT_REPLY reply;					ZeroMemory(&reply, sizeof(reply));
	CHAR msgBuffer[USER_TO_FLT_REPLY_MSG_SIZE]; ZeroMemory(msgBuffer, sizeof(msgBuffer));
	HRESULT hr = 0;
	DWORD returnedBytes = 0;

	while (true)
	{
		// 5�ʸ��� �����Ѵ�
		Sleep(5000);
		sent.RtvCode = RTV_HEARTBEAT;
		wcscpy_s(sent.Msg, ARRAYSIZE(sent.Msg), L"FrtvKrnl is you alive?");
		hr = FilterSendMessage(params->Context->Port, &sent, sizeof(sent), &reply, sizeof(reply), &returnedBytes);
		
		if (IS_ERROR(hr))
		{
			// Heartbeat ���� �� IOCP �����带 ��� �����Ѵ�.
			sprintf_s(msgBuffer, "[frtvkrnl.sys] ���� ������. FilterSendMessage() HRESULT: %d", hr);
			CallDebugCallback(LOG_LEVEL_ERROR, msgBuffer);
			// ������� ���α׷��� Ŀ�� ����̹��� ������ �����Ǿ����� �˸��� �Լ��� ȣ���Ѵ�.
			CallDisconnectCallback();
			for (int i = 0; i < params->ThreadCount; i++)
			{
				// IOCP ThreadWorker���� Ư�� key�� ������ ������ ����
				PostQueuedCompletionStatus(params->Context->Completion, 0, IOCP_QUIT_KEY, NULL);
			}
			break;
		}
		else
		{
			if (returnedBytes > 0)
			{
				sprintf_s(msgBuffer, "[frtvkrnl.sys] Heartbeat message received: %ws", reply.Msg);
				CallDebugCallback(LOG_LEVEL_DEBUG, msgBuffer);
				ZeroMemory(msgBuffer, sizeof(msgBuffer));
			}
			else
			{
				CallDebugCallback(LOG_LEVEL_DEBUG, "[frtvkrnl.sys] Heartbeat success but no message recieved.");
			}
		}
	}
}
