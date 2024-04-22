#ifndef _CALLBACKS_H_
#define _CALLBACKS_H_

#include <Windows.h>

typedef void __stdcall DebugCallback(LPCSTR message);
typedef void __stdcall DBCallback(LPCSTR fileName, DWORD crc32);
typedef void __stdcall ConnectCallback();
typedef void __stdcall DisconnectCallback();

void CallDebugCallback(LPCSTR message);
void CallDBCallback(LPCSTR fileName, DWORD crc32);
void CallConnectCallback();
void CallDisconnectCallback();
extern "C" __declspec(dllexport) void RegisterCallbacks(DebugCallback * guiCallback, DBCallback* dbCallback, ConnectCallback* ccCallback, DisconnectCallback* dcCallback);

#endif