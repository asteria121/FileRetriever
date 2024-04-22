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

    // 복사할 파일 정보 설정
    InitializeObjectAttributes(&sourceAttributes, srcPath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    // 다른 프로세스에서 파일 핸들을 사용중일 때 ACCESS_MASK와 SHARE_ACCESS에 모든 권한을 부여하지 않으면 0xC0000043 (STATUS_SHARING_VIOLATION) 오류가 발생할 수 있음
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

    // 복사할 파일 정보 설정
    InitializeObjectAttributes(&sourceAttributes, srcPath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    // 다른 프로세스에서 파일 핸들을 사용중일 때 ACCESS_MASK와 SHARE_ACCESS에 모든 권한을 부여하지 않으면 0xC0000043 (STATUS_SHARING_VIOLATION) 오류가 발생할 수 있음
    NTSTATUS status = ZwCreateFile(
        &sourceHandle, GENERIC_ALL, &sourceAttributes, &ioStatus, NULL, FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_VALID_FLAGS, FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0
    );

    if (!NT_SUCCESS(status))
    {
        DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[FileRtv] CopyFile() source file create failed: 0x%x\n", status);
        return status;
    }

    // 파일 크기 구하기
    GetFileSizeWithHandle(sourceHandle, &fileSize);

    // 복사될 경로의 파일 정보 설정
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
        // TODO PC용량 부족 무슨 코드인지 확인한다
        DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[FileRtv] Free space is not available: 0x%x\n", status);
        ZwClose(sourceHandle);
        ZwClose(destinationHandle);
        return status;
    }

    DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[FileRtv] File Size Change complete\n");

    // 4GB 이상 파일 인식을 위해서는 64비트 정수를 사용해야한다.
    for (LONGLONG i = 0; i < fileSize; i+= 4096)
    {
        // 4096바이트 단위로 쓰기 때문에 남은 크기가 4096보다 작은 경우 bufferSize를 재조정한다.
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
    // 파일 삭제
    OBJECT_ATTRIBUTES sourceAttributes;
    // 복사할 파일 정보 설정
    InitializeObjectAttributes(&sourceAttributes, deletePath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
    NTSTATUS status = ZwDeleteFile(&sourceAttributes);

    return status;
}

// TODO 문자열 처리는 함수에서 하지 않도록 변경한다.
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

    // 백업 여부 판단을 위해 미리 파일 크기를 구한다
    LONGLONG fileSize = 0;
    status = GetFileSizeWithUnicodeString(&fni->Name, &fileSize);

    // 크기가 0바이트인 파일은 백업하지 않는다.
    if (!NT_SUCCESS(status) || fileSize == 0)
    {
        DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[FileRtv] Failed to get file size. 0x%x\r\n", status);
        return FALSE;
    }

    if (IsBackupEnabled() == FALSE)
    {
        return FALSE;
    }
    
    // 백업 하지 않는 폴더 하위에 존재하는 파일일 경우 백업하지 않는다
    if (FindExceptionPath(fni->Name.Buffer) != NULL)
    {
        DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[FileRtv] This is exists on exception path.\r\n");
        return FALSE;
    }
    
    // 백업 대상 확장자가 아닌 경우 백업하지 않는다
    if (FindExtension(fni->Extension.Buffer, fileSize))
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
실제 반환하는 경로를 UNICODE_STRING으로 처리하지 않는 이유:
 RtlInitUnicodeString은 실제로 문자열을 복사하지 않음.
 따라서 함수 내부에서 해당 함수를 호출한다면 지역변수가 사라지기 때문에 참조할 문자가 없어져버린다.
 그래서 PWSTR 형태로 꺼낸 후 밖에서 UNICODE_STRING으로 변환해 사용해야한다.

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
    // 파일의 마지막 \까지 인덱스를 구하고 만약 문제가 생기면 -1 반환

    status = RtlStringCchPrintfW(backupFullPath, backupFullPathSize, L"%ws\\%08lX", backupFolder, crc32);
    // 위에서 구한 인덱스 주소를 인자로 넣어 최종 경로를 조합한다.

    return status;
}*/