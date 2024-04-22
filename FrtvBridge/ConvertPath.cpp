#include <strsafe.h>
#include "ConvertPath.h"

/// <summary>
/// C:\Users... ������ ��θ� \\Device\\HardDiskVolume0 �� ���� ����̽� ���� ��η� ��ȯ�ϴ� �Լ�
/// Ŀ�� ����̹��� ��θ� ������ �� ���
/// </summary>
/// <param name="pszDeviceFileName">Ŀ�ο��� ����ϴ� ����̽� ���� ��θ� �����ͷ� ��ȯ</param>
/// <param name="pszWin32FileName">����̺� ���� �ٴ� Win32 ���</param>
/// <returns>���� ���ο� ���� TRUE, FALSE�� ��ȯ</returns>
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
/// C:\Users... ������ ��θ� \\Device\\HardDiskVolume0 �� ���� ����̽� ���� ��η� ��ȯ�ϴ� �Լ�
/// Ŀ�� ����̹��� ��θ� ������ �� ���
/// </summary>
/// <param name="pszDeviceFileName">Ŀ�ο��� ����ϴ� ����̽� ���� ��θ� �����ͷ� ��ȯ</param>
/// <param name="pszWin32FileName">����̺� ���� �ٴ� Win32 ���</param>
/// <returns>���� ���ο� ���� TRUE, FALSE�� ��ȯ</returns>
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
/// \\Device\\HardDiskVolume0 ������ ��θ� C:\Users... �� ���� Win32 ��η� ��ȯ�ϴ� �Լ�
/// Ŀ�� ����̹��κ��� ���Ź��� ��θ� GUI�� ������ �� ���
/// </summary>
/// <param name="pszDeviceFileName">Ŀ�ο��� ����ϴ� ����̽� ���� ���</param>
/// <param name="pszWin32FileName">����̺� ���� �ٴ� Win32 ��θ� �����ͷ� ��ȯ</param>
/// <returns>���� ���ο� ���� TRUE, FALSE�� ��ȯ</returns>
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
					// QueryDosDeviceW �Լ��� ȹ���� \\Device ����� ���̸�ŭ�� ��
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