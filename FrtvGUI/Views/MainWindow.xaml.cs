using Microsoft.Win32;
using System.Collections.ObjectModel;
using System.IO;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using MahApps.Metro.Controls;
using System.ComponentModel;
using FrtvGUI.Elements;
using Microsoft.WindowsAPICodePack.Dialogs;
using System.Runtime.InteropServices;
using System;
using System.Data.SQLite;
using System.Runtime.CompilerServices;
using System.Reflection;
using System.Windows.Threading;
using System.Xml;
using MahApps.Metro.Controls.Dialogs;
using FrtvGUI.Database;
using System.Runtime.Intrinsics.Arm;


namespace FrtvGUI.Views
{
    public partial class MainWindow
    {
        private static CancellationTokenSource TokenSource = new CancellationTokenSource();
        private static MetroDialogSettings _DialogSettings;
        public static MetroDialogSettings DialogSettings
        {
            get { return _DialogSettings; }
            set { _DialogSettings = value; }
        }

        // static 함수 및 분리된 클래스에서 MainWindow 객체에 접근하기 위한 멤버 변수
        private static MainWindow _Wnd;
        public static MainWindow Wnd
        {
            get { return _Wnd; }
            private set { _Wnd = value; }
        }

        private static NotifyIcon TrayIcon;
        private static ContextMenuStrip TrayMenu;
        private static ToolStripMenuItem _OpenMenu;
        public static ToolStripMenuItem OpenMenu
        {
            get { return _OpenMenu; }
            private set { _OpenMenu = value; }
        }
        private static ToolStripMenuItem _ToggleMenu;
        public static ToolStripMenuItem ToggleMenu
        {
            get { return _ToggleMenu; }
            private set { _ToggleMenu = value; }
        }
        private static ToolStripMenuItem _ExitMenu;
        public static ToolStripMenuItem ExitMenu
        {
            get { return _ExitMenu; }
            private set { _ExitMenu = value; }
        }
        public static bool ToggleMenuProgramChanged = false;

        List<BackupFile> lists;


        // 디버그 메세지를 출력하는 콜백 함수
        private static void DebugCallbackFunction(uint logLevel, string message)
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


        // TODO: DB 쿼리 결과를 커널 드라이버에 전달하도록 한다.
        private static void DBCallbackFunction(string fileName, uint crc32)
        {
            try
            {
                var extension = System.IO.Path.GetExtension(fileName);
                var fi = new FileInfo(fileName);
                
                var targetExt = BackupExtension.GetInstance().Where(x => string.Equals(extension.Replace(".", string.Empty), x.Extension)).FirstOrDefault();
                if (targetExt != null)
                {
                    var file = new BackupFile(crc32, fileName, 1, DateTime.Now, DateTime.Now + targetExt.Expiration);
                    Task.Run(file.AddAsync);
                }
            }
            catch (Exception e)
            {
                System.Windows.MessageBox.Show(e.Message);
            }
        }

