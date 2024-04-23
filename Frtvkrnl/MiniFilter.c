#include <fltKernel.h>
#include <ntstrsafe.h>
#include "MiniFilter.h"
#include "ManageFile.h"
#include "SettingsGeneral.h"
#include "Utility.h"

PFLT_FILTER fltHandle;

FLT_PREOP_CALLBACK_STATUS PreCreateCallback(
    _Inout_ PFLT_CALLBACK_DATA data,
    _In_    PCFLT_RELATED_OBJECTS fltObjects,
    _Out_   PVOID* completionContext
)
{
    UNREFERENCED_PARAMETER(completionContext);
    UNREFERENCED_PARAMETER(fltObjects);
    
    LPWSTR backupFolder = NULL;
    UNICODE_STRING destPath;    RtlZeroMemory(&destPath, sizeof(destPath));
    PFLT_FILE_NAME_INFORMATION fni = NULL;
    NTSTATUS status = STATUS_SUCCESS;
    FLT_PREOP_CALLBACK_STATUS cbStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;

    // 커널 모드에서 진행된 작업은 백업하지 않음
    if (data->RequestorMode == KernelMode)
        return cbStatus;

    // 파일 이름 구조체 생성
    status = FltGetFileNameInformation(data, FLT_FILE_NAME_OPENED | FLT_FILE_NAME_QUERY_DEFAULT, &fni);
    if (fni == NULL)
    {
        return cbStatus;
    }
    status = FltParseFileNameInformation(fni);
    if (!NT_SUCCESS(status))
    {
        FltReleaseFileNameInformation(fni);
        return cbStatus;
    }

    // 백업 폴더를 구한 후 백업 폴더의 경로로 시작하는지 확인
    backupFolder = GetBackupPath();
    RtlInitUnicodeString(&destPath, backupFolder);

    if (RtlPrefixUnicodeString(&destPath, &fni->Name, FALSE) && destPath.Length > 2)
    {
        DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[FileRtv] Preop delete prevent.\r\n");
        data->IoStatus.Status = STATUS_ACCESS_DENIED;
        cbStatus = FLT_PREOP_COMPLETE;
    }
    else
    {
        if (data->Iopb->MajorFunction == IRP_MJ_CREATE)
        {
            if ((data->Iopb->Parameters.Create.Options & FILE_DIRECTORY_FILE) == FILE_DIRECTORY_FILE)
                return FLT_PREOP_SUCCESS_NO_CALLBACK;

            if ((data->Iopb->Parameters.Create.Options >> 24) == FILE_OPEN)
            {
                if ((data->Iopb->Parameters.Create.Options & FILE_DELETE_ON_CLOSE) == FILE_DELETE_ON_CLOSE)
                {
                    DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[FileRtv] FILE_DELETE_ON_CLOSE: CODE: 0x%X %wZ\r\n", data->Iopb->Parameters.Create.Options, fni->Name);
                    BackupIfTarget(fni);
                }
            }
            else if ((data->Iopb->Parameters.Create.Options >> 24) == FILE_CREATE)
            {
                //DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[FileRtv] FILE_CREATE: CODE: 0x%X %wZ\r\n", data->Iopb->Parameters.Create.Options, fni->Name);
            }
            else if ((data->Iopb->Parameters.Create.Options >> 24) == FILE_OPEN_IF)
            {
                //DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[FileRtv] FILE_OPEN_IF: CODE: 0x%X %wZ\r\n", data->Iopb->Parameters.Create.Options, fni->Name);
            }
            else if ((data->Iopb->Parameters.Create.Options >> 24) == FILE_OVERWRITE)
            {
                DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[FileRtv] FILE_OVERWRITE: CODE: 0x%X %wZ\r\n", data->Iopb->Parameters.Create.Options, fni->Name);
                if (BackupIfTarget(fni) == TRUE)
                    BackupIfTarget(fni);
            }
            else if ((data->Iopb->Parameters.Create.Options >> 24) == FILE_OVERWRITE_IF)
            {
                DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[FileRtv] FILE_OVERWRITE_IF: CODE: 0x%X %wZ\r\n", data->Iopb->Parameters.Create.Options, fni->Name);
                if (BackupIfTarget(fni) == TRUE)
                    BackupIfTarget(fni);
            }
        }
    }

    FltReleaseFileNameInformation(fni);

    return cbStatus;
}

