#ifndef _CALLBACKS_H_
#define _CALLBACKS_H_

#include <Windows.h>

#define LOG_LEVEL_DEBUG 1
#define LOG_LEVEL_NORMAL 2
#define LOG_LEVEL_ERROR 3

typedef void __stdcall DebugCallback(DWORD logLevel, LPCSTR message);
typedef void __stdcall DBCallback(LPCSTR fileName, DWORD crc32);
typedef void __stdcall ConnectCallback();
typedef void __stdcall DisconnectCallback();

void CallDebugCallback(DWORD logLevel, LPCSTR message);
void CallDBCallback(LPCSTR fileName, DWORD crc32);
void CallConnectCallback();
void CallDisconnectCallback();
extern "C" __declspec(dllexport) void RegisterCallbacks(DebugCallback * guiCallback, DBCallback* dbCallback, ConnectCallback* ccCallback, DisconnectCallback* dcCallback);

#endif