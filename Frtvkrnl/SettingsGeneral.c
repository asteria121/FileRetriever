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
	_In_	LPCWSTR path
)
{
	NTSTATUS status = STATUS_SUCCESS;
	status = RtlStringCchCopyW(backupPath, 2000, path);
	DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "Backup path set to: %ws\r\n", backupPath);
	return status;
}

BOOLEAN IsBackupEnabled()
{
	return isBackupEnabled;
}

// TODO: C:\, C:\Windows 등 시스템 폴더는 지정하지 못하도록 설정한다.
VOID UpdateBackupEnabled(
	_In_	BOOLEAN enabled
)
{
	isBackupEnabled = enabled;
	DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "Backup settings set to: %d\r\n", isBackupEnabled);
}