#ifndef _FRTV_COMMUNICATE_H_
#define _FRTV_COMMUNICATE_H_
#include <Windows.h>
#include "FrtvConnection.h"
#include "IOCP.h"

int SendMinifltPortW(int rtvCode, LPCWSTR msg, DWORD crc32, LONGLONG fileSize, HRESULT* hr);
extern "C" __declspec(dllexport) int InitializeCommunicator();

extern "C" __declspec(dllexport) int AddExceptionPath(LPCSTR path, HRESULT* hr);
extern "C" __declspec(dllexport) int RemoveExceptionPath(LPCSTR path, HRESULT * hr);
extern "C" __declspec(dllexport) int AddIncludePath(LPCSTR path, LONGLONG maximumFileSize, HRESULT * hr);
extern "C" __declspec(dllexport) int RemoveIncludePath(LPCSTR path, HRESULT * hr);
extern "C" __declspec(dllexport) int AddExtension(LPCSTR extension, LONGLONG maximumFileSize, HRESULT * hr);
extern "C" __declspec(dllexport) int RemoveExtension(LPCSTR extension, HRESULT * hr);

extern "C" __declspec(dllexport) int ToggleBackupSwitch(int enabled, HRESULT * hr);
extern "C" __declspec(dllexport) int UpdateBackupFolder(LPCSTR folder, HRESULT * hr);
extern "C" __declspec(dllexport) int RestoreBackupFile(LPCSTR dstPath, DWORD crc32, BOOLEAN overwriteDst, HRESULT * hr);
extern "C" __declspec(dllexport) int DeleteBackupFile(DWORD crc32, HRESULT* hr);

#endif