FLT_PREOP_CALLBACK_STATUS PreSetInformationCallback(
    _Inout_ PFLT_CALLBACK_DATA data,
    _In_    PCFLT_RELATED_OBJECTS fltObjects,
    _Out_   PVOID* completionContext
)
{
    UNREFERENCED_PARAMETER(completionContext);
    UNREFERENCED_PARAMETER(fltObjects);

    PFLT_FILE_NAME_INFORMATION fni = NULL;
    LPWSTR backupFolder = NULL;
    UNICODE_STRING destPath;    RtlZeroMemory(&destPath, sizeof(destPath));
    NTSTATUS status = STATUS_SUCCESS;
    FLT_PREOP_CALLBACK_STATUS cbStatus = FLT_PREOP_SUCCESS_NO_CALLBACK;

    // 커널 모드에서 진행된 작업은 백업하지 않음
    if (data->RequestorMode == KernelMode)
        return cbStatus;

    // 파일 이름 구조체 생성
    status = FltGetFileNameInformation(data, FLT_FILE_NAME_OPENED | FLT_FILE_NAME_QUERY_DEFAULT, &fni);
    if (fni == NULL)
    {
        return cbStatus;
    }

    status = FltParseFileNameInformation(fni);
    if (!NT_SUCCESS(status))
    {
        FltReleaseFileNameInformation(fni);
        return cbStatus;
    }

    // 백업 폴더를 구한 후 백업 폴더의 경로로 시작하는지 확인
    backupFolder = GetBackupPath();
    RtlInitUnicodeString(&destPath, backupFolder);

    if (RtlPrefixUnicodeString(&destPath, &fni->Name, FALSE) && destPath.Length > 2)
    {
        DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[FileRtv] Preop delete prevent.\r\n");
        data->IoStatus.Status = STATUS_ACCESS_DENIED;
        cbStatus = FLT_PREOP_COMPLETE;
    }
    else
    {
        if (data->Iopb->Parameters.SetFileInformation.FileInformationClass == FileDispositionInformation)
        {
            if (((PFILE_DISPOSITION_INFORMATION)data->Iopb->Parameters.SetFileInformation.InfoBuffer)->DeleteFile)
            {
                DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[FileRtv] DELETEFILE: %wZ\r\n", fni->Name);
                BackupIfTarget(fni);
            }
        }
        else if (data->Iopb->Parameters.SetFileInformation.FileInformationClass == FileDispositionInformationEx)
        {
            if (((PFILE_DISPOSITION_INFORMATION_EX)data->Iopb->Parameters.SetFileInformation.InfoBuffer)->Flags & FILE_DISPOSITION_DELETE)
            {
                DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[FileRtv] FILE_DISPOSITION_DELETE: %wZ\r\n", fni->Name);
                BackupIfTarget(fni);
            }
        }
    }

    FltReleaseFileNameInformation(fni);
    return cbStatus;
}


FLT_POSTOP_CALLBACK_STATUS MinifltExampleCreatePostRoutine(
    _Inout_      PFLT_CALLBACK_DATA data,
    _In_         PCFLT_RELATED_OBJECTS fltObjects,
    _In_opt_     PVOID completionContext,
    _In_         FLT_POST_OPERATION_FLAGS flags
)
{
    UNREFERENCED_PARAMETER(data);
    UNREFERENCED_PARAMETER(completionContext);
    UNREFERENCED_PARAMETER(flags);

    if (fltObjects && fltObjects->FileObject && fltObjects->FileObject->FileName.Buffer)
    {
        
    }

    return FLT_POSTOP_FINISHED_PROCESSING;
}

NTSTATUS MinifltExampleFilterUnloadRoutine(
    _In_ FLT_FILTER_UNLOAD_FLAGS flags
)
{
    UNREFERENCED_PARAMETER(flags);
    if (fltHandle)
    {
        FltUnregisterFilter(fltHandle);
        DBGPRT(DPFLTR_IHVDRIVER_ID, 0, "[FileRtv] Minifilter Unloading..\r\n");
    }

    return STATUS_SUCCESS;
}



FLT_OPERATION_REGISTRATION operations[] = {
    {
        IRP_MJ_CREATE,
        0,
        PreCreateCallback,
        MinifltExampleCreatePostRoutine,
        NULL
    },
    {
        IRP_MJ_SET_INFORMATION,
        0,
        PreSetInformationCallback,
        MinifltExampleCreatePostRoutine,
        NULL
    },
    {
        IRP_MJ_OPERATION_END
    }
};

const FLT_REGISTRATION registration =
{
    sizeof(FLT_REGISTRATION),          // size
    FLT_REGISTRATION_VERSION,          // version
    0,                                 // flags
    NULL,                              // context registration
    operations,                        // operation registration
    MinifltExampleFilterUnloadRoutine, // filter unload callback
    NULL,                              // instance setup callback
    NULL,                              // instance query teardown callback
    NULL,                              // instance teardown start callback
    NULL,                              // instance teardown complete callback
    NULL,                              // generate file name callback
    NULL,                              // normalize name component callback
    NULL,                              // normalize context cleanup callback
    NULL,                              // transaction notification callback
    NULL,                              // normalize name component ex callback
    NULL                               // section notification callback
};

NTSTATUS StartMiniFilter(
    _In_ PDRIVER_OBJECT driverObject,
    _Out_ PFLT_FILTER* filterHandle
)
{
    NTSTATUS status = FltRegisterFilter(driverObject, &registration, &fltHandle);
    status = FltStartFiltering(fltHandle);
    *filterHandle = fltHandle;

    return status;
}

VOID StopMiniFilter()
{
    if (fltHandle)
    {
        FltUnregisterFilter(fltHandle);
    }
}