        // FrtvBridge.dll에서 연결 / 재연결 성공 시 각 1회 호출되는 콜백 함수
        private static void ConnectCallbackFunction()
        {
            try
            {
                string backupPath = Settings.GetBackupPath();
                int isBackupEnabled = 0;
                if (Settings.GetBackupEnabled() == true)
                    isBackupEnabled = 1;

                // TODO 설정 손상시 로그 또는 알림 추가하기
                int res;
                res = BridgeFunctions.ToggleBackupSwitch(isBackupEnabled);
                if (res != 0)
                {
                    
                }

                res = BridgeFunctions.UpdateBackupFolder(backupPath);
                if (res != 0)
                {
                    Settings.SetBackupPath(string.Empty);
                    System.Windows.MessageBox.Show($"백업 폴더 설정에 실패했습니다.\r\n백업 폴더를 다시 지정해주세요.\r\n{backupPath}", "FileRetriever", MessageBoxButton.OK, MessageBoxImage.Error);
                }

                // 재연결 시 모든 데이터를 초기화 후 다시 등록한다
                if (Thread.CurrentThread == Wnd.Dispatcher.Thread) // UI THREAD
                {
                    BackupFile.GetInstance().Clear();
                    BackupExtension.GetInstance().Clear();
                    ExceptionPath.GetInstance().Clear();
                }
                else // OTHER THREAD
                {
                    Wnd.Dispatcher.Invoke(DispatcherPriority.Normal, new Action(delegate
                    {
                        BackupFile.GetInstance().Clear();
                        BackupExtension.GetInstance().Clear();
                        ExceptionPath.GetInstance().Clear();
                    }));
                }
                
                RtvDB.InitializeDatabaseAsync().GetAwaiter();
                BackupFile.LoadDatabaseAsync().GetAwaiter();
                BackupExtension.LoadDatabaseAsync().GetAwaiter();
                ExceptionPath.LoadDatabaseAsync().GetAwaiter();

                // 유효기간 만료 데이터베이스 자동 제거 쓰레드
                // 연결 해제 시 Disconnect 콜백 함수가 CancellationToken에 취소 신호를 보냄
                Task.Run(async() => await ExpirationWatchdog.InitializeWatchdog(TokenSource.Token));

                // DB와 레지스트리에서 취득한 설정을 UI에 업데이트한다.
                SettingsView.UpdateBackupSettingsUI();
                SettingsView.UpdateBackupPathUI();
                SettingsView.UpdateExtensionListUI();
                SettingsView.UpdateExceptionPathListUI();
            }
            catch (Exception ex)
            {
                System.Windows.MessageBox.Show(ex.Message);
            }
        }

        // 커널 드라이버 연결 해제 시 호출되는 콜백 함수
        private static void DisconnectCallbackFunction()
        {
            // 커널 드라이버가 작동중이지 않기 때문에 유효기간 만료 체크 또한 진행하지 않는다.
            TokenSource.Cancel();
        }

        // 파일 목록을 새로고침하는 함수
        public static void UpdateFileListUI()
        {
            if (Thread.CurrentThread == Wnd.Dispatcher.Thread) // UI THREAD
            {
                Wnd.fileDataGrid.ItemsSource ??= BackupFile.GetInstance();
                Wnd.fileDataGrid.Items.Refresh();
            }
            else // OTHER THREAD
            {
                Wnd.Dispatcher.Invoke(DispatcherPriority.Normal, new Action(delegate
                {
                    Wnd.fileDataGrid.ItemsSource ??= BackupFile.GetInstance();
                    Wnd.fileDataGrid.Items.Refresh();
                }));
            }
        }

        // Bar 형식의 메세지를 출력하는 함수
        public static void ShowAppBar(string message, long autoCloseInterval = 3000, bool isAutoClose = true)
        {
            // TODO: 메세지 색상을 지정할 수 있도록 만들 예정
            var flyout = new AppBarFlyout(message, System.Windows.Media.Brushes.Red);
            flyout.AutoCloseInterval = autoCloseInterval;
            flyout.IsAutoCloseEnabled = isAutoClose;

            RoutedEventHandler? closingFinishedHandler = null;
            closingFinishedHandler = (o, args) =>
            {
                flyout.ClosingFinished -= closingFinishedHandler;
                ((MainWindow)System.Windows.Application.Current.MainWindow).flyoutsControl.Items.Remove(flyout);
            };

            flyout.ClosingFinished += closingFinishedHandler;
            ((MainWindow)System.Windows.Application.Current.MainWindow).flyoutsControl.Items.Add(flyout);
            flyout.IsOpen = true;
        }

