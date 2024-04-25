#ifndef _SETTINGS_GENERAL_H_
#define _SETTINGS_GENERAL_H_

LPWSTR GetBackupPath();

NTSTATUS UpdateBackupPath(
	_In_	LPCWSTR drive
);

BOOLEAN IsBackupEnabled();

VOID UpdateBackupEnabled(
	_In_	BOOLEAN enabled
);

#endif