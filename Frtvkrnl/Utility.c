#include <ntddk.h>
#include "Utility.h"

/// <summary>
/// UNICODE_STRING 구조체를 동적으로 할당한다
/// </summary>
/// <param name="dst">UNICODE_STRING 포인터</param>
/// <param name="maximumBufferLen">Buffer의 최대 바이트 수 (문자 수 아님)</param>
/// <returns>NTSTATUS</returns>
NTSTATUS AllocUnicodeString(
	PUNICODE_STRING dst,
	USHORT maximumBufferLen
)
{
	dst->Buffer = (PWCH)ExAllocatePool2(POOL_FLAG_NON_PAGED, maximumBufferLen, 'frtv');

	if (dst->Buffer == NULL)
		return STATUS_UNSUCCESSFUL;

	RtlZeroMemory(dst->Buffer, maximumBufferLen);
	dst->Length = 0;
	dst->MaximumLength = maximumBufferLen;
	
	return STATUS_SUCCESS;
}

/// <summary>
/// 동적 할당된 UNICOIDE_STRING 구조체를 할당 해제한다
/// </summary>
/// <param name="dst">UNICODE_STRING 포인터</param>
VOID FreeUnicodeString(
	PUNICODE_STRING dst
)
{
	if (dst->Buffer != NULL)
	{
		RtlZeroMemory(dst->Buffer, dst->MaximumLength);
		RtlZeroMemory(dst, sizeof(UNICODE_STRING));
		ExFreePoolWithTag(dst->Buffer, 'frtv');
	}
}