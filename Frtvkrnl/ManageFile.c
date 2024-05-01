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

// maximumFileSize를 0으로 지정할 경우 파일 크기에 상관없이 복사를 진행한다.
// maximumFileSize가 0이 아닐 경우 해당 크기보다 srcPath의 파일이 큰 경우 복사하지 않는다.
// 파일 크기를 구하는 과정이 오버헤드가 심해(파일 핸들 열기 닫기) 이 함수에 통합한다.
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

    // 파일을 덮어쓸지 말지 결정
    if (overwriteDst == TRUE)
        createDisposition = FILE_OVERWRITE_IF;
    else
        createDisposition = FILE_CREATE;

    // 복사할 파일 정보 설정
    InitializeObjectAttributes(&sourceAttributes, srcPath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    // 다른 프로세스에서 파일 핸들을 사용중일 때 ACCESS_MASK와 SHARE_ACCESS에 모든 권한을 부여하지 않으면 0xC0000043 (STATUS_SHARING_VIOLATION) 오류가 발생할 수 있음
    NTSTATUS status = ZwCreateFile(
        &sourceHandle, GENERIC_ALL, &sourceAttributes, &ioStatus, NULL, FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_VALID_FLAGS, FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0
    );

    if (!NT_SUCCESS(status))
    {
        DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "CopyFile() failed. Failed to open source file. NTSTATUS: 0x%x\n", status);
        return status;
    }

    // 파일 크기 구하기
    status = ZwQueryInformationFile(sourceHandle, &ioStatus, &srcFileInfo, sizeof(srcFileInfo), FileStandardInformation);
    if (!NT_SUCCESS(status))
    {
        return status;
    }
    else
    {
        // 파일 크기를 포인터로 전달
        if (fileSize != NULL)
            *fileSize = srcFileInfo.EndOfFile.QuadPart;

        if (srcFileInfo.EndOfFile.QuadPart > maximumFileSize && maximumFileSize != 0)
        {
            return STATUS_UNSUCCESSFUL;
        }
    }

    // 복사될 경로의 파일 정보 설정
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
        // TODO PC용량 부족 무슨 코드인지 확인한다
        DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "CopyFile() failed. Free space is not available. NTSTATUS: 0x%x\n", status);
        ZwClose(sourceHandle);
        ZwClose(destinationHandle);
        return status;
    }

    // 4GB 이상 파일 인식을 위해서는 64비트 정수를 사용해야한다.
    for (LONGLONG i = 0; i < dstEofInfo.EndOfFile.QuadPart; i+= 4096)
    {
        // 4096바이트 단위로 쓰기 때문에 남은 크기가 4096보다 작은 경우 bufferSize를 재조정한다.
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
    
    // 백업 하지 않는 폴더 하위에 존재하는 파일일 경우 백업하지 않는다
    if (FindExceptionPath(fni->Name.Buffer, TRUE) != NULL)
    {
        DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[FileRtv] This is exists on exception path.\r\n");
        return FALSE;
    }

    // 파일 확장자 구하기
    FltParseFileNameInformation(fni);
    
    // 백업 대상 확장자인 경우 또는 백업 대상 폴더인 경우에 백업을 진행한다.
    // 백업 대상 폴더는 예외 폴더가 우선 적용된다. (1. 예외폴더, 2. 확장자, 3. 백업 폴더 순)
    pExt = FindExtension(fni->Extension.Buffer);
    pInc = FindIncludePath(fni->Name.Buffer, TRUE);
    if (pExt != NULL || pInc != NULL)
    {
        // 백업을 위해 CRC32를 구함
        status = GetFileCRC(&fni->Name, &crc);
        if (!NT_SUCCESS(status))
        {
            DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[FileRtv] RtlUnicodeStringPrintf() failed. 0x%x\r\n", status);
            return FALSE;
        }

        // 최종적으로 백업할 경로 구하기
        backupFolder = GetBackupPath();
        status = RtlUnicodeStringPrintf(&destPath, L"%ws%08lX", backupFolder, crc);
        DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[FileRtv] CRC CALC COMPLETE. (CRC32: %d)\r\n", crc);
        if (!NT_SUCCESS(status))
        {
            DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[FileRtv] RtlUnicodeStringPrintf() failed. 0x%x\r\n", status);
            return FALSE;
        }

        // 백업 진행 후 필터포트로 메세지 발송
        // 파일 크기 구분은 CopyFile() 내부에서 진행한다.
        // 확장자 별 설정이 우선 적용된다. (1. 예외폴더, 2. 확장자, 3. 백업 폴더 순)
        
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