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
/// 커널 드라이버 메세지 전송 함수 WSTR 버전
/// </summary>
/// <param name="rtvCode">메세지 코드</param>
/// <param name="msg">전송할 메세지</param>
/// <param name="crc32">파일 CRC32 값 (null 가능)</param>
/// <param name="fileSize">파일 크기 (확장자 등록에서 사용, null 가능)</param>
/// <returns></returns>
int SendMinifltPortW(int rtvCode, LPCWSTR msg, DWORD crc32, LONGLONG fileSize, HRESULT* hr)
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
	
	*hr = FilterSendMessage(
		fltPort,
		&sent,
		sizeof(sent),
		&reply,
		sizeof(reply),
		&returned_bytes
	);

	return reply.RtvResult;
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

__declspec(dllexport) int AddExceptionPath(LPCSTR path, HRESULT* hr)
{
	int result = 0;
	PWCHAR wPath;
	WCHAR devicePath[MAX_PATH];
	RtlZeroMemory(devicePath, MAX_PATH * sizeof(WCHAR));
	wPath = ChartoWChar((LPSTR)path);
	GetDeivceFolderName(devicePath, wPath);

	result = SendMinifltPortW(RTVCMD_EXCPATH_ADD, devicePath, NULL, NULL, hr);
	delete[] wPath;

	return result;
}

__declspec(dllexport) int RemoveExceptionPath(LPCSTR path, HRESULT* hr)
{
	int result = 0;
	PWCHAR wPath;
	WCHAR devicePath[MAX_PATH];
	RtlZeroMemory(devicePath, MAX_PATH * sizeof(WCHAR));
	wPath = ChartoWChar((LPSTR)path);
	GetDeivceFolderName(devicePath, wPath);

	result = SendMinifltPortW(RTVCMD_EXCPATH_REMOVE, devicePath, NULL, NULL, hr);
	delete[] wPath;

	return result;
}

__declspec(dllexport) int AddIncludePath(LPCSTR path, LONGLONG maximumFileSize, HRESULT* hr)
{
	int result = 0;
	PWCHAR wPath;
	WCHAR devicePath[MAX_PATH];
	RtlZeroMemory(devicePath, MAX_PATH * sizeof(WCHAR));
	wPath = ChartoWChar((LPSTR)path);
	GetDeivceFolderName(devicePath, wPath);

	result = SendMinifltPortW(RTVCMD_INCPATH_ADD, devicePath, NULL, maximumFileSize, hr);
	delete[] wPath;

	return result;
}

__declspec(dllexport) int RemoveIncludePath(LPCSTR path, HRESULT* hr)
{
	int result = 0;
	PWCHAR wPath;
	WCHAR devicePath[MAX_PATH];
	RtlZeroMemory(devicePath, MAX_PATH * sizeof(WCHAR));
	wPath = ChartoWChar((LPSTR)path);
	GetDeivceFolderName(devicePath, wPath);

	result = SendMinifltPortW(RTVCMD_INCPATH_REMOVE, devicePath, NULL, NULL, hr);
	delete[] wPath;

	return result;
}

__declspec(dllexport) int AddExtension(LPCSTR extension, LONGLONG maximumFileSize, HRESULT* hr)
{
	int result = 0;
	PWCHAR wExt;
	wExt = ChartoWChar((LPSTR)extension);

	result = SendMinifltPortW(RTVCMD_EXTENSION_ADD, wExt, NULL, maximumFileSize, hr);
	delete[] wExt;

	return result;
}

__declspec(dllexport) int RemoveExtension(LPCSTR extension, HRESULT* hr)
{
	int result = 0;
	PWCHAR wExt;
	wExt = ChartoWChar((LPSTR)extension);
	result = SendMinifltPortW(RTVCMD_EXTENSION_REMOVE, wExt, NULL, NULL, hr);
	delete[] wExt;

	return result;
}

__declspec(dllexport) int ToggleBackupSwitch(int enabled, HRESULT* hr)
{
	int result = 0;
	if (enabled == 0)
		result = SendMinifltPortW(RTVCMD_BACKUP_OFF, L"Backup OFF", NULL, NULL, hr);
	else
		result = SendMinifltPortW(RTVCMD_BACKUP_ON, L"Backup ON", NULL, NULL, hr);

	return result;
}

__declspec(dllexport) int UpdateBackupFolder(LPCSTR folder, HRESULT* hr)
{
	int result = 0;
	PWCHAR wFolder;
	WCHAR devicePath[2000];			RtlZeroMemory(devicePath, 2000 * sizeof(WCHAR));
	BOOLEAN deviceResult = FALSE;

	wFolder = ChartoWChar((LPSTR)folder);
	deviceResult = GetDeivceFolderName(devicePath, wFolder);
	delete[] wFolder;

	if (deviceResult == FALSE)
		return 1;

	result = SendMinifltPortW(RTVCMD_UPDATE_BACKUP_STORAGE, devicePath, NULL, NULL, hr);

	return result;
}

__declspec(dllexport) int RestoreBackupFile(LPCSTR dstPath, DWORD crc32, BOOLEAN overwriteDst, HRESULT* hr)
{
	int result = 0;
	PWCHAR wDstPath;
	WCHAR devicePath[2000];			RtlZeroMemory(devicePath, 2000 * sizeof(WCHAR));
	BOOLEAN deviceResult = FALSE;

	wDstPath = ChartoWChar((LPSTR)dstPath);
	deviceResult = GetDeivceFileName(devicePath, wDstPath);
	delete[] wDstPath;
	if (deviceResult == FALSE)
		return 1;

	if (overwriteDst == TRUE)
		result = SendMinifltPortW(RTVCMD_FILE_RESTORE_OVERWRITE, devicePath, crc32, NULL, hr);
	else
		result = SendMinifltPortW(RTVCMD_FILE_RESTORE, devicePath, crc32, NULL, hr);

	return result;
}

__declspec(dllexport) int DeleteBackupFile(DWORD crc32, HRESULT* hr)
{
	int result = 0;
	result = SendMinifltPortW(RTVCMD_FILE_DELETE_NORMAL, L"Delete cmd", crc32, NULL, hr);

	return result;
}
