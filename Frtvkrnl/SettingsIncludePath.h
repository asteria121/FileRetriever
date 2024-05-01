#ifndef _SETTINGS_INCLUDE_PATH_H_
#define _SETTINGS_INCLUDE_PATH_H_

#define MAX_KERNEL_PATH 32767

typedef struct INCPATHLIST
{
	PWCHAR Path;
	LONGLONG MaximumSize;
	struct INCPATHLIST* PrevNode;
	struct INCPATHLIST* NextNode;
} INCPATHLIST, * PINCPATHLIST;

typedef enum INCPATHRESULT
{
	INCPATH_OPERATION_SUCCESS,
	INCPATH_EXISTS,
	INCPATH_NOT_EXISTS,
	INCPATH_TOO_LONG,
	INCPATH_OUT_OF_MEMORY,
} INCPATHRESULT;

VOID InitializeIncludePathList();

PINCPATHLIST FindIncludePath(
	_In_	LPWSTR exceptionPath,
	_In_	BOOLEAN checkSubDirectory
);

int RemoveIncludePath(
	_In_	LPWSTR exceptionPath
);

int AddNewIncludePath(
	_In_	LPWSTR exceptionPath,
	_In_	LONGLONG maximumFileSize
);

#endif