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
/// 커널 드라이버 메세지 전송 함수 CSTR 버전
/// </summary>
/// <param name="rtvCode">메세지 코드</param>
/// <param name="msg">전송할 메세지</param>
/// <param name="crc32">파일 CRC32 값 (null 가능)</param>
/// <param name="fileSize">파일 크기 (확장자 등록에서 사용, null 가능)</param>
/// <returns></returns>
__declspec(dllexport) int SendMinifltPortA(int rtvCode, LPCSTR msg, DWORD crc32, LONGLONG fileSize)
{
	USER_TO_FLT sent;			ZeroMemory(&sent, sizeof(sent));
	USER_TO_FLT_REPLY reply;	ZeroMemory(&reply, sizeof(reply));
	CHAR msgBuffer[4096];		ZeroMemory(msgBuffer, sizeof(msgBuffer));
	DWORD returned_bytes = 0;

	if (fltPort == NULL)
		return RTVSENDRESULT::RTV_SEND_PORT_NULL;

	// WCHAR로 변환
	PWCHAR tmp = ChartoWChar((LPSTR)msg);
	wcscpy_s(sent.Msg, tmp);

	// ChartoWChar에서 메모리를 할당하였음
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
		sprintf_s(msgBuffer, "[SendMinifltPortA] FilterSendMessage() 실패. HRESULT: %d", hr);
		CallDebugCallback(LOG_LEVEL_ERROR, msgBuffer);
	}
	else
	{
		if (returned_bytes > 0)
		{
			// 메세지 전송 후 Reply가 있는 경우
			sprintf_s(msgBuffer, "[SendMinifltPortA] FilterSendMessage() success with reply. Send: %s, Reply: %ws", msg, reply.Msg);
			CallDebugCallback(LOG_LEVEL_DEBUG, msgBuffer);
		}
		else
		{
			// 메세지 전송만 한 경우
			sprintf_s(msgBuffer, "[SendMinifltPortA] FilterSendMessage() success with no reply. Send: %s", msg);
			CallDebugCallback(LOG_LEVEL_DEBUG, msgBuffer);
		}
	}

	return hr;
}

/// <summary>
/// 커널 드라이버 메세지 전송 함수 WSTR 버전
/// </summary>
/// <param name="rtvCode">메세지 코드</param>
/// <param name="msg">전송할 메세지</param>
/// <param name="crc32">파일 CRC32 값 (null 가능)</param>
/// <param name="fileSize">파일 크기 (확장자 등록에서 사용, null 가능)</param>
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
		sprintf_s(msgBuffer, "[SendMinifltPortW] FilterSendMessage() 실패. HRESULT: %d", hr);
		CallDebugCallback(LOG_LEVEL_ERROR, msgBuffer);
	}
	else
	{
		if (returned_bytes > 0)
		{
			// 메세지 전송 후 Reply가 있는 경우
			sprintf_s(msgBuffer, "[SendMinifltPortW] FilterSendMessage() success with reply. Send: %ws, Reply: %ws", msg, reply.Msg);
			CallDebugCallback(LOG_LEVEL_DEBUG, msgBuffer);
		}
		else
		{
			// 메세지 전송만 한 경우
			sprintf_s(msgBuffer, "[SendMinifltPortW] FilterSendMessage() success with no reply. Send: %ws", msg);
			CallDebugCallback(LOG_LEVEL_DEBUG, msgBuffer);
		}
	}

	return hr;
}

/// <summary>
/// 커널 드라이버와 연결을 시도하는 함수.
/// </summary>
/// <returns></returns>
__declspec(dllexport) int InitializeCommunicator()
{
	// WCHAR 로케일 설정
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

#pragma prefast(suppress:__WARNING_MEMORY_LEAK, "IOCP 쓰레드에서 메모리 할당을 해제해 메모리 누수 발생하지 않음. 해당 경고 무시 가능.")
		recv = (PFLT_TO_USER_WRAPPER)malloc(sizeof(FLT_TO_USER_WRAPPER));

		if (recv == NULL)
		{
			hr = ERROR_NOT_ENOUGH_MEMORY;
			CallDebugCallback(LOG_LEVEL_ERROR, "[IOCP] 수신 버퍼 메모리 할당 실패.");
			result = RTVCOMMRESULT::RTV_COMM_OUT_OF_MEMORY;
			break;
		}
		
		ZeroMemory(&recv->Ovl, sizeof(OVERLAPPED));
		
		// IOCP 쓰레드에 메세지 수신을 요청한다
		hr = FilterGetMessage(context.Port,
			&recv->hdr,
			FIELD_OFFSET(FLT_TO_USER_WRAPPER, Ovl),
			&recv->Ovl);

		if (hr != HRESULT_FROM_WIN32(ERROR_IO_PENDING))
		{
			sprintf_s(msgBuffer, "[IOCP] FilterGetMessage() 오류 발생. (HRESULT: %d)", hr);
			CallDebugCallback(LOG_LEVEL_ERROR, msgBuffer);
			result = hr;
			break;
		}
		
		// 연결 완료 후 연결 완료 콜백함수 호출
		CallConnectCallback();
		CallDebugCallback(LOG_LEVEL_NORMAL, "[frtvkrnl.sys] 드라이버 연결 성공.");

		// 쓰레드는 무한루프 내에서 돌아가기 때문에 에러가 발생할 때 까지 기다리게 된다.
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
