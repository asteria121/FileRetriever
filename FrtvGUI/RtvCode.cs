using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace FrtvGUI
{
    enum RTVCMDCODE
    {
        RTV_BACKUP_ALERT,
        RTVCMD_FILE_MANUAL_BACKUP,
        RTVCMD_FILE_RESTORE,
        RTVCMD_FILE_DELETE_NORMAL,
        RTVCMD_FILE_DELETE_1PASS,
        RTVCMD_FILE_DELETE_3PASS,
        RTVCMD_FILE_DELETE_7PASS,
        RTV_HEARTBEAT,
        RTVCMD_UPDATE_BACKUP_STORAGE,
        RTVCMD_EXCPATH_ADD,
        RTVCMD_EXCPATH_REMOVE,
        RTVCMD_EXTENSION_ADD,
        RTVCMD_EXTENSION_REMOVE,
        RTVCMD_BACKUP_ON,
        RTVCMD_BACKUP_OFF,
        RTV_DBG_MESSAGE
    }

    enum RTVCMDRESULT
    {
        RTVCMD_SUCCESS,
        RTVCMD_FAIL_INVALID_PARAMETER,
        RTVCMD_FAIL_OUT_OF_MEMORY,
        RTVCMD_FAIL_LONG_STRING
    }

    enum RTVCOMMRESULT
    {
        RTV_COMM_SUCCESS,
        RTV_COMM_OUT_OF_MEMORY
    };

    enum RTVSENDRESULT
    {
        RTV_SEND_SUCCESS,
        RTV_SEND_PORT_NULL
    };

}
