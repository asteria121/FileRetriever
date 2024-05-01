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
/// 백업 예외 경로를 찾는 함수
/// </summary>
/// <param name="exceptionPath">찾을 예외 경로</param>
/// <param name="checkSubDirectory">등록된 경로 중 하위 폴더에 포함될 수 있는 예외 경로도 반환할지</param>
/// <returns>찾은 경로 or NULL</returns>
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
		
		// 대소문자를 무시한다
		if (checkSubDirectory == TRUE)
		{
			if (RtlPrefixUnicodeString(&listPath, &paramPath, TRUE) == TRUE)
				return currentNode;
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
				// 노드 맨 앞이고 뒤에 노드가 있는 경우
				pathHead = targetNode->NextNode;
				targetNode->NextNode->PrevNode = NULL;
			}
			else
			{
				// 노드 맨 앞이고 혼자인 경우
				pathHead = NULL;
				pathTail = NULL;
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
		POOL_FLAG_NON_PAGED,	// 항상 메모리에 존재, 페이지 아웃 혹은 인이 되는데 페이지 아웃이 되면 BSoD 발생
		sizeof(PATHLIST),		// 할당 메모리 크기
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