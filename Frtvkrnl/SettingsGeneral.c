#include <fltKernel.h>
#include <ntstrsafe.h>
#include "SettingsGeneral.h"
#include "Utility.h"

static BOOLEAN isBackupEnabled = TRUE;
static WCHAR backupPath[2000];

LPWSTR GetBackupPath()
{
	return backupPath;
}

NTSTATUS UpdateBackupPath(
	_In_	LPCWSTR drive
)
{
	NTSTATUS status = STATUS_SUCCESS;
    OBJECT_ATTRIBUTES directoryAttributes;
    HANDLE directoryHandle = NULL;
    IO_STATUS_BLOCK ioStatus;

    UNICODE_STRING usBackupPath;
    RtlInitUnicodeString(&usBackupPath, backupPath);
    usBackupPath.MaximumLength = 2000;
    RtlUnicodeStringPrintf(&usBackupPath, L"%wsFrtvBackup\\", drive);

    // 해당 폴더가 존재하거나 만들 수 있는지 확인한다.
    InitializeObjectAttributes(&directoryAttributes, &usBackupPath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    status = ZwCreateFile(
        &directoryHandle, GENERIC_WRITE, &directoryAttributes, &ioStatus, NULL, FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_OPEN_IF, FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0
    );

    if (!NT_SUCCESS(status))
    {
        DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "UpdateBackupPath() failed. Failed to open backup path. NTSTATUS: 0x%x\r\n", status);
        return status;
    }

    ZwClose(directoryHandle);


	DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "Backup path set to: %ws\r\n", backupPath);
	return status;
}

BOOLEAN IsBackupEnabled()
{
	return isBackupEnabled;
}

VOID UpdateBackupEnabled(
	_In_	BOOLEAN enabled
)
{
	isBackupEnabled = enabled;
	DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "Backup settings set to: %d\r\n", isBackupEnabled);
}