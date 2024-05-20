using FrtvGUI.Database;
using FrtvGUI.Elements;
using FrtvGUI.Views;
using System.IO;
using System.Runtime.InteropServices;
using System.Threading;
using System.Windows.Threading;
using System.Windows;
using FrtvGUI.Enums;

namespace FrtvGUI
{
    public static class BridgeFunctions
    {
        // DLL에 콜백 함수 전달을 위한 콜백 함수 대리자
        public delegate void DebugCallback(uint logLevel, string message);
        public delegate void DBCallback(string fileName, long fileSize, uint crc32);
        public delegate void ConnectCallback();
        public delegate void DisconnectCallback();
        public static CancellationTokenSource TokenSource = new CancellationTokenSource();

        // 디버그 메세지를 출력하는 콜백 함수
        public static void DebugCallbackFunction(uint logLevel, string message)
        {
            try
            {
                var log = new Log(DateTime.Now, logLevel, message);
                log.AddAsync().GetAwaiter();
            }
            catch (Exception e)
            {
                System.Windows.MessageBox.Show(e.Message);
            }

        }

        public static void DBCallbackFunction(string fileName, long fileSize, uint crc32)
        {
            try
            {
                var extension = System.IO.Path.GetExtension(fileName);
                var fi = new FileInfo(fileName);

                var targetExt = BackupExtension.GetInstance().Where(x => string.Equals(extension.Replace(".", string.Empty), x.Extension)).FirstOrDefault();
                if (targetExt != null)
                {
                    var file = new BackupFile(crc32, fileName, fileSize, DateTime.Now, DateTime.Now + targetExt.Expiration);
                    Task.Run(file.AddAsync);
                }
                // 확장자 설정이 존재하지 않는 경우 (백업 대상 폴더 목록인 경우)
                else
                {
                    var targetPath = IncludePath.GetInstance().Where(x => fileName.StartsWith(x.Path)).FirstOrDefault();
                    if (targetPath != null)
                    {
                        var file = new BackupFile(crc32, fileName, fileSize, DateTime.Now, DateTime.Now + targetPath.Expiration);
                        Task.Run(file.AddAsync);
                    }
                }
            }
            catch (Exception e)
            {
                System.Windows.MessageBox.Show(e.Message);
            }
        }

