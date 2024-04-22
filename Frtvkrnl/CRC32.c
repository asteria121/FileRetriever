#include <fltKernel.h>

#include "CRC32.h"
#include "Utility.h"

NTSTATUS GetFileCRC(
    _In_    PUNICODE_STRING srcPath,
    _Out_   PULONG crc32
)
{
    OBJECT_ATTRIBUTES sourceAttributes;
    HANDLE sourceHandle = NULL;
    IO_STATUS_BLOCK ioStatus;
    *crc32 = 0;

    // ����� ���� ���� ����
    InitializeObjectAttributes(&sourceAttributes, srcPath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    // �ٸ� ���μ������� ���� �ڵ��� ������� �� ACCESS_MASK�� SHARE_ACCESS�� ��� ������ �ο����� ������ 0xC0000043 (STATUS_SHARING_VIOLATION) ������ �߻��� �� ����
    NTSTATUS status = ZwCreateFile(
        &sourceHandle, GENERIC_ALL, &sourceAttributes, &ioStatus, NULL, FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_VALID_FLAGS, FILE_OPEN, FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0
    );

    if (!NT_SUCCESS(status))
    {
        DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[FileRtv] GetFileCRC() Failed to open source file: 0x%x\n", status);
        
        return status;
    }

    FILE_STANDARD_INFORMATION fileInfo = { 0 };
    status = ZwQueryInformationFile(sourceHandle, &ioStatus, &fileInfo, sizeof(fileInfo), FileStandardInformation);
    if (!NT_SUCCESS(status))
    {
        DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[FileRtv] GetFileCRC() ZwQueryInformationFile() failed: 0x%x\n", status);
        ZwClose(sourceHandle);
        return status;
    }

    ULONG CRC = 0;
    ULONG table[256];
    MakeCRCtable(table, 0xEDB88320);
    char buffer[4096];
    ULONG bufferSize = sizeof(buffer);
    for (LONGLONG i = 0; i < fileInfo.EndOfFile.QuadPart; i += 4096)
    {
        // 4096����Ʈ ������ ���� ������ ���� ũ�Ⱑ 4096���� ���� ��� bufferSize�� �������Ѵ�.
        if (fileInfo.EndOfFile.QuadPart - i < 4096)
            bufferSize = (ULONG)(fileInfo.EndOfFile.QuadPart - i);

        status = ZwReadFile(sourceHandle, NULL, NULL, NULL, &ioStatus, buffer, bufferSize, NULL, NULL);
        if (!NT_SUCCESS(status))
        {
            DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[FileRtv] GetFileCRC() ZwReadFile() failed: 0x%x\n", status);
            ZwClose(sourceHandle);
            return status;
        }

        CRC = CalcCRC((LPCSTR)buffer, bufferSize, CRC, table);
    }

    ZwClose(sourceHandle);
    *crc32 = CRC;
    return 0;
}


ULONG CalcCRC(
    _In_    LPCSTR mem,
    _In_    LONG size,
    _In_    ULONG CRC,
    _In_    PULONG table
)
{
    CRC = ~CRC;

    while (size--)
        CRC = table[(CRC ^ *(mem++)) & 0xFF] ^ (CRC >> 8);

    return ~CRC;
}

VOID MakeCRCtable(
    _Out_    PULONG table,
    _In_    ULONG id
)
{
    ULONG i, j, k;

    for (i = 0; i < 256; ++i)
    {
        k = i;
        for (j = 0; j < 8; ++j)
        {
            if (k & 1) k = (k >> 1) ^ id;
            else k >>= 1;
        }
        table[i] = k;
    }
}