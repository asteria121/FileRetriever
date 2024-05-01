#include <fltKernel.h>
#include <ntstrsafe.h>

#include "Protocol.h"
#include "MiniFilterPort.h"
#include "ManageFile.h"
#include "Utility.h"
#include "SettingsGeneral.h"
#include "SettingsExtension.h"
#include "SettingsPath.h"
#include "SettingsIncludePath.h"

// 커널 드라이버가 수신받은 미니필터 메세지를 처리하는 함수
NTSTATUS ParseUsermodeCommand(
	_In_		PUSER_TO_FLT cmd,
	_Out_opt_	PVOID replyBuffer,
	_In_		ULONG replyBufferSize,
	_Out_		PULONG replyBufferNumberOfWrittenBytes
)
{
	UNREFERENCED_PARAMETER(replyBufferSize);

	NTSTATUS status = STATUS_SUCCESS;
	PUSER_TO_FLT_REPLY reply = (PUSER_TO_FLT_REPLY)replyBuffer;
	size_t replySize;
	UNICODE_STRING usBackupPath, usDestPath;
	WCHAR backupPath[MAX_PATH];		RtlZeroMemory(backupPath, sizeof(backupPath));
	LPWSTR backupFolder = NULL;
	//WCHAR tmp[2044];				RtlZeroMemory(tmp, sizeof(tmp));
	//SIZE_T maximumReplySize = (BUFFER_SIZE / sizeof(WCHAR)) - (sizeof(DWORD) * 1);
	SIZE_T maximumReplySize = ARRAYSIZE(reply->Msg);

	if (cmd->RtvCode == RTVCMD_FILE_DELETE_NORMAL)
	{
		if (cmd->Crc32 == 0)
		{
			status = RTVCMD_FAIL_INVALID_PARAMETER;
			RtlStringCchPrintfW(reply->Msg, maximumReplySize, L"%ws remove failed.", backupPath);
		}
		else
		{
			backupFolder = GetBackupPath();
			RtlStringCchPrintfW(backupPath, MAX_PATH, L"%ws%08lX", backupFolder, cmd->Crc32);
			RtlInitUnicodeString(&usBackupPath, backupPath);

			status = DeleteFile(&usBackupPath);
			if (!NT_SUCCESS(status))
				RtlStringCchPrintfW(reply->Msg, maximumReplySize, L"%ws remove failed.", backupPath);
			else
				RtlStringCchPrintfW(reply->Msg, maximumReplySize, L"%ws remove success.", backupPath);
		}
	}
	else if (cmd->RtvCode == RTVCMD_FILE_RESTORE)
	{
		RtlInitUnicodeString(&usDestPath, cmd->Msg);

		if (cmd->Crc32 == 0)
		{
			status = RTVCMD_FAIL_INVALID_PARAMETER;
			RtlStringCchPrintfW(reply->Msg, maximumReplySize, L"%ws restore failed.\r\n", backupPath);
		}
		else
		{
			backupFolder = GetBackupPath();
			RtlStringCchPrintfW(backupPath, MAX_PATH, L"%ws%08lX", backupFolder, cmd->Crc32);
			RtlInitUnicodeString(&usBackupPath, backupPath);

			status = RestoreFile(&usDestPath, &usBackupPath, FALSE);
			if (!NT_SUCCESS(status))
				RtlStringCchPrintfW(reply->Msg, maximumReplySize, L"%ws restore failed.\r\n", backupPath);
			else
				RtlStringCchPrintfW(reply->Msg, maximumReplySize, L"%ws restore success.\r\n", backupPath);
		}
	}
	else if (cmd->RtvCode == RTVCMD_FILE_RESTORE_OVERWRITE)
	{
		RtlInitUnicodeString(&usDestPath, cmd->Msg);

		if (cmd->Crc32 == 0)
		{
			status = RTVCMD_FAIL_INVALID_PARAMETER;
			RtlStringCchPrintfW(reply->Msg, maximumReplySize, L"%ws restore failed.\r\n", backupPath);
		}
		else
		{
			backupFolder = GetBackupPath();
			RtlStringCchPrintfW(backupPath, MAX_PATH, L"%ws%08lX", backupFolder, cmd->Crc32);
			RtlInitUnicodeString(&usBackupPath, backupPath);

			status = RestoreFile(&usDestPath, &usBackupPath, TRUE);
			if (!NT_SUCCESS(status))
				RtlStringCchPrintfW(reply->Msg, maximumReplySize, L"%ws restore(overwrite) failed.\r\n", backupPath);
			else
				RtlStringCchPrintfW(reply->Msg, maximumReplySize, L"%ws restore(overwrite) success.\r\n", backupPath);
		}
	}
	else if (cmd->RtvCode == RTV_HEARTBEAT)
	{
		status = RTVCMD_SUCCESS;
		RtlStringCchPrintfW(reply->Msg, maximumReplySize, L"Heartbeat success.\r\n");
	}
	else if (cmd->RtvCode == RTVCMD_UPDATE_BACKUP_STORAGE)
	{
		status = UpdateBackupPath(cmd->Msg);
		if (status != STATUS_SUCCESS)
			RtlStringCchPrintfW(reply->Msg, maximumReplySize, L"Backup path change failed.\r\n");
		else
			RtlStringCchPrintfW(reply->Msg, maximumReplySize, L"Backup path changed to {%ws}.\r\n", cmd->Msg);
	}
	else if (cmd->RtvCode == RTVCMD_EXTENSION_ADD)
	{
		status = AddNewExtension(cmd->Msg, cmd->FileSize);
		if (status != STATUS_SUCCESS)
			RtlStringCchPrintfW(reply->Msg, maximumReplySize, L"Extension {%ws} add failed.\r\n", cmd->Msg);
		else
			RtlStringCchPrintfW(reply->Msg, maximumReplySize, L"Extension {%ws} add success.\r\n", cmd->Msg);
	}
	else if (cmd->RtvCode == RTVCMD_EXTENSION_REMOVE)
	{
		status = RemoveExtension(cmd->Msg);
		if (status != STATUS_SUCCESS)
			RtlStringCchPrintfW(reply->Msg, maximumReplySize, L"Extension {%ws} remove failed.\r\n", cmd->Msg);
		else
			RtlStringCchPrintfW(reply->Msg, maximumReplySize, L"Extension {%ws} remove success.\r\n", cmd->Msg);
	}
	else if (cmd->RtvCode == RTVCMD_EXCPATH_ADD)
	{
		status = AddNewExceptionPath(cmd->Msg);
		if (status != STATUS_SUCCESS)
			RtlStringCchPrintfW(reply->Msg, maximumReplySize, L"Exception path {%ws} add failed.\r\n", cmd->Msg);
		else
			RtlStringCchPrintfW(reply->Msg, maximumReplySize, L"Exception path {%ws} add success.\r\n", cmd->Msg);
	}
	else if (cmd->RtvCode == RTVCMD_EXCPATH_REMOVE)
	{
		status = RemoveExceptionPath(cmd->Msg);
		if (status != STATUS_SUCCESS)
			RtlStringCchPrintfW(reply->Msg, maximumReplySize, L"Exception path {%ws} remove failed.\r\n", cmd->Msg);
		else
			RtlStringCchPrintfW(reply->Msg, maximumReplySize, L"Exception path {%ws} remove success.\r\n", cmd->Msg);
	}
	else if (cmd->RtvCode == RTVCMD_INCPATH_ADD)
	{
		status = AddNewIncludePath(cmd->Msg, cmd->FileSize);
		if (status != STATUS_SUCCESS)
			RtlStringCchPrintfW(reply->Msg, maximumReplySize, L"Include path {%ws} add failed.\r\n", cmd->Msg);
		else
			RtlStringCchPrintfW(reply->Msg, maximumReplySize, L"Include path {%ws} add success.\r\n", cmd->Msg);
	}
	else if (cmd->RtvCode == RTVCMD_INCPATH_REMOVE)
	{
		status = RemoveIncludePath(cmd->Msg);
		if (status != STATUS_SUCCESS)
			RtlStringCchPrintfW(reply->Msg, maximumReplySize, L"Include path {%ws} remove failed.\r\n", cmd->Msg);
		else
			RtlStringCchPrintfW(reply->Msg, maximumReplySize, L"Include path {%ws} remove success.\r\n", cmd->Msg);
	}
	else if (cmd->RtvCode == RTVCMD_BACKUP_ON)
	{
		UpdateBackupEnabled(TRUE);
		status = RTVCMD_SUCCESS;
		RtlStringCchPrintfW(reply->Msg, maximumReplySize, L"Backup switch: ON\r\n");
	}
	else if (cmd->RtvCode == RTVCMD_BACKUP_OFF)
	{
		UpdateBackupEnabled(FALSE);
		status = RTVCMD_SUCCESS;
		RtlStringCchPrintfW(reply->Msg, maximumReplySize, L"Backup switch: OFF\r\n");
	}

	reply->RtvResult = status;
	// replyBufferNumberOfWrittenBytes는 문자열 길이가 아닌 Byte 길이를 전달해야함.
	RtlStringCbLengthW(reply->Msg, maximumReplySize, &replySize);
	*replyBufferNumberOfWrittenBytes = (ULONG)replySize;

	return STATUS_SUCCESS;
}