        public MainWindow()
        {
            // 타 클래스 및 static 함수에서 참조할 수 있는 MainWindow 객체 할당
            Wnd = ((MainWindow)System.Windows.Application.Current.MainWindow);

            // 메세지박스 다이얼로그 기본값 설정
            DialogSettings = new MetroDialogSettings();
            DialogSettings.AnimateHide = false;
            DialogSettings.AnimateShow = false;
            DialogSettings.ColorScheme = this.MetroDialogOptions.ColorScheme;
            DialogSettings.OwnerCanCloseWithDialog = true;

            // MainWindow 객체 할당 후 InitializeComponent() 함수를 호출해야함
            InitializeComponent();

            // 커널 드라이버 서비스 시작
            App.LoadKernelDriver();

            // 콜백함수를 DLL에 등록한다
            // 유효기간 만료 확인은 ConnectCallback에서 호출 후 연결이 끊길 경우 DisconnectCallback에서 CancellationToken으로 종료시킴.
            BridgeFunctions.RegisterCallbacks(DebugCallbackFunction, DBCallbackFunction, ConnectCallbackFunction, DisconnectCallbackFunction);

            // DLL이 드라이버와 통신하는 스레드를 생성한다
            Task task = Task.Run(() =>
            {
                // 에러가 발생하지 않는 한 아래 함수는 무한 루프 내에서 작동한다.
                int exitCode = BridgeFunctions.InitializeCommunicator();

                if (exitCode == (int)RTVCOMMRESULT.RTV_COMM_OUT_OF_MEMORY)
                {
                    System.Windows.MessageBox.Show("메모리 할당에 실패했습니다.\r\n프로그램을 종료합니다.", "FileRetriever", MessageBoxButton.OK, MessageBoxImage.Error);
                }
                else
                {
                    System.Windows.MessageBox.Show($"IOCP FilterGetMessage() 오류 발생.\r\n프로그램을 종료합니다.\r\nHRESULT: {exitCode}", "FileRetriever", MessageBoxButton.OK, MessageBoxImage.Error);
                }

                System.Windows.Application.Current.Shutdown();
            });

            fileDataGrid.ItemsSource ??= BackupFile.GetInstance();

            // 시스템 트레이 아이콘 생성
            CreateTrayIcon();
        }

        private void CreateTrayIcon()
        {
            // 프로그램 열기 메뉴
            OpenMenu = new ToolStripMenuItem("열기");
            OpenMenu.Click += delegate (object? o, EventArgs e)
            {
                this.Show();
                this.WindowState = WindowState.Normal;
            };

            // 실시간 백업 메뉴
            ToggleMenu = new ToolStripMenuItem("실시간 백업");
            ToggleMenu.CheckOnClick = true;
            ToggleMenu.CheckedChanged += delegate (object? o, EventArgs e)
            {
                // 프로그램이 상태를 변경해도 이벤트가 호출되지 않도록 함
                try
                {
                    if (ToggleMenuProgramChanged == false)
                    {
                        if (ToggleMenu.Checked == true)
                        {
                            int result = BridgeFunctions.ToggleBackupSwitch(1);
                            if (result == 0)
                            {
                                // 성공 시 레지스트리와 모든 UI 설정을 동기화한다.
                                Settings.SetBackupEnabled(true);
                            }
                            else
                            {
                                System.Windows.MessageBox.Show($"실시간 백업 설정 변경에 실패했습니다.\r\nError Code: {result}", "FileRetriever", MessageBoxButton.OK, MessageBoxImage.Error);
                            }

                        }
                        else
                        {
                            var msgResult = System.Windows.MessageBox.Show("실시간 백업이 중단됩니다.\r\n계속 하시겠습니까?", "FileRetriever", MessageBoxButton.YesNo, MessageBoxImage.Warning);
                            if (msgResult == MessageBoxResult.Yes)
                            {
                                int result = BridgeFunctions.ToggleBackupSwitch(0);
                                if (result == 0)
                                {
                                    // 성공 시 레지스트리와 모든 UI 설정을 동기화한다.
                                    Settings.SetBackupEnabled(false);
                                }
                                else
                                {
                                    System.Windows.MessageBox.Show($"실시간 백업 설정 변경에 실패했습니다.\r\nError Code: {result}", "FileRetriever", MessageBoxButton.OK, MessageBoxImage.Error);
                                }
                            }
                        }

                        // 최종 레지스트리 값으로 UI와 우클릭 메뉴를 동기화한다.
                        SettingsView.UpdateBackupSettingsUI();
                    }
                }
                catch (Exception ex)
                {

                }
            };

            // 종료 메뉴
            ExitMenu = new ToolStripMenuItem("프로그램 종료");
            ExitMenu.Click += delegate (object? o, EventArgs e)
            {
                var msgResult = System.Windows.MessageBox.Show("정말 프로그램을 종료하시겠습니까?", "FileRetriever", MessageBoxButton.YesNo, MessageBoxImage.Question);
                if (msgResult == MessageBoxResult.Yes)
                {
                    TrayIcon.Dispose();         // 트레이 아이콘 제거
                    App.UnloadKernelDriver();   // 드라이버 언로드
                    System.Windows.Application.Current.Shutdown();
                }
            };

            TrayMenu = new ContextMenuStrip();
            TrayMenu.Items.Add(OpenMenu);
            TrayMenu.Items.Add(ToggleMenu);
            TrayMenu.Items.Add(ExitMenu);

            // 트레이 아이콘
            TrayIcon = new NotifyIcon();
            TrayIcon.Icon = System.Drawing.Icon.ExtractAssociatedIcon(System.Windows.Forms.Application.ExecutablePath);
            TrayIcon.Visible = true;
            TrayIcon.Text = "FileRetriever";
            TrayIcon.ContextMenuStrip = TrayMenu;
            TrayIcon.DoubleClick +=
            delegate (object? sender, EventArgs args)
            {
                this.Show();
                this.WindowState = WindowState.Normal;
            };
        }

