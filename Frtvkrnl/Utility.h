#ifndef _UTILITY_H_
#define _UTILITY_H_

#ifdef _DEBUG
#define DBGPRT(...) DbgPrintEx(__VA_ARGS__)
#else
#define DBGPRT(...)
#endif

NTSTATUS AllocUnicodeString(
	PUNICODE_STRING dst,
	USHORT maximumBufferLen
);

VOID FreeUnicodeString(
	PUNICODE_STRING dst
);

#endif