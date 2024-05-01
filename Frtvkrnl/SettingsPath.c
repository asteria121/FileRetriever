#include <fltKernel.h>
#include <ntstrsafe.h>
#include "SettingsPath.h"
#include "MiniFilterPort.h"
#include "Utility.h"

static PPATHLIST pathHead, pathTail;

VOID InitializePathList()
{
	pathHead = NULL;
	pathTail = NULL;
}

/// <summary>
/// ��� ���� ��θ� ã�� �Լ�
/// </summary>
/// <param name="exceptionPath">ã�� ���� ���</param>
/// <param name="checkSubDirectory">��ϵ� ��� �� ���� ������ ���Ե� �� �ִ� ���� ��ε� ��ȯ����</param>
/// <returns>ã�� ��� or NULL</returns>
PPATHLIST FindExceptionPath(
	_In_	LPWSTR exceptionPath,
	_In_	BOOLEAN checkSubDirectory
)
{
	DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[LinkedList] Finding path: %ws.\r\n", exceptionPath);
	PPATHLIST currentNode = pathHead;
	UNICODE_STRING paramPath, listPath;
	RtlInitUnicodeString(&paramPath, exceptionPath);
	
	while (currentNode != NULL)
	{
		RtlInitUnicodeString(&listPath, currentNode->Path);
		
		// ��ҹ��ڸ� �����Ѵ�
		if (checkSubDirectory == TRUE)
		{
			if (RtlPrefixUnicodeString(&listPath, &paramPath, TRUE) == TRUE)
				return currentNode;
		}
		else
		{
			// https://learn.microsoft.com/ko-kr/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlcompareunicodestring
			// ���� ��� 0�� ��ȯ��
			if (RtlCompareUnicodeString(&listPath, &paramPath, TRUE) == 0)
				return currentNode;
		}

		if (currentNode->NextNode == NULL)
			break;
		else
			currentNode = currentNode->NextNode;
	}

	return NULL;
}

int RemoveExceptionPath(
	_In_	LPWSTR exceptionPath
)
{
	SIZE_T extensionLen;
	NTSTATUS status;
	PPATHLIST targetNode = NULL;

	status = RtlStringCchLengthW(exceptionPath, MAX_KERNEL_PATH, &extensionLen);
	if (status == STATUS_INVALID_PARAMETER)
	{
		DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[LinkedList] Extension length is longer than %dB.\r\n", MAX_KERNEL_PATH);
		return PATH_TOO_LONG;
	}

	targetNode = FindExceptionPath(exceptionPath, FALSE);
	if (targetNode != NULL)
	{
		if (targetNode == pathHead)
		{
			if (targetNode->NextNode != NULL)
			{
				// ��� �� ���̰� �ڿ� ��尡 �ִ� ���
				pathHead = targetNode->NextNode;
				targetNode->NextNode->PrevNode = NULL;
			}
			else
			{
				// ��� �� ���̰� ȥ���� ���
				pathHead = NULL;
				pathTail = NULL;
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
				pathTail = targetNode->PrevNode;
				targetNode->PrevNode->NextNode = NULL;
			}
		}

		ExFreePoolWithTag(targetNode->Path, 'frtv');
		RtlZeroMemory(targetNode, sizeof(PATHLIST));
		ExFreePoolWithTag(targetNode, 'frtv');

		return PATH_OPERATION_SUCCESS;
	}
	else
	{
		return PATH_NOT_EXISTS;
	}
}

int AddNewExceptionPath(
	_In_	LPWSTR exceptionPath
)
{
	SIZE_T pathLen;
	NTSTATUS status;
	PPATHLIST newNode = NULL;
	PWCHAR newPath = NULL;

	status = RtlStringCchLengthW(exceptionPath, MAX_KERNEL_PATH, &pathLen);
	if (status == STATUS_INVALID_PARAMETER)
	{
		DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[LinkedList] Extension length is longer than %dB.\r\n", MAX_KERNEL_PATH);
		return PATH_TOO_LONG;
	}

	if (FindExceptionPath(exceptionPath, FALSE) != NULL)
	{
		DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[LinkedList] %ws is already exists.\r\n", exceptionPath);
		return PATH_EXISTS;
	}

	newNode = (PPATHLIST)ExAllocatePool2(
		POOL_FLAG_NON_PAGED,	// �׻� �޸𸮿� ����, ������ �ƿ� Ȥ�� ���� �Ǵµ� ������ �ƿ��� �Ǹ� BSoD �߻�
		sizeof(PATHLIST),		// �Ҵ� �޸� ũ��
		'frtv'					// �޸� �ĺ���
	);

	

	if (newNode != NULL)
	{
		newPath = (PWCHAR)ExAllocatePool2(
			POOL_FLAG_NON_PAGED,			// �׻� �޸𸮿� ����, ������ �ƿ� Ȥ�� ���� �Ǵµ� ������ �ƿ��� �Ǹ� BSoD �߻�
			(pathLen + 1) * sizeof(WCHAR),	// �Ҵ� �޸� ũ��
			'frtv'							// �޸� �ĺ���
		);
		
		if (newPath == NULL)
		{
			ExFreePoolWithTag(newNode, 'frtv');
			return PATH_OUT_OF_MEMORY;
		}

		RtlZeroMemory(newNode, sizeof(PATHLIST));
		newNode->Path = newPath;
		RtlStringCchCopyW(newNode->Path, (pathLen + 1), exceptionPath);

		if (pathHead == NULL)
		{
			pathHead = newNode;
			pathTail = newNode;
		}
		else
		{
			pathTail->NextNode = newNode;
			newNode->PrevNode = pathTail;
			pathTail = newNode;
		}
	}
	else
	{
		DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[LinkedList] Memory allocation failed.\r\n");
		return PATH_OUT_OF_MEMORY;
	}

	return PATH_OPERATION_SUCCESS;
}

NTSTATUS ToggleException(
	_In_	BOOLEAN isException
)
{
	UNREFERENCED_PARAMETER(isException);
	NTSTATUS status = STATUS_SUCCESS;
	return status;
}