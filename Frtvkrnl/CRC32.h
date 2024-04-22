#ifndef _CRC32_H_
#define _CRC32_H_

NTSTATUS GetFileCRC(
    _In_    PUNICODE_STRING srcPath,
    _Out_   PULONG crc32
);

ULONG CalcCRC(
    _In_    LPCSTR mem,
    _In_    LONG size,
    _In_    ULONG CRC,
    _In_    PULONG table
);

VOID MakeCRCtable(
    _Out_    PULONG table,
    _In_    ULONG id
);

#endif