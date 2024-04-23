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
	_In_		LPWSTR extension,
	_In_opt_	LONGLONG fileSize
)
{
	DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[LinkedList] Finding extension: %ws.\r\n", extension);
	PEXTENSIONLIST currentNode = extHead;
	UNICODE_STRING ext1, ext2;
	RtlInitUnicodeString(&ext1, extension);
	while (currentNode != NULL)
	{
		RtlInitUnicodeString(&ext2, currentNode->Name);

		// MaximumSize�� 0�� ��� ���� ũ�⿡ ������� ��ȯ��
		// �Ǵ� fileSize�� 0�� ���
		if (RtlCompareUnicodeString(&ext1, &ext2, FALSE) == 0 &&
			(fileSize <= currentNode->MaximumSize || currentNode->MaximumSize == 0 || fileSize == 0))
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

VOID PrintExtensionList()
{
	PEXTENSIONLIST currentNode = extHead;
	DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "Extensions: ");
	while (currentNode != NULL)
	{
		DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "%ws,", currentNode->Name);

		if (currentNode->NextNode == NULL)
			break;
		else
			currentNode = currentNode->NextNode;
	}

	DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "\n");
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

	PEXTENSIONLIST targetNode = FindExtension(extension, 0);
	if (targetNode != NULL)
	{
		if (targetNode == extHead)
		{
			if (targetNode->NextNode != NULL)
			{
				// ��� �� ���̰� �ڿ� ��尡 �ִ� ���
				extHead = targetNode->NextNode;
				targetNode->NextNode->PrevNode = NULL;
			}
			else
			{
				// ��� �� ���̰� ȥ���� ���
				extHead = NULL;
				extTail = NULL;
			}
		}
		else
		{
			if (targetNode->NextNode != NULL)
			{
				// ��� �߰��� �ִ� ���
				targetNode->NextNode->PrevNode = targetNode->PrevNode;
				targetNode->PrevNode->NextNode = targetNode->NextNode;
			}
			else
			{
				// ��� �� ���� ��ġ�� ���
				extTail = targetNode->PrevNode;
				targetNode->PrevNode->NextNode = NULL;
			}
		}

		RtlZeroMemory(targetNode, sizeof(EXTENSIONLIST));
		ExFreePoolWithTag(targetNode, 'frtv');

		PrintExtensionList();
		return EXT_OPERATION_SUCCESS;
	}
	else
	{
		return EXT_NOT_EXISTS;
	}
}

int AddNewExtension(
	_In_	LPWSTR extension,
	_In_	LONGLONG fileSize
)
{
	SIZE_T extensionLen;
	NTSTATUS status = RtlStringCchLengthW(extension, MAX_EXTENSION_LEN, &extensionLen);
	if (status == STATUS_INVALID_PARAMETER)
	{
		DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[LinkedList] Extension length is longer than %dB.\r\n", MAX_EXTENSION_LEN);
		return EXT_TOO_LONG;
	}
	
	if (FindExtension(extension, 0) != NULL)
	{
		DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[LinkedList] %ws is already exists.\r\n", extension);
		return EXT_EXISTS;
	}

	// NonPagedPool 
	PEXTENSIONLIST newNode = (PEXTENSIONLIST)ExAllocatePool2(
		POOL_FLAG_NON_PAGED,	// �׻� �޸𸮿� ����, ������ �ƿ� Ȥ�� ���� �Ǵµ� ������ �ƿ��� �Ǹ� BSoD �߻�
		sizeof(EXTENSIONLIST),	// �Ҵ� �޸� ũ��
		'frtv'					// �޸� �ĺ���
	);
	
	if (newNode != NULL)
	{
		DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[LinkedList] Memory allocation success.\r\n");
		RtlZeroMemory(newNode, sizeof(EXTENSIONLIST));
		RtlStringCchCopyW(newNode->Name, MAX_EXTENSION_LEN, extension);
		newNode->MaximumSize = fileSize;

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

	PrintExtensionList();

	return STATUS_SUCCESS;
}