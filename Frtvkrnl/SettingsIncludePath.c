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
/// 백업 포함 경로를 찾는 함수
/// </summary>
/// <param name="exceptionPath">찾을 예외 경로</param>
/// <param name="checkSubDirectory">등록된 경로 중 하위 폴더에 포함될 수 있는 포함 경로도 반환할지</param>
/// <returns>찾은 경로 or NULL</returns>
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

		// 대소문자를 무시한다
		if (checkSubDirectory == TRUE)
		{
			// 상위폴더 또는 하위폴더가 포함되어있는지 확인한다.
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
			// 같을 경우 0을 반환함
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
				// 노드 맨 앞이고 뒤에 노드가 있는 경우
				incPathHead = targetNode->NextNode;
				targetNode->NextNode->PrevNode = NULL;
			}
			else
			{
				// 노드 맨 앞이고 혼자인 경우
				incPathHead = NULL;
				incPathTail = NULL;
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
		POOL_FLAG_NON_PAGED,	// 항상 메모리에 존재, 페이지 아웃 혹은 인이 되는데 페이지 아웃이 되면 BSoD 발생
		sizeof(INCPATHLIST),	// 할당 메모리 크기
		'frtv'					// 메모리 식별자
	);

	if (newNode != NULL)
	{
		newPath = (PWCHAR)ExAllocatePool2(
			POOL_FLAG_NON_PAGED,			// 항상 메모리에 존재, 페이지 아웃 혹은 인이 되는데 페이지 아웃이 되면 BSoD 발생
			(pathLen + 1) * sizeof(WCHAR),	// 할당 메모리 크기
			'frtv'							// 메모리 식별자
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