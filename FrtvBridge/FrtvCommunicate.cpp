#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include <fltUser.h>
#include <iostream>

#include "Protocol.h"
#include "FrtvCommunicate.h"
#include "IOCP.h"
#include "Callbacks.h"
#include "ConvertPath.h"

static HANDLE fltPort;

wchar_t* ChartoWChar(char* chr)
{
	wchar_t* pWchr = NULL;
	int chrSize = MultiByteToWideChar(CP_ACP, 0, chr, -1, NULL, NULL);
	pWchr = new WCHAR[chrSize];
	MultiByteToWideChar(CP_ACP, 0, chr, (int)strlen(chr) + 1, pWchr, chrSize);
	return pWchr;
}

/// <summary>
/// Ŀ�� ����̹� �޼��� ���� �Լ� CSTR ����
/// </summary>
/// <param name="rtvCode">�޼��� �ڵ�</param>
/// <param name="msg">������ �޼���</param>
/// <param name="crc32">���� CRC32 �� (null ����)</param>
/// <param name="fileSize">���� ũ�� (Ȯ���� ��Ͽ��� ���, null ����)</param>
/// <returns></returns>
__declspec(dllexport) int SendMinifltPortA(int rtvCode, LPCSTR msg, DWORD crc32, LONGLONG fileSize)
{
	USER_TO_FLT sent;			ZeroMemory(&sent, sizeof(sent));
	USER_TO_FLT_REPLY reply;	ZeroMemory(&reply, sizeof(reply));
	CHAR msgBuffer[4096];		ZeroMemory(msgBuffer, sizeof(msgBuffer));
	DWORD returned_bytes = 0;

	if (fltPort == NULL)
		return RTVSENDRESULT::RTV_SEND_PORT_NULL;

	// WCHAR�� ��ȯ
	PWCHAR tmp = ChartoWChar((LPSTR)msg);
	wcscpy_s(sent.Msg, tmp);

	// ChartoWChar���� �޸𸮸� �Ҵ��Ͽ���
	delete[] tmp;

	sent.RtvCode = rtvCode;
	sent.Crc32 = crc32;
	sent.FileSize = fileSize;

	HRESULT hr = FilterSendMessage(
		fltPort,
		&sent,
		sizeof(sent),
		&reply,
		sizeof(reply),
		&returned_bytes
	);

	if (IS_ERROR(hr))
	{
		sprintf_s(msgBuffer, "[SendMinifltPortA] FilterSendMessage() ����. HRESULT: %d", hr);
		CallDebugCallback(LOG_LEVEL_ERROR, msgBuffer);
	}
	else
	{
		if (returned_bytes > 0)
		{
			// �޼��� ���� �� Reply�� �ִ� ���
			sprintf_s(msgBuffer, "[SendMinifltPortA] FilterSendMessage() success with reply. Send: %s, Reply: %ws", msg, reply.Msg);
			CallDebugCallback(LOG_LEVEL_DEBUG, msgBuffer);
		}
		else
		{
			// �޼��� ���۸� �� ���
			sprintf_s(msgBuffer, "[SendMinifltPortA] FilterSendMessage() success with no reply. Send: %s", msg);
			CallDebugCallback(LOG_LEVEL_DEBUG, msgBuffer);
		}
	}

	return hr;
}

