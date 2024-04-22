#ifndef _MINIFLT_H_
#define _MINIFLT_H_

#define MAX_PATH 260

NTSTATUS StartMiniFilter(
    _In_ PDRIVER_OBJECT driverObject,
    _Out_ PFLT_FILTER* filterHandle
);

VOID StopMiniFilter();

#endif