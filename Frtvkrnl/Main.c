#include <fltKernel.h>
#include <ntstrsafe.h>
#include "MiniFilter.h"
#include "MiniFilterPort.h"
#include "SettingsPath.h"
#include "SettingsExtension.h"
#include "Utility.h"

// Driver Unload Routine
VOID UnloadDriver(
    _In_ PDRIVER_OBJECT driverObject
)
{
    UNREFERENCED_PARAMETER(driverObject);
    DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[FileRtv] Unload driver\r\n");

    MinifltPortFinalize();
    StopMiniFilter();
}

NTSTATUS DriverEntry(
    IN PDRIVER_OBJECT driverObject,
    IN PUNICODE_STRING registryPath)
{
    UNREFERENCED_PARAMETER(driverObject);
    UNREFERENCED_PARAMETER(registryPath);

    NTSTATUS status = STATUS_SUCCESS;
    DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[FileRtv] Minifilter Loading..\r\n");

    InitializeExtensionList();
    InitializePathList();

    // 미니필터 필터링 시작
    PFLT_FILTER fltHandle = NULL;
    status = StartMiniFilter(driverObject, &fltHandle);
    if (!NT_SUCCESS(status))
    {
        DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[FileRtv] MiniFilter start failed (status: 0x%x)", status);
        StopMiniFilter();
        return status;
    }
    else if (fltHandle == NULL)
    {
        DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[FileRtv] Failed to initialize mini filter handle (status: 0x%x)", status);
        StopMiniFilter();
        return status;
    }

    // 미니필터 포트 (통신) 개방
    status = MinifltPortInitialize(fltHandle);
    if (!NT_SUCCESS(status))
    {
        DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[FileRtv] MiniFilterPort initialize failed (status: 0x%x)", status);
        MinifltPortFinalize();
        StopMiniFilter();
    }

    driverObject->DriverUnload = UnloadDriver;

    return STATUS_SUCCESS;
}

