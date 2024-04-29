#ifndef _SETTINGS_EXTENSION_H_
#define _SETTINGS_EXTENSION_H_

#define MAX_EXTENSION_LEN 24

typedef struct EXTENSIONLIST
{
	WCHAR Name[MAX_EXTENSION_LEN];
	LONGLONG MaximumSize;
	struct EXTENSIONLIST* PrevNode;
	struct EXTENSIONLIST* NextNode;
} EXTENSIONLIST, * PEXTENSIONLIST;

typedef enum EXTRESULT
{
	EXT_OPERATION_SUCCESS,
	EXT_EXISTS,
	EXT_NOT_EXISTS,
	EXT_TOO_LONG,
	EXT_OUT_OF_MEMORY,
} EXTRESULT;

VOID InitializeExtensionList();

PEXTENSIONLIST FindExtension(
	_In_		LPWSTR extension
);

int RemoveExtension(
	_In_	LPWSTR extension
);

int AddNewExtension(
	_In_	LPWSTR extension,
	_In_	LONGLONG fileSize
);

#endif