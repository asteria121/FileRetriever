#include <strsafe.h>
#include "ConvertPath.h"

/// <summary>
/// C:\Users... 따위의 경로를 \\Device\\HardDiskVolume0 와 같은 디바이스 단위 경로로 변환하는 함수
/// 커널 드라이버에 경로를 전달할 때 사용
/// </summary>
/// <param name="pszDeviceFileName">커널에서 사용하는 디바이스 단위 경로를 포인터로 반환</param>
/// <param name="pszWin32FileName">드라이브 라벨이 붙는 Win32 경로</param>
/// <returns>성공 여부에 따라 TRUE, FALSE를 반환</returns>
BOOL GetDeivceFolderName(LPWSTR pszDeviceFileName, LPCWSTR pszWin32FileName)
{
	BOOL bFound = FALSE;
	WCHAR dosDrive[MAX_PATH];
	RtlZeroMemory(dosDrive, MAX_PATH * sizeof(WCHAR));
	WCHAR win32Drive[3] = L" :";
	*win32Drive = *pszWin32FileName;

	if (QueryDosDeviceW(win32Drive, dosDrive, MAX_PATH))
	{
		StringCchPrintfW(pszDeviceFileName,
			MAX_PATH,
			L"%ws%ws\\",
			dosDrive,
			pszWin32FileName + 2);
		bFound = TRUE;
	}

	return bFound;
}

/// <summary>
/// C:\Users... 따위의 경로를 \\Device\\HardDiskVolume0 와 같은 디바이스 단위 경로로 변환하는 함수
/// 커널 드라이버에 경로를 전달할 때 사용
/// </summary>
/// <param name="pszDeviceFileName">커널에서 사용하는 디바이스 단위 경로를 포인터로 반환</param>
/// <param name="pszWin32FileName">드라이브 라벨이 붙는 Win32 경로</param>
/// <returns>성공 여부에 따라 TRUE, FALSE를 반환</returns>
BOOL GetDeivceFileName(LPWSTR pszDeviceFileName, LPCWSTR pszWin32FileName)
{
	BOOL bFound = FALSE;
	WCHAR dosDrive[MAX_PATH];
	RtlZeroMemory(dosDrive, MAX_PATH * sizeof(WCHAR));
	WCHAR win32Drive[3] = L" :";
	*win32Drive = *pszWin32FileName;

	if (QueryDosDeviceW(win32Drive, dosDrive, MAX_PATH))
	{
		StringCchPrintfW(pszDeviceFileName,
			MAX_PATH,
			L"%ws%ws",
			dosDrive,
			pszWin32FileName + 2);
		bFound = TRUE;
	}

	return bFound;
}

/// <summary>
/// \\Device\\HardDiskVolume0 따위의 경로를 C:\Users... 와 같은 Win32 경로로 변환하는 함수
/// 커널 드라이버로부터 수신받은 경로를 GUI에 전달할 때 사용
/// </summary>
/// <param name="pszDeviceFileName">커널에서 사용하는 디바이스 단위 경로</param>
/// <param name="pszWin32FileName">드라이브 라벨이 붙는 Win32 경로를 포인터로 반환</param>
/// <returns>성공 여부에 따라 TRUE, FALSE를 반환</returns>
BOOL GetWin32FileName(LPCWSTR pszDeviceFileName, LPWSTR pszWin32FileName)
{
	BOOL bFound = FALSE;

	// Translate path with device name to drive letters.
	WCHAR szTemp[MAX_PATH];
	RtlZeroMemory(szTemp, MAX_PATH * sizeof(WCHAR));

	if (GetLogicalDriveStringsW(MAX_PATH - 1, szTemp))
	{
		WCHAR szName[MAX_PATH];
		WCHAR szDrive[3] = TEXT(" :");
		WCHAR* p = szTemp;

		do
		{
			// Copy the drive letter to the template string
			*szDrive = *p;

			// Look up each device name
			if (QueryDosDeviceW(szDrive, szName, MAX_PATH))
			{
				size_t uNameLen = wcsnlen_s(szName, MAX_PATH);

				if (uNameLen < MAX_PATH)
				{
					// QueryDosDeviceW 함수로 획득한 \\Device 경로의 길이만큼만 비교
					bFound = _wcsnicmp(pszDeviceFileName, szName, uNameLen) == 0
						&& *(pszDeviceFileName + uNameLen) == TEXT('\\');

					if (bFound)
					{
						// Replace device path with DOS path
						StringCchPrintfW(pszWin32FileName,
							MAX_PATH,
							TEXT("%s%s"),
							szDrive,
							pszDeviceFileName + uNameLen);
					}
				}
			}
			// Go to the next NULL character.
			while (*p++);
		} while (!bFound && *p);
	}

	return(bFound);
}