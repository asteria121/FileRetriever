#ifndef _IOCP_H_
#define _IOCP_H_

#define IOCP_QUIT_KEY 500000

typedef struct WORKER_IOCP_PARAMS
{
	HANDLE Port;
	HANDLE Completion;
} WORKER_IOCP_PARAMS, * PWORKER_IOCP_PARAMS;

DWORD CreateIOCPThreads(PWORKER_IOCP_PARAMS Context, PHANDLE iocpThreads, DWORD threadCount);
DWORD FrtvWorker(PWORKER_IOCP_PARAMS Context);
#endif