#include <ntddk.h>
#include <ntstrsafe.h>
#include "SettingsExtension.h"
#include "Utility.h"


static PEXTENSIONLIST extHead, extTail;

VOID InitializeExtensionList()
{
	extHead = NULL;
	extTail = NULL;
}

PEXTENSIONLIST FindExtension(
	_In_		LPWSTR extension
)
{
	DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[LinkedList] Finding extension: %ws.\r\n", extension);
	PEXTENSIONLIST currentNode = extHead;
	UNICODE_STRING ext1, ext2;
	RtlInitUnicodeString(&ext1, extension);
	while (currentNode != NULL)
	{
		RtlInitUnicodeString(&ext2, currentNode->Name);

		// MaximumSize가 0인 경우 파일 크기에 상관없이 반환함
		// 또는 fileSize가 0인 경우
		if (RtlCompareUnicodeString(&ext1, &ext2, FALSE) == 0)
		{
			return currentNode;
		}

		if (currentNode->NextNode == NULL)
			break;
		else
			currentNode = currentNode->NextNode;
	}

	return NULL;
}

int RemoveExtension(
	_In_	LPWSTR extension
)
{
	SIZE_T extensionLen;
	NTSTATUS status = RtlStringCchLengthW(extension, MAX_EXTENSION_LEN, &extensionLen);
	if (status == STATUS_INVALID_PARAMETER)
	{
		DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[LinkedList] Extension length is longer than %dB.\r\n", MAX_EXTENSION_LEN);
		return EXT_TOO_LONG;
	}

	PEXTENSIONLIST targetNode = FindExtension(extension);
	if (targetNode != NULL)
	{
		if (targetNode == extHead)
		{
			if (targetNode->NextNode != NULL)
			{
				// 노드 맨 앞이고 뒤에 노드가 있는 경우
				extHead = targetNode->NextNode;
				targetNode->NextNode->PrevNode = NULL;
			}
			else
			{
				// 노드 맨 앞이고 혼자인 경우
				extHead = NULL;
				extTail = NULL;
			}
		}
		else
		{
			if (targetNode->NextNode != NULL)
			{
				// 노드 중간에 있는 경우
				targetNode->NextNode->PrevNode = targetNode->PrevNode;
				targetNode->PrevNode->NextNode = targetNode->NextNode;
			}
			else
			{
				// 노드 맨 끝에 위치한 경우
				extTail = targetNode->PrevNode;
				targetNode->PrevNode->NextNode = NULL;
			}
		}

		RtlZeroMemory(targetNode, sizeof(EXTENSIONLIST));
		ExFreePoolWithTag(targetNode, 'frtv');

		return EXT_OPERATION_SUCCESS;
	}
	else
	{
		return EXT_NOT_EXISTS;
	}
}

int AddNewExtension(
	_In_	LPWSTR extension,
	_In_	LONGLONG maximumFileSize
)
{
	SIZE_T extensionLen;
	NTSTATUS status = RtlStringCchLengthW(extension, MAX_EXTENSION_LEN, &extensionLen);
	PEXTENSIONLIST newNode = NULL;
	if (status == STATUS_INVALID_PARAMETER)
	{
		DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[LinkedList] Extension length is longer than %dB.\r\n", MAX_EXTENSION_LEN);
		return EXT_TOO_LONG;
	}
	
	if (FindExtension(extension) != NULL)
	{
		DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[LinkedList] %ws is already exists.\r\n", extension);
		return EXT_EXISTS;
	}

	// NonPagedPool 
	newNode = (PEXTENSIONLIST)ExAllocatePool2(
		POOL_FLAG_NON_PAGED,	// 항상 메모리에 존재, 페이지 아웃 혹은 인이 되는데 페이지 아웃이 되면 BSoD 발생
		sizeof(EXTENSIONLIST),	// 할당 메모리 크기
		'frtv'					// 메모리 식별자
	);
	
	if (newNode != NULL)
	{
		DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[LinkedList] Memory allocation success.\r\n");
		RtlZeroMemory(newNode, sizeof(EXTENSIONLIST));
		RtlStringCchCopyW(newNode->Name, MAX_EXTENSION_LEN, extension);
		newNode->MaximumSize = maximumFileSize;

		if (extHead == NULL)
		{
			extHead = newNode;
			extTail = newNode;
		}
		else
		{
			extTail->NextNode = newNode;
			newNode->PrevNode = extTail;
			extTail = newNode;
		}
	}
	else
	{
		DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[LinkedList] Memory allocation failed.\r\n");
		return EXT_OUT_OF_MEMORY;
	}

	return STATUS_SUCCESS;
}