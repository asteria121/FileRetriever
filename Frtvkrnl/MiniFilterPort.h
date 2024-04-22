#ifndef _MINIFLT_PORT_H_
#define _MINIFLT_PORT_H_

NTSTATUS MinifltPortNotifyRoutine(
	_In_	PFLT_PORT connectedClientPort,
	_In_	PVOID serverCookie,
	_In_	PVOID connectionCtx,
	_In_	ULONG connectionCtxSize,
	_Out_	PVOID* connectionPortCookie
);

VOID MinifltPortDisconnectRoutine(
	_In_	PVOID connectionCookie
);

NTSTATUS MinifltPortMessageRoutine(
	_In_		PVOID portCookie,
	_In_opt_	PVOID inputBuffer,
	_In_		ULONG inputBufferSize,
	_Out_opt_	PVOID replyBuffer,
	_In_		ULONG replyBufferSize,
	_Out_		PULONG replyBufferNumberOfWrittenBytes
);

NTSTATUS MinifltPortSendMessage(
	_In_		PVOID sendData,
	_In_		ULONG sendDataSize,
	_Out_opt_	PVOID recvBuffer,
	_In_		ULONG recvBufferSize,
	_Out_		PULONG recvBufferNumberOfWrittenBytes
);

NTSTATUS MinifltPortInitialize(
	_In_	PFLT_FILTER minifltHandle
);

VOID MinifltPortFinalize();

#endif