/// <summary>
/// Ŀ�� ����̹� �޼��� ���� �Լ� WSTR ����
/// </summary>
/// <param name="rtvCode">�޼��� �ڵ�</param>
/// <param name="msg">������ �޼���</param>
/// <param name="crc32">���� CRC32 �� (null ����)</param>
/// <param name="fileSize">���� ũ�� (Ȯ���� ��Ͽ��� ���, null ����)</param>
/// <returns></returns>
__declspec(dllexport) int SendMinifltPortW(int rtvCode, LPCWSTR msg, DWORD crc32, LONGLONG fileSize)
{
	USER_TO_FLT sent;			ZeroMemory(&sent, sizeof(sent));
	USER_TO_FLT_REPLY reply;	ZeroMemory(&reply, sizeof(reply));
	CHAR msgBuffer[4096];		ZeroMemory(msgBuffer, sizeof(msgBuffer));
	DWORD returned_bytes = 0;

	if (fltPort == NULL)
		return RTVSENDRESULT::RTV_SEND_PORT_NULL;

	wcscpy_s(sent.Msg, msg);
	sent.RtvCode = rtvCode;
	sent.Crc32 = crc32;
	sent.FileSize = fileSize;

	HRESULT hr = FilterSendMessage(
		fltPort,
		&sent,
		sizeof(sent),
		&reply,
		sizeof(reply),
		&returned_bytes
	);

	if (IS_ERROR(hr))
	{
		sprintf_s(msgBuffer, "[SendMinifltPortW] FilterSendMessage() ����. HRESULT: %d", hr);
		CallDebugCallback(LOG_LEVEL_ERROR, msgBuffer);
	}
	else
	{
		if (returned_bytes > 0)
		{
			// �޼��� ���� �� Reply�� �ִ� ���
			sprintf_s(msgBuffer, "[SendMinifltPortW] FilterSendMessage() success with reply. Send: %ws, Reply: %ws", msg, reply.Msg);
			CallDebugCallback(LOG_LEVEL_DEBUG, msgBuffer);
		}
		else
		{
			// �޼��� ���۸� �� ���
			sprintf_s(msgBuffer, "[SendMinifltPortW] FilterSendMessage() success with no reply. Send: %ws", msg);
			CallDebugCallback(LOG_LEVEL_DEBUG, msgBuffer);
		}
	}

	return hr;
}

/// <summary>
/// Ŀ�� ����̹��� ������ �õ��ϴ� �Լ�.
/// </summary>
/// <returns></returns>
__declspec(dllexport) int InitializeCommunicator()
{
	// WCHAR ������ ����
	setlocale(LC_ALL, "korean");
	_wsetlocale(LC_ALL, L"korean");

	HRESULT hr;
	int result = RTVCOMMRESULT::RTV_COMM_SUCCESS;
	DWORD returned_bytes = 0, wait_count = 0;
	DWORD threadId;
	PFLT_TO_USER_WRAPPER recv;
	WORKER_IOCP_PARAMS context;	ZeroMemory(&context, sizeof(context));
	OVERLAPPED overlapped;		ZeroMemory(&overlapped, sizeof(overlapped));
	CHAR msgBuffer[4096];		ZeroMemory(msgBuffer, sizeof(msgBuffer));

	// IOCP �����带 CPU �ھ� �� ��ŭ �����Ѵ�
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	PHANDLE threads = new HANDLE[sysinfo.dwNumberOfProcessors];
	HANDLE hbThread;

	while (true)
	{
		// �ϴ� WaitForMultipleObjects���� ���� ����ϱ� ������ �� �κ����� �ٽ� ���ƿ��� ����
		// IOCP �����尡 ��� ����� �����̴�. �翬���� ��� �ݺ��ϰԵ�.
		ConnectMinifltPort(&context, threads, sysinfo.dwNumberOfProcessors);
		fltPort = context.Port;

		SEND_HB_PARAMS params = { &context, sysinfo.dwNumberOfProcessors };
		hbThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)SendMinifltHeartbeat, &params, 0, &threadId);

#pragma prefast(suppress:__WARNING_MEMORY_LEAK, "IOCP �����忡�� �޸� �Ҵ��� ������ �޸� ���� �߻����� ����. �ش� ��� ���� ����.")
		recv = (PFLT_TO_USER_WRAPPER)malloc(sizeof(FLT_TO_USER_WRAPPER));

		if (recv == NULL)
		{
			hr = ERROR_NOT_ENOUGH_MEMORY;
			CallDebugCallback(LOG_LEVEL_ERROR, "[IOCP] ���� ���� �޸� �Ҵ� ����.");
			result = RTVCOMMRESULT::RTV_COMM_OUT_OF_MEMORY;
			break;
		}
		
		ZeroMemory(&recv->Ovl, sizeof(OVERLAPPED));
		
		// IOCP �����忡 �޼��� ������ ��û�Ѵ�
		hr = FilterGetMessage(context.Port,
			&recv->hdr,
			FIELD_OFFSET(FLT_TO_USER_WRAPPER, Ovl),
			&recv->Ovl);

		if (hr != HRESULT_FROM_WIN32(ERROR_IO_PENDING))
		{
			sprintf_s(msgBuffer, "[IOCP] FilterGetMessage() ���� �߻�. (HRESULT: %d)", hr);
			CallDebugCallback(LOG_LEVEL_ERROR, msgBuffer);
			result = hr;
			break;
		}
		
		// ���� �Ϸ� �� ���� �Ϸ� �ݹ��Լ� ȣ��
		CallConnectCallback();
		CallDebugCallback(LOG_LEVEL_NORMAL, "[frtvkrnl.sys] ����̹� ���� ����.");

		// ������� ���ѷ��� ������ ���ư��� ������ ������ �߻��� �� ���� ��ٸ��� �ȴ�.
		WaitForMultipleObjects(sysinfo.dwNumberOfProcessors, threads, TRUE, INFINITE);
	}

	CloseHandle(context.Port);
	CloseHandle(context.Completion);
	delete[] threads;
	threads = nullptr;

	if (context.Port)
	{
		FilterClose(context.Port);
	}

	return result;
}

