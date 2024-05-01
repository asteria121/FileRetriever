#include <fltKernel.h>
#include <ntstrsafe.h>
#include "SettingsIncludePath.h"
#include "MiniFilterPort.h"
#include "Utility.h"

static PINCPATHLIST incPathHead, incPathTail;

VOID InitializeIncludePathList()
{
	incPathHead = NULL;
	incPathTail = NULL;
}

/// <summary>
/// ��� ���� ��θ� ã�� �Լ�
/// </summary>
/// <param name="exceptionPath">ã�� ���� ���</param>
/// <param name="checkSubDirectory">��ϵ� ��� �� ���� ������ ���Ե� �� �ִ� ���� ��ε� ��ȯ����</param>
/// <returns>ã�� ��� or NULL</returns>
PINCPATHLIST FindIncludePath(
	_In_	LPWSTR exceptionPath,
	_In_	BOOLEAN checkSubDirectory
)
{
	DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[LinkedList] Finding include path: %ws.\r\n", exceptionPath);
	PINCPATHLIST currentNode = incPathHead;
	UNICODE_STRING paramPath, listPath;
	RtlInitUnicodeString(&paramPath, exceptionPath);

	while (currentNode != NULL)
	{
		RtlInitUnicodeString(&listPath, currentNode->Path);

		// ��ҹ��ڸ� �����Ѵ�
		if (checkSubDirectory == TRUE)
		{
			// �������� �Ǵ� ���������� ���ԵǾ��ִ��� Ȯ���Ѵ�.
			if (listPath.Length > paramPath.Length)
			{
				if (RtlPrefixUnicodeString(&paramPath, &listPath, TRUE) == TRUE)
					return currentNode;
			}
			else
			{
				if (RtlPrefixUnicodeString(&listPath, &paramPath, TRUE) == TRUE)
					return currentNode;
			}
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

int RemoveIncludePath(
	_In_	LPWSTR exceptionPath
)
{
	SIZE_T extensionLen;
	NTSTATUS status;
	PINCPATHLIST targetNode = NULL;

	status = RtlStringCchLengthW(exceptionPath, MAX_KERNEL_PATH, &extensionLen);
	if (status == STATUS_INVALID_PARAMETER)
	{
		DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[LinkedList] Extension length is longer than %dB.\r\n", MAX_KERNEL_PATH);
		return INCPATH_TOO_LONG;
	}

	targetNode = FindIncludePath(exceptionPath, FALSE);
	if (targetNode != NULL)
	{
		if (targetNode == incPathHead)
		{
			if (targetNode->NextNode != NULL)
			{
				// ��� �� ���̰� �ڿ� ��尡 �ִ� ���
				incPathHead = targetNode->NextNode;
				targetNode->NextNode->PrevNode = NULL;
			}
			else
			{
				// ��� �� ���̰� ȥ���� ���
				incPathHead = NULL;
				incPathTail = NULL;
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
				incPathTail = targetNode->PrevNode;
				targetNode->PrevNode->NextNode = NULL;
			}
		}

		ExFreePoolWithTag(targetNode->Path, 'frtv');
		RtlZeroMemory(targetNode, sizeof(INCPATHLIST));
		ExFreePoolWithTag(targetNode, 'frtv');

		return INCPATH_OPERATION_SUCCESS;
	}
	else
	{
		return INCPATH_NOT_EXISTS;
	}
}

int AddNewIncludePath(
	_In_	LPWSTR exceptionPath,
	_In_	LONGLONG maximumFileSize
)
{
	SIZE_T pathLen;
	NTSTATUS status;
	PINCPATHLIST newNode = NULL;
	PWCHAR newPath = NULL;

	status = RtlStringCchLengthW(exceptionPath, MAX_KERNEL_PATH, &pathLen);
	if (status == STATUS_INVALID_PARAMETER)
	{
		DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[LinkedList] Path length is longer than %dB.\r\n", MAX_KERNEL_PATH);
		return INCPATH_TOO_LONG;
	}

	if (FindIncludePath(exceptionPath, TRUE) != NULL)
	{
		DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[LinkedList] %ws is already exists.\r\n", exceptionPath);
		return INCPATH_EXISTS;
	}

	newNode = (PINCPATHLIST)ExAllocatePool2(
		POOL_FLAG_NON_PAGED,	// �׻� �޸𸮿� ����, ������ �ƿ� Ȥ�� ���� �Ǵµ� ������ �ƿ��� �Ǹ� BSoD �߻�
		sizeof(INCPATHLIST),	// �Ҵ� �޸� ũ��
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
			return INCPATH_OUT_OF_MEMORY;
		}

		RtlZeroMemory(newNode, sizeof(INCPATHLIST));
		newNode->Path = newPath;
		RtlStringCchCopyW(newNode->Path, (pathLen + 1), exceptionPath);
		newNode->MaximumSize = maximumFileSize;

		if (incPathHead == NULL)
		{
			incPathHead = newNode;
			incPathTail = newNode;
		}
		else
		{
			incPathTail->NextNode = newNode;
			newNode->PrevNode = incPathTail;
			incPathTail = newNode;
		}
	}
	else
	{
		DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[LinkedList] Memory allocation failed.\r\n");
		return INCPATH_OUT_OF_MEMORY;
	}

	return INCPATH_OPERATION_SUCCESS;
}