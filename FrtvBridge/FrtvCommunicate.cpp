#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include <fltUser.h>
#include <mutex>

#include <format>
#include "Protocol.h"
#include "FrtvCommunicate.h"
#include "IOCP.h"
#include "Callbacks.h"
#include "ConvertPath.h"

#include <iostream>
#include <exception>
using namespace std;

static HANDLE fltPort;

wchar_t* ChartoWChar(char* chr)
{
	wchar_t* pWchr = NULL;
	int chrSize = MultiByteToWideChar(CP_ACP, 0, chr, -1, NULL, NULL);
	pWchr = new WCHAR[chrSize];
	MultiByteToWideChar(CP_ACP, 0, chr, (int)strlen(chr) + 1, pWchr, chrSize);
	return pWchr;
}

__declspec(dllexport) int SendMinifltPortA(int rtvCode, LPCSTR msg, DWORD crc32, LONGLONG fileSize)
{
	USER_TO_FLT sent;							ZeroMemory(&sent, sizeof(sent));
	USER_TO_FLT_REPLY reply;					ZeroMemory(&reply, sizeof(reply));
	CHAR msgBuffer[4096];	ZeroMemory(msgBuffer, sizeof(msgBuffer));
	DWORD returned_bytes = 0;

	if (fltPort == NULL)
		return 5;

	PWCHAR tmp = ChartoWChar((LPSTR)msg);
	wcscpy_s(sent.Msg, tmp);
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
		sprintf_s(msgBuffer, "[FrtvBridge]-[SendMinifltPortA] FilterSendMessage() failed. HRESULT: %d", hr);
		CallDebugCallback(msgBuffer);
		return 1;
	}
	else
	{
		if (returned_bytes > 0)
		{
			// 메세지 전송 후 Reply가 있는 경우
			sprintf_s(msgBuffer, "[FrvBridge]-[SendMinifltPortA] FilterSendMessage() success with reply. Send: %s, Reply: %ws", msg, reply.Msg);
			CallDebugCallback(msgBuffer);
		}
		else
		{
			// 메세지 전송만 한 경우
			sprintf_s(msgBuffer, "[FrvBridge]-[SendMinifltPortA] FilterSendMessage() success with no reply. Send: %s", msg);
			CallDebugCallback(msgBuffer);
		}
	}

	return hr;
}

__declspec(dllexport) int SendMinifltPortW(int rtvCode, LPCWSTR msg, DWORD crc32, LONGLONG fileSize)
{
	USER_TO_FLT sent;			ZeroMemory(&sent, sizeof(sent));
	USER_TO_FLT_REPLY reply;	ZeroMemory(&reply, sizeof(reply));
	CHAR msgBuffer[4096];	ZeroMemory(msgBuffer, sizeof(msgBuffer));
	DWORD returned_bytes = 0;

	if (fltPort == NULL)
		return 5;

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
		sprintf_s(msgBuffer, "[FrtvBridge]-[SendMinifltPortW] FilterSendMessage() failed. HRESULT: %d", hr);
		CallDebugCallback(msgBuffer);
	}
	else
	{
		if (returned_bytes > 0)
		{
			// 메세지 전송 후 Reply가 있는 경우
			sprintf_s(msgBuffer, "[FrvBridge]-[SendMinifltPortW] FilterSendMessage() success with reply. Send: %ws, Reply: %ws", msg, reply.Msg);
			CallDebugCallback(msgBuffer);
		}
		else
		{
			// 메세지 전송만 한 경우
			sprintf_s(msgBuffer, "[FrvBridge]-[SendMinifltPortW] FilterSendMessage() success with no reply. Send: %ws", msg);
			CallDebugCallback(msgBuffer);
		}
	}

	return hr;
}

__declspec(dllexport) int InitializeCommunicator()
{
	// WCHAR 로케일 설정
	setlocale(LC_ALL, "korean");
	_wsetlocale(LC_ALL, L"korean");

	HRESULT hr;
	DWORD returned_bytes = 0, wait_count = 0;
	DWORD threadId;
	PFLT_TO_USER_WRAPPER recv;
	WORKER_IOCP_PARAMS context;	ZeroMemory(&context, sizeof(context));
	OVERLAPPED overlapped;		ZeroMemory(&overlapped, sizeof(overlapped));
	CHAR msgBuffer[4096];		ZeroMemory(msgBuffer, sizeof(msgBuffer));

	// IOCP 스레드를 CPU 코어 수 만큼 생성한다
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	PHANDLE threads = new HANDLE[sysinfo.dwNumberOfProcessors];
	HANDLE hbThread;

	while (true)
	{
		// 하단 WaitForMultipleObjects에서 무한 대기하기 때문에 이 부분으로 다시 돌아오는 경우는
		// IOCP 쓰레드가 모두 종료된 이후이다. 재연결을 계속 반복하게됨.
		ConnectMinifltPort(&context, threads, sysinfo.dwNumberOfProcessors);
		fltPort = context.Port;

		SEND_HB_PARAMS params = { &context, sysinfo.dwNumberOfProcessors };
		hbThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)SendMinifltHeartbeat, &params, 0, &threadId);

#pragma prefast(suppress:__WARNING_MEMORY_LEAK, "IOCP 쓰레드에서 메모리 할당을 해제해 메모리 누수 발생하지 않음. 경고 무시 가능.")
		recv = (PFLT_TO_USER_WRAPPER)malloc(sizeof(FLT_TO_USER_WRAPPER));

		if (recv == NULL)
		{
			hr = ERROR_NOT_ENOUGH_MEMORY;
			CallDebugCallback("[FrvBridge]-[Communicator] IOCP FLT_TO_USER_WRAPPER memory allocation failed.");
			break;
		}
		
		ZeroMemory(&recv->Ovl, sizeof(OVERLAPPED));
		
		//
		//  Request messages from the filter driver.
		//

		hr = FilterGetMessage(context.Port,
			&recv->hdr,
			FIELD_OFFSET(FLT_TO_USER_WRAPPER, Ovl),
			&recv->Ovl);

		if (hr != HRESULT_FROM_WIN32(ERROR_IO_PENDING))
		{
			sprintf_s(msgBuffer, "[FrvBridge]-[Communicator] FilterGetMessage() failed. HRESULT: %d", hr);
			CallDebugCallback(msgBuffer);
			break;
		}

		CallConnectCallback();
		CallDebugCallback("[FrvBridge]-[Communicator] Connect success.");
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

	return 0;
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