__declspec(dllexport) int AddExceptionPath(LPCSTR path)
{
	int result = 0;
	PWCHAR wPath;
	WCHAR devicePath[MAX_PATH];
	RtlZeroMemory(devicePath, MAX_PATH * sizeof(WCHAR));
	wPath = ChartoWChar((LPSTR)path);
	GetDeivceFolderName(devicePath, wPath);

	result = SendMinifltPortW(RTVCMD_EXCPATH_ADD, devicePath, NULL, NULL);
	delete[] wPath;

	return result;
}

__declspec(dllexport) int RemoveExceptionPath(LPCSTR path)
{
	int result = 0;
	PWCHAR wPath;
	WCHAR devicePath[MAX_PATH];
	RtlZeroMemory(devicePath, MAX_PATH * sizeof(WCHAR));
	wPath = ChartoWChar((LPSTR)path);
	GetDeivceFolderName(devicePath, wPath);

	result = SendMinifltPortW(RTVCMD_EXCPATH_REMOVE, devicePath, NULL, NULL);
	delete[] wPath;

	return result;
}

__declspec(dllexport) int AddExtension(LPCSTR extension, LONGLONG maximumFileSize)
{
	int result = 0;
	PWCHAR wExt;
	wExt = ChartoWChar((LPSTR)extension);

	result = SendMinifltPortW(RTVCMD_EXTENSION_ADD, wExt, NULL, maximumFileSize);
	delete[] wExt;

	return result;
}

__declspec(dllexport) int RemoveExtension(LPCSTR extension)
{
	int result = 0;
	PWCHAR wExt;
	wExt = ChartoWChar((LPSTR)extension);
	result = SendMinifltPortW(RTVCMD_EXTENSION_REMOVE, wExt, NULL, NULL);
	delete[] wExt;

	return result;
}

__declspec(dllexport) int ToggleBackupSwitch(int enabled)
{
	int result = 0;
	if (enabled == 0)
	{
		result = SendMinifltPortW(RTVCMD_BACKUP_OFF, L"Backup OFF", NULL, NULL);
	}
	else
	{
		result = SendMinifltPortW(RTVCMD_BACKUP_ON, L"Backup ON", NULL, NULL);
	}

	return result;
}

__declspec(dllexport) int UpdateBackupFolder(LPCSTR folder)
{
	int result = 0;
	PWCHAR wFolder;
	WCHAR devicePath[2000];			RtlZeroMemory(devicePath, 2000 * sizeof(WCHAR));
	BOOLEAN deviceResult = FALSE;

	wFolder = ChartoWChar((LPSTR)folder);
	deviceResult = GetDeivceFolderName(devicePath, wFolder);
	delete[] wFolder;

	if (deviceResult == FALSE)
	{
		return 1;
	}

	result = SendMinifltPortW(RTVCMD_UPDATE_BACKUP_STORAGE, devicePath, NULL, NULL);

	return result;
}

__declspec(dllexport) int RestoreBackupFile(LPCSTR dstPath, DWORD crc32)
{
	int result = 0;
	PWCHAR wDstPath;
	WCHAR devicePath[2000];			RtlZeroMemory(devicePath, 2000 * sizeof(WCHAR));
	BOOLEAN deviceResult = FALSE;

	wDstPath = ChartoWChar((LPSTR)dstPath);
	deviceResult = GetDeivceFileName(devicePath, wDstPath);
	delete[] wDstPath;
	if (deviceResult == FALSE)
	{
		return 1;
	}

	result = SendMinifltPortW(RTVCMD_FILE_RESTORE, devicePath, crc32, NULL);

	return result;
}

__declspec(dllexport) int DeleteBackupFile(DWORD crc32)
{
	int result = 0;
	result = SendMinifltPortW(RTVCMD_FILE_DELETE_NORMAL, L"Delete cmd", crc32, NULL);

	return result;
}
