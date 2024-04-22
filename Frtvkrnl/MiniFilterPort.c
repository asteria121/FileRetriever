#include <fltKernel.h>
#include <ntstrsafe.h>

#include "MiniFilterPort.h"
#include "Protocol.h"
#include "Utility.h"

static PFLT_FILTER fltHandle;
static PFLT_PORT fltPort;
static PFLT_PORT clientPort;

// 유저모드 프로세스가 미니필터 포트에 연결할 경우 호출되는 콜백 함수
NTSTATUS MinifltPortNotifyRoutine(
	_In_	PFLT_PORT connectedClientPort,
	_In_	PVOID serverCookie,
	_In_	PVOID connectionCtx,
	_In_	ULONG connectionCtxSize,
	_Out_	PVOID* connectionPortCookie
)
{
	UNREFERENCED_PARAMETER(serverCookie);
	UNREFERENCED_PARAMETER(connectionCtx);
	UNREFERENCED_PARAMETER(connectionCtxSize);
	connectionPortCookie = NULL;
	
	DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[FileRetvPort] " __FUNCTION__ " User-mode application(%u) connect to this filter\n", PtrToUint(PsGetCurrentProcessId()));
	clientPort = connectedClientPort;
	return STATUS_SUCCESS;
}

// 미니필터 드라이버가 언로드 되거나 유저모드 프로세스가 연결을 해제할 경우 호출되는 콜백 함수
VOID MinifltPortDisconnectRoutine(
	_In_	PVOID connectionCookie
)
{
	UNREFERENCED_PARAMETER(connectionCookie);

	DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[FileRetvPort] " __FUNCTION__ " User-mode application(%u) disconnect with this filter\n", PtrToUint(PsGetCurrentProcessId()));
}

// 유저모드에서 FltSendMessage()를 호출할 때 호출되는 콜백 함수
NTSTATUS MinifltPortMessageRoutine(
	_In_		PVOID portCookie,
	_In_opt_	PVOID inputBuffer,
	_In_		ULONG inputBufferSize,
	_Out_opt_	PVOID replyBuffer,
	_In_		ULONG replyBufferSize,
	_Out_		PULONG replyBufferNumberOfWrittenBytes
)
{
	UNREFERENCED_PARAMETER(portCookie);

	DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[FileRetvPort] Usermode application (%u) send data. size: %lu\r\n", PtrToUint(PsGetCurrentProcessId()), inputBufferSize);

	if (inputBuffer && inputBufferSize == sizeof(USER_TO_FLT))
	{
		PUSER_TO_FLT sent = (PUSER_TO_FLT)inputBuffer;
		DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[FileRetvPort] Usermode application (%ws) send data. (TYPE=%d)\r\n", sent->Msg, sent->RtvCode);
		
		if (replyBuffer && replyBufferSize == sizeof(USER_TO_FLT_REPLY))
		{
			ParseUsermodeCommand(sent, replyBuffer, replyBufferSize, replyBufferNumberOfWrittenBytes);
		}
		else
		{
			DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[FileRetvPort] Reply buffer is empty.\r\n");
		}
	}

	return STATUS_SUCCESS;
}

NTSTATUS MinifltPortSendMessage(
	_In_		PVOID sendData,
	_In_		ULONG sendDataSize,
	_Out_opt_	PVOID recvBuffer,
	_In_		ULONG recvBufferSize,
	_Out_		PULONG recvBufferNumberOfWrittenBytes
)
{
	NTSTATUS status = STATUS_SUCCESS;

	if (recvBuffer)	// 양방향 메세지 송신
	{
		LARGE_INTEGER timeout;
		timeout.QuadPart = 100000000;

		status = FltSendMessage(
			fltHandle,
			&clientPort,
			sendData,
			sendDataSize,
			recvBuffer,
			&recvBufferSize,
			NULL
		);
		if (!NT_SUCCESS(status))
		{
			DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[FileRetvPort] FilterSendMessage (Recv) Failed (TYPE=%x)\r\n", status);
		}
		else if (status == STATUS_TIMEOUT)
		{
			DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[FileRetvPort] FilterSendMessage (Recv) Failed (TIME OUT)\r\n");
		}
		
		*recvBufferNumberOfWrittenBytes = recvBufferSize;
	}
	else
	{
		LARGE_INTEGER timeout;
		timeout.QuadPart = 0;

		status = FltSendMessage(
			fltHandle,
			&clientPort,
			sendData,
			sendDataSize,
			NULL,
			NULL,
			&timeout
		);
		if (!NT_SUCCESS(status))
		{
			DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[FileRetvPort] FilterSendMessage (Send Only) Failed (TYPE=%x)\r\n", status);
		}

		*recvBufferNumberOfWrittenBytes = 0;
	}

	return status;
}

NTSTATUS MinifltPortInitialize(
	_In_	PFLT_FILTER minifltHandle
)
{
	NTSTATUS status = STATUS_SUCCESS;
	UNICODE_STRING port_name; RtlZeroMemory(&port_name, sizeof(port_name));
	PSECURITY_DESCRIPTOR sd;  RtlZeroMemory(&sd, sizeof(sd));
	OBJECT_ATTRIBUTES oa;     RtlZeroMemory(&oa, sizeof(oa));

	status = FltBuildDefaultSecurityDescriptor(&sd, FLT_PORT_ALL_ACCESS);
	if (!NT_SUCCESS(status))
	{
		DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[FileRetvPort] FltBuildDefaultSecurityDescriptor() Failed (TYPE=%x)\r\n", status);
		MinifltPortFinalize();
		return status;
	}

	RtlInitUnicodeString(&port_name, L"\\FrtvPort");
	InitializeObjectAttributes(
		&oa, &port_name, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, sd
	);

	status = FltCreateCommunicationPort(
		minifltHandle,
		&fltPort,
		&oa,
		NULL,
		MinifltPortNotifyRoutine,
		MinifltPortDisconnectRoutine,
		MinifltPortMessageRoutine,
		1
	);

	if (!NT_SUCCESS(status))
	{
		DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[FileRetvPort] FltCreateCommunicationPort() Failed (TYPE=%x)\r\n", status);
		MinifltPortFinalize();
		return status;
	}

	fltHandle = minifltHandle;
	return STATUS_SUCCESS;
}

VOID MinifltPortFinalize()
{
	if (fltPort)
	{
		FltCloseCommunicationPort(fltPort);
	}
}