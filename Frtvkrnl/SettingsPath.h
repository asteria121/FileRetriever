#ifndef _SETTINGS_PATH_H_
#define _SETTINGS_PATH_H_

#define MAX_KERNEL_PATH 32767

typedef struct PATHLIST
{
	PWCHAR Path;
	struct PATHLIST* PrevNode;
	struct PATHLIST* NextNode;
} PATHLIST, * PPATHLIST;

typedef enum PATHRESULT
{
	PATH_OPERATION_SUCCESS,
	PATH_EXISTS,
	PATH_NOT_EXISTS,
	PATH_TOO_LONG,
	PATH_OUT_OF_MEMORY,
} PATHRESULT;

VOID InitializePathList();

VOID PrintPathList();

PPATHLIST FindExceptionPath(
	_In_		LPWSTR exceptionPath
);

int RemoveExceptionPath(
	_In_	LPWSTR exceptionPath
);

int AddNewExceptionPath(
	_In_	LPWSTR exceptionPath
);

NTSTATUS ToggleException(
	_In_	BOOLEAN isException
);

#endif