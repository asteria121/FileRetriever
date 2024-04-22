#ifndef _CONVERT_PATH_H_
#define _CONVERT_PATH_H_

#include <Windows.h>

BOOL GetDeivceFolderName(LPWSTR pszDeviceFileName, LPCWSTR pszWin32FileName);
BOOL GetDeivceFileName(LPWSTR pszDeviceFileName, LPCWSTR pszWin32FileName);
BOOL GetWin32FileName(LPCWSTR pszDeviceFileName, LPWSTR pszWin32FileName);

#endif