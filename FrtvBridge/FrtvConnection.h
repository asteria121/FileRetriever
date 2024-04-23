#ifndef _FRTV_CONNECTION_H_
#define _FRTV_CONNECTION_H_
#include "IOCP.h"

typedef struct SEND_HB_PARAMS
{
	PWORKER_IOCP_PARAMS Context;
	DWORD ThreadCount;
} SEND_HB_PARAMS, *PSEND_HB_PARAMS;

DWORD ConnectMinifltPort(PWORKER_IOCP_PARAMS Context, PHANDLE threads, DWORD threadCount);
VOID SendMinifltHeartbeat(PSEND_HB_PARAMS params);

#endif