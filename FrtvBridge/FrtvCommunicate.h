#ifndef _FRTV_COMMUNICATE_H_
#define _FRTV_COMMUNICATE_H_
#include <Windows.h>
#include "FrtvConnection.h"
#include "IOCP.h"

extern "C" __declspec(dllexport) int SendMinifltPortA(int rtvCode, LPCSTR msg, DWORD crc32, LONGLONG fileSize);
extern "C" __declspec(dllexport) int SendMinifltPortW(int rtvCode, LPCWSTR msg, DWORD crc32, LONGLONG fileSize);
extern "C" __declspec(dllexport) int InitializeCommunicator();

extern "C" __declspec(dllexport) int AddExceptionPath(LPCSTR path);
extern "C" __declspec(dllexport) int RemoveExceptionPath(LPCSTR path);
extern "C" __declspec(dllexport) int AddExtension(LPCSTR extension, LONGLONG maximumFileSize);
extern "C" __declspec(dllexport) int RemoveExtension(LPCSTR extension);

extern "C" __declspec(dllexport) int ToggleBackupSwitch(int enabled);
extern "C" __declspec(dllexport) int UpdateBackupFolder(LPCSTR folder);
extern "C" __declspec(dllexport) int RestoreBackupFile(LPCSTR dstPath, DWORD crc32, BOOLEAN overwriteDst);
extern "C" __declspec(dllexport) int DeleteBackupFile(DWORD crc32);

#endif