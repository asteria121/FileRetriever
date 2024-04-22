#include <ntddk.h>
#include "Utility.h"

/// <summary>
/// UNICODE_STRING ����ü�� �������� �Ҵ��Ѵ�
/// </summary>
/// <param name="dst">UNICODE_STRING ������</param>
/// <param name="maximumBufferLen">Buffer�� �ִ� ����Ʈ �� (���� �� �ƴ�)</param>
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
/// ���� �Ҵ�� UNICOIDE_STRING ����ü�� �Ҵ� �����Ѵ�
/// </summary>
/// <param name="dst">UNICODE_STRING ������</param>
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