        // FrtvBridge.dll에서 연결 / 재연결 성공 시 각 1회 호출되는 콜백 함수
        public static void ConnectCallbackFunction()
        {
            try
            {
                string backupPath = Settings.GetBackupPath();
                int isBackupEnabled = 0;
                int hr = 0;
                if (Settings.GetBackupEnabled() == true)
                    isBackupEnabled = 1;
                
                int res;
                res = BridgeFunctions.ToggleBackupSwitch(isBackupEnabled, out hr);
                if (res != 0 && hr != 0)
                {
                    System.Windows.MessageBox.Show($"드라이버와 통신에 실패했습니다.\r\nHRESULT: 0x{hr:X8}", "FileRetriever", MessageBoxButton.OK, MessageBoxImage.Error);
                    Log log = new Log(DateTime.Now, (uint)FrtvLogLevel.FrtvError, $"드라이버 통신 실패 (HRESULT: 0x{hr:X8})");
                    log.AddAsync().GetAwaiter();
                }
                else if (hr == 0 && res != 0)
                {
                    Settings.SetBackupPath(string.Empty);
                    System.Windows.MessageBox.Show($"알 수 없는 이유로 실시간 백업 설정에 실패했습니다.\r\nErrorCode: {res}", "FileRetriever", MessageBoxButton.OK, MessageBoxImage.Error);
                    Log log = new Log(DateTime.Now, (uint)FrtvLogLevel.FrtvError, $"실시간 백업 초기 설정 실패 (Error Code: {res})");
                    log.AddAsync().GetAwaiter();
                }

                res = BridgeFunctions.UpdateBackupFolder(backupPath, out hr);
                if (res != 0 && hr != 0)
                {
                    System.Windows.MessageBox.Show($"드라이버와 통신에 실패했습니다.\r\nHRESULT: 0x{hr:X8}", "FileRetriever", MessageBoxButton.OK, MessageBoxImage.Error);
                    Log log = new Log(DateTime.Now, (uint)FrtvLogLevel.FrtvError, $"드라이버 통신 실패 (HRESULT: 0x{hr:X8})");
                    log.AddAsync().GetAwaiter();
                }
                else if (hr == 0 && res != 0)
                {
                    Settings.SetBackupPath(string.Empty);
                    System.Windows.MessageBox.Show($"백업 폴더 설정에 실패했습니다.\r\n백업 폴더를 다시 지정해주세요.\r\n{backupPath}", "FileRetriever", MessageBoxButton.OK, MessageBoxImage.Error);
                    Log log = new Log(DateTime.Now, (uint)FrtvLogLevel.FrtvError, $"백업 폴더 초기 설정 실패: {backupPath} (Error Code: {res})");
                    log.AddAsync().GetAwaiter();
                }

                // 재연결 시 모든 데이터를 초기화 후 다시 등록한다
                if (Thread.CurrentThread == MainWindow.Wnd.Dispatcher.Thread) // UI THREAD
                {
                    BackupFile.GetInstance().Clear();
                    BackupExtension.GetInstance().Clear();
                    ExceptionPath.GetInstance().Clear();
                }
                else // OTHER THREAD
                {
                    MainWindow.Wnd.Dispatcher.Invoke(DispatcherPriority.Normal, new Action(delegate
                    {
                        BackupFile.GetInstance().Clear();
                        BackupExtension.GetInstance().Clear();
                        ExceptionPath.GetInstance().Clear();
                    }));
                }

                // DB 로드
                RtvDB.InitializeDatabaseAsync().GetAwaiter();
                BackupFile.LoadDatabaseAsync().GetAwaiter();
                BackupExtension.LoadDatabaseAsync().GetAwaiter();
                ExceptionPath.LoadDatabaseAsync().GetAwaiter();
                IncludePath.LoadDatabaseAsync().GetAwaiter();

                // 유효기간 만료 데이터베이스 자동 제거 쓰레드
                // 연결 해제 시 Disconnect 콜백 함수가 CancellationToken에 취소 신호를 보냄
                Task.Run(async () => await ExpirationWatchdog.InitializeWatchdog(TokenSource.Token));

                // DB와 레지스트리에서 취득한 설정을 UI에 업데이트한다.
                SettingsView.UpdateBackupSettingsUI();
                SettingsView.UpdateBackupPathUI();
            }
            catch (Exception ex)
            {
                System.Windows.MessageBox.Show(ex.Message);
            }
        }

        // 커널 드라이버 연결 해제 시 호출되는 콜백 함수
        public static void DisconnectCallbackFunction()
        {
            // 커널 드라이버가 작동중이지 않기 때문에 유효기간 만료 체크 또한 진행하지 않는다.
            TokenSource.Cancel();
        }

        [DllImport("FrtvBridge.dll")]
        public static extern int InitializeCommunicator();
        [DllImport("FrtvBridge.dll")]
        public static extern int RegisterCallbacks(DebugCallback dbgcb, DBCallback dcb, ConnectCallback ccb, DisconnectCallback dccb);

        [DllImport("FrtvBridge.dll")]
        public static extern int AddExceptionPath(string path, out int hr);
        [DllImport("FrtvBridge.dll")]
        public static extern int RemoveExceptionPath(string path, out int hr);
        [DllImport("FrtvBridge.dll")]
        public static extern int AddIncludePath(string path, long maximumSize, out int hr);
        [DllImport("FrtvBridge.dll")]
        public static extern int RemoveIncludePath(string path, out int hr);
        [DllImport("FrtvBridge.dll")]
        public static extern int AddExtension(string extension, long maximumFileSize, out int hr);
        [DllImport("FrtvBridge.dll")]
        public static extern int RemoveExtension(string extension, out int hr);
        [DllImport("FrtvBridge.dll")]
        public static extern int ToggleBackupSwitch(int enabled, out int hr);
        [DllImport("FrtvBridge.dll")]
        public static extern int UpdateBackupFolder(string folder, out int hr);
        [DllImport("FrtvBridge.dll")]
        public static extern int RestoreBackupFile(string dstPath, uint crc32, bool overwriteDst, out int hr);
        [DllImport("FrtvBridge.dll")]
        public static extern int DeleteBackupFile(uint crc32, out int hr);
    }
}
