#ifndef _MANAGE_FILE_H_
#define _MANAGE_FILE_H_

#define MAX_PATH 260

NTSTATUS CopyFile(
    _In_    PUNICODE_STRING srcPath,
    _In_    PUNICODE_STRING dstPath,
    _In_    LONGLONG maximumFileSize
);

NTSTATUS DeleteFile(
    _In_    PUNICODE_STRING deletePath
);

NTSTATUS RestoreFile(
    _In_    PUNICODE_STRING restorePath,
    _In_    PUNICODE_STRING backupPath
);

BOOLEAN BackupIfTarget(
    _In_    PFLT_FILE_NAME_INFORMATION fni
);

#endif