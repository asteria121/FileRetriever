#include <fltKernel.h>
#include <ntstrsafe.h>

#include "ManageFile.h"
#include "Protocol.h"
#include "CRC32.h"
#include "SettingsExtension.h"
#include "SettingsIncludePath.h"
#include "SettingsPath.h"
#include "SettingsGeneral.h"
#include "Utility.h"

// maximumFileSize�� 0���� ������ ��� ���� ũ�⿡ ������� ���縦 �����Ѵ�.
// maximumFileSize�� 0�� �ƴ� ��� �ش� ũ�⺸�� srcPath�� ������ ū ��� �������� �ʴ´�.
// ���� ũ�⸦ ���ϴ� ������ ������尡 ����(���� �ڵ� ���� �ݱ�) �� �Լ��� �����Ѵ�.
NTSTATUS CopyFile(
    _In_        PUNICODE_STRING srcPath,
    _In_        PUNICODE_STRING dstPath,
    _In_        BOOLEAN overwriteDst,
    _In_        LONGLONG maximumFileSize,
    _Out_opt_   PLONGLONG fileSize
)
{
    OBJECT_ATTRIBUTES sourceAttributes, destinationAttributes;
    HANDLE sourceHandle = NULL, destinationHandle = NULL;
    IO_STATUS_BLOCK ioStatus;
    FILE_STANDARD_INFORMATION srcFileInfo = { 0 };
    ULONG createDisposition;

    // ������ ����� ���� ����
    if (overwriteDst == TRUE)
        createDisposition = FILE_OVERWRITE_IF;
    else
        createDisposition = FILE_CREATE;

    // ������ ���� ���� ����
    InitializeObjectAttributes(&sourceAttributes, srcPath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    // �ٸ� ���μ������� ���� �ڵ��� ������� �� ACCESS_MASK�� SHARE_ACCESS�� ��� ������ �ο����� ������ 0xC0000043 (STATUS_SHARING_VIOLATION) ������ �߻��� �� ����
    NTSTATUS status = ZwCreateFile(
        &sourceHandle, GENERIC_ALL, &sourceAttributes, &ioStatus, NULL, FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_VALID_FLAGS, FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0
    );

    if (!NT_SUCCESS(status))
    {
        DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "CopyFile() failed. Failed to open source file. NTSTATUS: 0x%x\n", status);
        return status;
    }

    // ���� ũ�� ���ϱ�
    status = ZwQueryInformationFile(sourceHandle, &ioStatus, &srcFileInfo, sizeof(srcFileInfo), FileStandardInformation);
    if (!NT_SUCCESS(status))
    {
        return status;
    }
    else
    {
        // ���� ũ�⸦ �����ͷ� ����
        if (fileSize != NULL)
            *fileSize = srcFileInfo.EndOfFile.QuadPart;

        if (srcFileInfo.EndOfFile.QuadPart > maximumFileSize && maximumFileSize != 0)
        {
            return STATUS_UNSUCCESSFUL;
        }
    }

    // ����� ����� ���� ���� ����
    InitializeObjectAttributes(&destinationAttributes, dstPath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    status = ZwCreateFile(
        &destinationHandle, GENERIC_WRITE, &destinationAttributes, &ioStatus, NULL, FILE_ATTRIBUTE_NORMAL,
        0, createDisposition, FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0
    );

    if (!NT_SUCCESS(status)) {
        DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "CopyFile() failed. Failed to create destination file. NTSTATUS: 0x%x\n", status);
        ZwClose(sourceHandle);
        return status;
    }

    char buffer[4096];
    ULONG bufferSize = sizeof(buffer);
    
    FILE_END_OF_FILE_INFORMATION dstEofInfo;
    dstEofInfo.EndOfFile.QuadPart = srcFileInfo.EndOfFile.QuadPart;
    status = ZwSetInformationFile(destinationHandle, &ioStatus, &dstEofInfo, sizeof(FILE_END_OF_FILE_INFORMATION), FileEndOfFileInformation);

    if (!NT_SUCCESS(status))
    {
        // TODO PC�뷮 ���� ���� �ڵ����� Ȯ���Ѵ�
        DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "CopyFile() failed. Free space is not available. NTSTATUS: 0x%x\n", status);
        ZwClose(sourceHandle);
        ZwClose(destinationHandle);
        return status;
    }

    // 4GB �̻� ���� �ν��� ���ؼ��� 64��Ʈ ������ ����ؾ��Ѵ�.
    for (LONGLONG i = 0; i < dstEofInfo.EndOfFile.QuadPart; i+= 4096)
    {
        // 4096����Ʈ ������ ���� ������ ���� ũ�Ⱑ 4096���� ���� ��� bufferSize�� �������Ѵ�.
        if (dstEofInfo.EndOfFile.QuadPart - i < 4096)
            bufferSize = (ULONG)(dstEofInfo.EndOfFile.QuadPart - i);

        status = ZwReadFile(sourceHandle, NULL, NULL, NULL, &ioStatus, buffer, bufferSize, NULL, NULL);
        if (!NT_SUCCESS(status))
        {
            DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "CopyFile() failed. Failed to read source file. NTSTATUS: 0x%x, %d\n", status, 5);
            ZwClose(sourceHandle);
            ZwClose(destinationHandle);
            return status;
        }
        status = ZwWriteFile(destinationHandle, NULL, NULL, NULL, &ioStatus, buffer, bufferSize, NULL, NULL);
        if (!NT_SUCCESS(status))
        {
            DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "CopyFile() failed. Failed to write destination file. NTSTATUS: 0x%x\n", status);
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
    OBJECT_ATTRIBUTES sourceAttributes;
    InitializeObjectAttributes(&sourceAttributes, deletePath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
    NTSTATUS status = ZwDeleteFile(&sourceAttributes);

    return status;
}

