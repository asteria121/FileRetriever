using System.Runtime.InteropServices;
using System.Threading;

namespace FrtvGUI
{
    public static class BridgeFunctions
    {
        // DLL에 콜백 함수 전달을 위한 콜백 함수 대리자
        public delegate void DebugCallback(uint logLevel, string message);
        public delegate void DBCallback(string fileName, long fileSize, uint crc32);
        public delegate void ConnectCallback();
        public delegate void DisconnectCallback();

        [DllImport("FrtvBridge.dll")]
        public static extern int SendMinifltPortA(int rtvCode, string msg, uint crc32, long fileSize);

        [DllImport("FrtvBridge.dll")]
        public static extern int InitializeCommunicator();
        [DllImport("FrtvBridge.dll")]
        public static extern int RegisterCallbacks(DebugCallback dbgcb, DBCallback dcb, ConnectCallback ccb, DisconnectCallback dccb);

        [DllImport("FrtvBridge.dll")]
        public static extern int UpdateBackupPath(string path);
        [DllImport("FrtvBridge.dll")]
        public static extern int AddExceptionPath(string path);
        [DllImport("FrtvBridge.dll")]
        public static extern int RemoveExceptionPath(string path);
        [DllImport("FrtvBridge.dll")]
        public static extern int ToggleBackupSwitch(int enabled);
        [DllImport("FrtvBridge.dll")]
        public static extern int UpdateBackupFolder(string folder);
        [DllImport("FrtvBridge.dll")]
        public static extern int RestoreBackupFile(string dstPath, uint crc32, bool overwriteDst);
        [DllImport("FrtvBridge.dll")]
        public static extern int DeleteBackupFile(uint crc32);
        [DllImport("FrtvBridge.dll")]
        public static extern int AddExtension(string extension, long maximumFileSize);
        [DllImport("FrtvBridge.dll")]
        public static extern int RemoveExtension(string extension);
        [DllImport("FrtvBridge.dll")]
        public static extern int Test(int count);
    }
}