// 커널 드라이버가 백업 이벤트 발생 시 파일 정보를 유저모드 프로그램에 보내는 함수
NTSTATUS SendFileInformation(
	_In_	PWCHAR sendMsg,
	_In_	ULONG crc32,
	_In_	LONGLONG fileSize
)
{
	FLT_TO_USER sent;        RtlZeroMemory(&sent, sizeof(sent));
	FLT_TO_USER_REPLY reply; RtlZeroMemory(&reply, sizeof(reply));
	ULONG returned_bytes = 0;
	NTSTATUS status = STATUS_SUCCESS;

	sent.RtvCode = RTV_BACKUP_ALERT;
	RtlStringCchPrintfW(sent.Msg, ARRAYSIZE(sent.Msg), L"%ws", sendMsg);
	sent.Crc32 = crc32;
	sent.FileSize = fileSize;

	DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[FileRetvPort] Sending file information (Path: %ws, CRC32: %08lX)\r\n", sent.Msg, crc32);

	status = MinifltPortSendMessage(
		&sent,
		sizeof(sent),
		&reply,
		sizeof(reply),
		&returned_bytes
	);

	if (NT_SUCCESS(status) && returned_bytes > 0)
	{
		if (reply.RtvResult == 0)
		{
			DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[FileRetvPort] DB synchronize success (msg: %ws)\r\n", reply.Msg);
		}
		else
		{
			DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[FileRetvPort] Unknwon reply (msg: %ws)\r\n", reply.Msg);
		}
	}

	return status;
}

// 커널 드라이버 디버그 메세지를 유저모드에게 보내는 함수

NTSTATUS SendDebugMessage(
	_In_	PWCHAR sendMsg
)
{
	FLT_TO_USER sent;        RtlZeroMemory(&sent, sizeof(sent));
	FLT_TO_USER_REPLY reply; RtlZeroMemory(&reply, sizeof(reply));
	ULONG returned_bytes = 0;
	NTSTATUS status = STATUS_SUCCESS;

	sent.RtvCode = RTV_DBG_MESSAGE;
	RtlStringCchPrintfW(sent.Msg, ARRAYSIZE(sent.Msg), L"%ws", sendMsg);

	status = MinifltPortSendMessage(
		&sent,
		sizeof(sent),
		&reply,
		sizeof(reply),
		&returned_bytes
	);

	if (NT_SUCCESS(status) && returned_bytes > 0)
	{
		DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[FrtvKrnl] Sending debug message success. (Reply msg: %ws)\r\n", reply.Msg);
	}

	return status;
}