#include <fltKernel.h>
#include <ntstrsafe.h>

#include "ManageFile.h"
#include "Protocol.h"
#include "CRC32.h"
#include "SettingsExtension.h"
#include "SettingsPath.h"
#include "SettingsGeneral.h"
#include "Utility.h"

NTSTATUS GetFileSizeWithUnicodeString(
    _In_    PUNICODE_STRING srcPath,
    _Out_   PLONGLONG fileSize
)
{
    OBJECT_ATTRIBUTES sourceAttributes;
    HANDLE sourceHandle = NULL;
    IO_STATUS_BLOCK ioStatus;
    NTSTATUS status;

    // ������ ���� ���� ����
    InitializeObjectAttributes(&sourceAttributes, srcPath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    // �ٸ� ���μ������� ���� �ڵ��� ������� �� ACCESS_MASK�� SHARE_ACCESS�� ��� ������ �ο����� ������ 0xC0000043 (STATUS_SHARING_VIOLATION) ������ �߻��� �� ����
    status = ZwCreateFile(
        &sourceHandle, GENERIC_ALL, &sourceAttributes, &ioStatus, NULL, FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_VALID_FLAGS, FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0
    );
    
    if (!NT_SUCCESS(status))
    {
        DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[FileRtv] GetFileSizeWithUnicodeString() source file create failed: 0x%x\n", status);
        return status;
    }

    status = GetFileSizeWithHandle(sourceHandle, fileSize);
    ZwClose(sourceHandle);
    return status;
}

NTSTATUS GetFileSizeWithHandle(
    _In_    HANDLE sourceHandle,
    _Out_   PLONGLONG fileSize
)
{
    FILE_STANDARD_INFORMATION fileInfo = { 0 };
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatus;

    status = ZwQueryInformationFile(sourceHandle, &ioStatus, &fileInfo, sizeof(fileInfo), FileStandardInformation);
    if (!NT_SUCCESS(status))
        *fileSize = 0;
    else
        *fileSize = fileInfo.EndOfFile.QuadPart;

    return status;
}

NTSTATUS CopyFile(
    _In_    PUNICODE_STRING srcPath,
    _In_    PUNICODE_STRING dstPath
)
{
    OBJECT_ATTRIBUTES sourceAttributes, destinationAttributes;
    HANDLE sourceHandle = NULL, destinationHandle = NULL;
    IO_STATUS_BLOCK ioStatus;
    LONGLONG fileSize;

    // ������ ���� ���� ����
    InitializeObjectAttributes(&sourceAttributes, srcPath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    // �ٸ� ���μ������� ���� �ڵ��� ������� �� ACCESS_MASK�� SHARE_ACCESS�� ��� ������ �ο����� ������ 0xC0000043 (STATUS_SHARING_VIOLATION) ������ �߻��� �� ����
    NTSTATUS status = ZwCreateFile(
        &sourceHandle, GENERIC_ALL, &sourceAttributes, &ioStatus, NULL, FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_VALID_FLAGS, FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0
    );

    if (!NT_SUCCESS(status))
    {
        DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[FileRtv] CopyFile() source file create failed: 0x%x\n", status);
        return status;
    }

    // ���� ũ�� ���ϱ�
    GetFileSizeWithHandle(sourceHandle, &fileSize);

    // ����� ����� ���� ���� ����
    InitializeObjectAttributes(&destinationAttributes, dstPath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
    status = ZwCreateFile(
        &destinationHandle, GENERIC_WRITE, &destinationAttributes, &ioStatus, NULL, FILE_ATTRIBUTE_NORMAL,
        0, FILE_CREATE, FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0
    );

    if (!NT_SUCCESS(status)) {
        DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[FileRtv] CopyFile() destination file create failed: 0x%x\n", status);
        ZwClose(sourceHandle);
        return status;
    }

    char buffer[4096];
    ULONG bufferSize = sizeof(buffer);

    DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[FileRtv] create destination file\n");
    
    FILE_END_OF_FILE_INFORMATION eofInfo;
    eofInfo.EndOfFile.QuadPart = fileSize;
    status = ZwSetInformationFile(destinationHandle, &ioStatus, &eofInfo, sizeof(FILE_END_OF_FILE_INFORMATION), FileEndOfFileInformation);

    if (!NT_SUCCESS(status))
    {
        // TODO PC�뷮 ���� ���� �ڵ����� Ȯ���Ѵ�
        DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[FileRtv] Free space is not available: 0x%x\n", status);
        ZwClose(sourceHandle);
        ZwClose(destinationHandle);
        return status;
    }

    DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[FileRtv] File Size Change complete\n");

    // 4GB �̻� ���� �ν��� ���ؼ��� 64��Ʈ ������ ����ؾ��Ѵ�.
    for (LONGLONG i = 0; i < fileSize; i+= 4096)
    {
        // 4096����Ʈ ������ ���� ������ ���� ũ�Ⱑ 4096���� ���� ��� bufferSize�� �������Ѵ�.
        if (fileSize - i < 4096)
            bufferSize = (ULONG)(fileSize - i);

        status = ZwReadFile(sourceHandle, NULL, NULL, NULL, &ioStatus, buffer, bufferSize, NULL, NULL);
        if (!NT_SUCCESS(status))
        {
            DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[FileRtv] CopyFile() ZwReadFile() failed: 0x%x, %d\n", status, 5);
            ZwClose(sourceHandle);
            ZwClose(destinationHandle);
            return status;
        }
        status = ZwWriteFile(destinationHandle, NULL, NULL, NULL, &ioStatus, buffer, bufferSize, NULL, NULL);
        if (!NT_SUCCESS(status))
        {
            DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[FileRtv] CopyFile() ZwWriteFile() failed: 0x%x\n", status);
            ZwClose(sourceHandle);
            ZwClose(destinationHandle);
            return status;
        }
    }

    ZwClose(sourceHandle);
    ZwClose(destinationHandle);

    return status;
}

NTSTATUS DeleteFile(
    _In_    PUNICODE_STRING deletePath
)
{
    // ���� ����
    OBJECT_ATTRIBUTES sourceAttributes;
    // ������ ���� ���� ����
    InitializeObjectAttributes(&sourceAttributes, deletePath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
    NTSTATUS status = ZwDeleteFile(&sourceAttributes);

    return status;
}

// TODO ���ڿ� ó���� �Լ����� ���� �ʵ��� �����Ѵ�.
NTSTATUS RestoreFile(
    _In_    PUNICODE_STRING restorePath,
    _In_    PUNICODE_STRING backupPath
)
{
    NTSTATUS status = CopyFile(backupPath, restorePath);
    if (!NT_SUCCESS(status))
        return status;

    status = DeleteFile(backupPath);
    if (!NT_SUCCESS(status))
        return status;

    return status;
}

BOOLEAN BackupIfTarget(
    _In_    PFLT_FILE_NAME_INFORMATION fni
)
{
    NTSTATUS status = STATUS_SUCCESS;
    DECLARE_UNICODE_STRING_SIZE(destPath, 2000);
    LPWSTR backupFolder = NULL;
    ULONG crc = 0;

    // ��� ���� �Ǵ��� ���� �̸� ���� ũ�⸦ ���Ѵ�
    LONGLONG fileSize = 0;
    status = GetFileSizeWithUnicodeString(&fni->Name, &fileSize);

    // ũ�Ⱑ 0����Ʈ�� ������ ������� �ʴ´�.
    if (!NT_SUCCESS(status) || fileSize == 0)
    {
        DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[FileRtv] Failed to get file size. 0x%x\r\n", status);
        return FALSE;
    }

    if (IsBackupEnabled() == FALSE)
    {
        return FALSE;
    }
    
    // ��� ���� �ʴ� ���� ������ �����ϴ� ������ ��� ������� �ʴ´�
    if (FindExceptionPath(fni->Name.Buffer) != NULL)
    {
        DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[FileRtv] This is exists on exception path.\r\n");
        return FALSE;
    }
    
    // ��� ��� Ȯ���ڰ� �ƴ� ��� ������� �ʴ´�
    if (FindExtension(fni->Extension.Buffer, fileSize))
    {
        // ����� ���� CRC32�� ����
        status = GetFileCRC(&fni->Name, &crc);
        if (!NT_SUCCESS(status))
        {
            DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[FileRtv] RtlUnicodeStringPrintf() failed. 0x%x\r\n", status);
            return FALSE;
        }

        // ���������� ����� ��� ���ϱ�
        backupFolder = GetBackupPath();
        status = RtlUnicodeStringPrintf(&destPath, L"%ws%08lX", backupFolder, crc);
        DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[FileRtv] CRC CALC COMPLETE. (CRC32: %d)\r\n", crc);
        if (!NT_SUCCESS(status))
        {
            DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[FileRtv] RtlUnicodeStringPrintf() failed. 0x%x\r\n", status);
            return FALSE;
        }

        // ��� ���� �� ������Ʈ�� �޼��� �߼�
        status = CopyFile(&fni->Name, &destPath);
        if (NT_SUCCESS(status))
        {
            status = SendFileInformation(fni->Name.Buffer, crc, fileSize);
            if (!NT_SUCCESS(status))
            {
                DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[FileRtv] send msg failed.\r\n");
            }
            DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[FileRtv] Backup success. %wZ (CRC32: %d)\r\n", destPath, crc);
        }
        else
        {
            DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[FileRtv] Backup failed. %wZ\r\n", destPath);
            return FALSE;
        }
    }

    return TRUE;
}

/*
���� ��ȯ�ϴ� ��θ� UNICODE_STRING���� ó������ �ʴ� ����:
 RtlInitUnicodeString�� ������ ���ڿ��� �������� ����.
 ���� �Լ� ���ο��� �ش� �Լ��� ȣ���Ѵٸ� ���������� ������� ������ ������ ���ڰ� ������������.
 �׷��� PWSTR ���·� ���� �� �ۿ��� UNICODE_STRING���� ��ȯ�� ����ؾ��Ѵ�.

NTSTATUS GetBackupPath(
    _In_    PUNICODE_STRING targetFile, 
    _In_    PCWSTR backupFolder,
    _Out_   PWSTR backupFullPath,
    _In_    SIZE_T backupFullPathSize
)
{
    size_t targetLength;
    NTSTATUS status = RtlStringCchLengthW(targetFile->Buffer, MAX_PATH, &targetLength);
    if (!NT_SUCCESS(status))
    {
        DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "Failed to get targetFile string length\r\n");
        return status;
    }

    ULONG crc32;
    GetFileCRC(targetFile, &crc32);

    wchar_t fileName[MAX_PATH];
    RtlSecureZeroMemory(fileName, sizeof(fileName));
    RtlSecureZeroMemory(backupFullPath, backupFullPathSize);

    //size_t i;
    //for (i = targetLength - 1; targetFile->Buffer[i] != '.' && i > 0; i--);
    //if (i == 0)
        //return -1;
    // ������ ������ \���� �ε����� ���ϰ� ���� ������ ����� -1 ��ȯ

    status = RtlStringCchPrintfW(backupFullPath, backupFullPathSize, L"%ws\\%08lX", backupFolder, crc32);
    // ������ ���� �ε��� �ּҸ� ���ڷ� �־� ���� ��θ� �����Ѵ�.

    return status;
}*/