        // 파일 백업
        private void AddFile_Click(object sender, RoutedEventArgs e)
        {
            if (fileDataGrid.ItemsSource == null)
            {
                fileDataGrid.ItemsSource = BackupFile.GetInstance();
            }
            else
            {
                fileDataGrid.ItemsSource = null;
            }
                
            /*
            OpenFileDialog openFileDialog = new OpenFileDialog();
            if (openFileDialog.ShowDialog() == true)
            {
                FileItem newItem = new FileItem
                {
                    FileName = System.IO.Path.GetFileName(openFileDialog.FileName),
                    FilePath = openFileDialog.FileName
                };
                newItem.UpdateFileSize(); // 파일 크기 업데이트
                fileItems.Add(newItem);
            }*/
        }

        // 파일 삭제
        private async void RemoveFile_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                // 파일 제거 버튼 클릭 이벤트 처리
                foreach (BackupFile item in fileDataGrid.SelectedItems)
                {
                    int result = BridgeFunctions.DeleteBackupFile(item.Crc32);
                    if (result == 0)
                        await item.RemoveAsync();
                }
            }
            catch (Exception ex)
            {
                System.Windows.MessageBox.Show(ex.Message + Environment.NewLine + ex.StackTrace);
            }
        }

        private async void RestoreButton_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                // 파일 제거 버튼 클릭 이벤트 처리
                foreach (BackupFile item in fileDataGrid.SelectedItems)
                {
                    int result = BridgeFunctions.RestoreBackupFile(item.OriginalPath, item.Crc32);
                    if (result == 0)
                        await item.RemoveAsync();
                    else
                        System.Windows.MessageBox.Show(result.ToString());
                }
            }
            catch (Exception ex)
            {

            }
        }

        private void OpenInformationWindowButton_Click(object sender, RoutedEventArgs e)
        {
            InformationWindow wnd = new InformationWindow();
            wnd.ShowDialog();
        }

        private void OpenLogWindowButton_Click(object sender, RoutedEventArgs e)
        {
            LogWindow wnd = new LogWindow();
            wnd.ShowDialog();
        }

        private void MainWindow_StateChanged(object sender, EventArgs e)
        {
            if (WindowState == WindowState.Minimized)
            {
                this.Hide();
            }
        }

        private async void MainWindow_Closing(object sender, CancelEventArgs e)
        {
            e.Cancel = true;
            await this.ShowMessageAsync("알림", "프로그램을 종료하려면 시스템 트레이 우클릭 메뉴에서 종료하실 수 있습니다.", settings: DialogSettings);
            this.Hide();
        }
    }
}