NTSTATUS RestoreFile(
    _In_    PUNICODE_STRING restorePath,
    _In_    PUNICODE_STRING backupPath,
    _In_    BOOLEAN overwriteDst
)
{
    NTSTATUS status = CopyFile(backupPath, restorePath, overwriteDst, 0, NULL);
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
    LPWSTR backupFolder = NULL;
    ULONG crc = 0;
    LONGLONG fileSize = 0;
    PEXTENSIONLIST pExt = NULL;
    PINCPATHLIST pInc = NULL;
    DECLARE_UNICODE_STRING_SIZE(destPath, 2000);

    if (IsBackupEnabled() == FALSE)
    {
        return FALSE;
    }
    
    // ��� ���� �ʴ� ���� ������ �����ϴ� ������ ��� ������� �ʴ´�
    if (FindExceptionPath(fni->Name.Buffer, TRUE) != NULL)
    {
        DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[FileRtv] This is exists on exception path.\r\n");
        return FALSE;
    }

    // ���� Ȯ���� ���ϱ�
    FltParseFileNameInformation(fni);
    
    // ��� ��� Ȯ������ ��� �Ǵ� ��� ��� ������ ��쿡 ����� �����Ѵ�.
    // ��� ��� ������ ���� ������ �켱 ����ȴ�. (1. ��������, 2. Ȯ����, 3. ��� ���� ��)
    pExt = FindExtension(fni->Extension.Buffer);
    pInc = FindIncludePath(fni->Name.Buffer, TRUE);
    if (pExt != NULL || pInc != NULL)
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
        // ���� ũ�� ������ CopyFile() ���ο��� �����Ѵ�.
        // Ȯ���� �� ������ �켱 ����ȴ�. (1. ��������, 2. Ȯ����, 3. ��� ���� ��)
        
        if (pInc != NULL)
        {
            DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[FileRtv] Inlcude path maximum size: %lld\r\n", pInc->MaximumSize);
            status = CopyFile(&fni->Name, &destPath, FALSE, pInc->MaximumSize, &fileSize);
        }
        else
        {
            DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[FileRtv] Extension maximum size: %lld\r\n", pExt->MaximumSize);
            status = CopyFile(&fni->Name, &destPath, FALSE, pExt->MaximumSize, &fileSize);
        }
        
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