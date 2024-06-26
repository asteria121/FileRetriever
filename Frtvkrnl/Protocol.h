#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_

typedef enum RTVCMDCODE
{
	RTV_BACKUP_ALERT,
	RTVCMD_FILE_MANUAL_BACKUP,
	RTVCMD_FILE_RESTORE,
	RTVCMD_FILE_RESTORE_OVERWRITE,
	RTVCMD_FILE_DELETE_NORMAL,
	RTVCMD_FILE_DELETE_1PASS,
	RTVCMD_FILE_DELETE_3PASS,
	RTVCMD_FILE_DELETE_7PASS,
	RTV_HEARTBEAT,
	RTVCMD_UPDATE_BACKUP_STORAGE,
	RTVCMD_EXCPATH_ADD,
	RTVCMD_EXCPATH_REMOVE,
	RTVCMD_EXTENSION_ADD,
	RTVCMD_EXTENSION_REMOVE,
	RTVCMD_BACKUP_ON,
	RTVCMD_BACKUP_OFF,
	RTV_DBG_MESSAGE,
	RTVCMD_INCPATH_ADD,
	RTVCMD_INCPATH_REMOVE
} RTVCMDCODE;

typedef enum RTVCMDRESULT
{
	RTVCMD_SUCCESS,
	RTVCMD_FAIL_INVALID_PARAMETER,
	RTVCMD_FAIL_OUT_OF_MEMORY,
	RTVCMD_FAIL_LONG_STRING,
	RTVCMD_NO_BACKUPFOLDER_TO_SYSTEM
} RTVCMDRESULT;

#define MEMORY_PAGE_SIZE 4096
#define MINIFLT_EXAMPLE_PORT_NAME "\\FrtvPort"

// KERNEL -> USER
// 사용처 : 파일 정보 전송
typedef struct _FLT_TO_USER
{
	DWORD RtvCode;
	WCHAR Msg[(MEMORY_PAGE_SIZE / sizeof(WCHAR)) - (sizeof(DWORD) * 2 / sizeof(WCHAR)) - (sizeof(LONGLONG) / sizeof(WCHAR))];
	DWORD Crc32;
	LONGLONG FileSize;
} FLT_TO_USER, * PFLT_TO_USER;

// KERNEL -> USER REPLY
// 사용처 : DB 백업 결과 알림
typedef struct _FLT_TO_USER_REPLY
{
	DWORD RtvResult;
	WCHAR Msg[(MEMORY_PAGE_SIZE / sizeof(WCHAR)) - (sizeof(DWORD) / sizeof(WCHAR))];
} FLT_TO_USER_REPLY, * PFLT_TO_USER_REPLY;

// USER -> KERNEL
// 사용처 : 설정 추가 및 삭제
typedef struct _USER_TO_FLT
{
	DWORD RtvCode;
	WCHAR Msg[(MEMORY_PAGE_SIZE / sizeof(WCHAR)) - (sizeof(DWORD) * 2 / sizeof(WCHAR)) - (sizeof(LONGLONG) / sizeof(WCHAR))];
	DWORD Crc32;
	LONGLONG FileSize;
} USER_TO_FLT, * PUSER_TO_FLT;

// USER -> KERNEL REPLY
// 사용처 : 설정 업데이트 결과 알림
typedef struct _USER_TO_FLT_REPLY
{
	DWORD RtvResult;
	WCHAR Msg[(MEMORY_PAGE_SIZE / sizeof(WCHAR)) - (sizeof(DWORD) / sizeof(WCHAR))];
} USER_TO_FLT_REPLY, * PUSER_TO_FLT_REPLY;

NTSTATUS ParseUsermodeCommand(
	_In_		PUSER_TO_FLT cmd,
	_Out_opt_	PVOID replyBuffer,
	_In_		ULONG replyBufferSize,
	_Out_		PULONG replyBufferNumberOfWrittenBytes
);

NTSTATUS SendFileInformation(
	_In_	PWCHAR sendMsg,
	_In_	ULONG crc32,
	_In_	LONGLONG fileSize
);

NTSTATUS SendDebugMessage(
	_In_	PWCHAR sendMsg
);

#endif