using ControlzEx.Standard;
using FrtvGUI.Database;
using FrtvGUI.Elements;
using FrtvGUI.Enums;
using MahApps.Metro.Controls;
using MahApps.Metro.Controls.Dialogs;
using Microsoft.WindowsAPICodePack.Dialogs;
using System;
using System.IO;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Shapes;
using System.Windows.Threading;

namespace FrtvGUI.Views
{
    /// <summary>
    /// SettingsView.xaml에 대한 상호 작용 논리
    /// </summary>
    public partial class SettingsView : System.Windows.Controls.UserControl
    {
        private static ToggleSwitch _BackupToggleSwtch;
        public static ToggleSwitch BackupToggleSwtch
        {
            get { return _BackupToggleSwtch; }
            private set { _BackupToggleSwtch = value; }
        }
        private static System.Windows.Controls.TextBox _BackupPathTextBox;
        private static string BackupFolderName = "FrtvBackup";
        public static bool ToggleMenuProgramChanged = false;

        public SettingsView()
        {
            InitializeComponent();
            _BackupToggleSwtch = BackupToggleSwitch;
            _BackupPathTextBox = BackupPathTextBox;

            ExtensionDataGrid.ItemsSource ??= BackupExtension.GetInstance();
            ExceptionPathDataGrid.ItemsSource ??= ExceptionPath.GetInstance();
            IncludePathDataGrid.ItemsSource ??= IncludePath.GetInstance();
        }

        public static void UpdateBackupSettingsUI()
        {
            // CheckedChanged 이벤트 발생 거부
            SettingsView.ToggleMenuProgramChanged = true;
            MainWindow.ToggleMenuProgramChanged = true;
            bool isOn = Settings.GetBackupEnabled();

            if (Thread.CurrentThread == MainWindow.Wnd.Dispatcher.Thread) // UI THREAD
            {
                BackupToggleSwtch.IsOn = isOn;
                MainWindow.ToggleMenu.Checked = isOn;
            }
            else
            {
                MainWindow.Wnd.Dispatcher.Invoke(DispatcherPriority.Normal, new Action(delegate
                {
                    BackupToggleSwtch.IsOn = isOn;
                    MainWindow.ToggleMenu.Checked = isOn;
                }));
            }

            // CheckedChanged 이벤트 발생 허용
            SettingsView.ToggleMenuProgramChanged = false;
            MainWindow.ToggleMenuProgramChanged = false;
        }

        public static void UpdateBackupPathUI()
        {
            if (Thread.CurrentThread == MainWindow.Wnd.Dispatcher.Thread) // UI THREAD
            {
                BackupToggleSwtch.IsOn = Settings.GetBackupEnabled();
                _BackupPathTextBox.Text = System.IO.Path.Combine(Settings.GetBackupPath(), BackupFolderName); // 말단 폴더는 FrtvBackup으로 고정됨
            }
            else
            {
                MainWindow.Wnd.Dispatcher.Invoke(DispatcherPriority.Normal, new Action(delegate
                {
                    BackupToggleSwtch.IsOn = Settings.GetBackupEnabled();
                    _BackupPathTextBox.Text = System.IO.Path.Combine(Settings.GetBackupPath(), BackupFolderName); // 말단 폴더는 FrtvBackup으로 고정됨
                }));
            }
        }

        private async void BackupToggleSwitch_Toggled(object sender, RoutedEventArgs e)
        {
            try
            {
                // 프로그램이 상태를 변경해도 이벤트가 호출되지 않도록 함
                if (ToggleMenuProgramChanged == false)
                {
                    int hr = 0, result = 0;
                    if (BackupToggleSwitch.IsOn == true)
                    {
                        result = BridgeFunctions.ToggleBackupSwitch(1, out hr);
                        if (result == 0 && hr == 0)
                        {
                            // 성공 시 레지스트리와 모든 UI 설정을 동기화한다.
                            Settings.SetBackupEnabled(true);
                            Log log = new Log(DateTime.Now, (uint)FrtvLogLevel.FrtvError, $"실시간 백업 설정 변경: ON");
                            log.AddAsync().GetAwaiter();
                        }
                    }
                    else
                    {
                        // 실시간 백업 OFF는 사용자에게 확인하는 절차를 갖는다.
                        var messageDialogResult = await MainWindow.Wnd.ShowMessageAsync("Info", $"실시간 백업 종료시 자동으로 백업되지 않습니다.\r\n계속 하시겠습니까?", MessageDialogStyle.AffirmativeAndNegative, settings: MainWindow.DialogSettings);
                        if (messageDialogResult == MessageDialogResult.Affirmative)
                        {
                            result = BridgeFunctions.ToggleBackupSwitch(0, out hr);
                            if (result == 0 && hr == 0)
                            {
                                // 성공 시 레지스트리와 모든 UI 설정을 동기화한다.
                                Settings.SetBackupEnabled(false);
                                Log log = new Log(DateTime.Now, (uint)FrtvLogLevel.FrtvError, $"실시간 백업 설정 변경: OFF");
                                log.AddAsync().GetAwaiter();
                            }
                        }
                    }

                    if (hr != 0)
                    {
                        await MainWindow.ShowHresultError(hr);
                        Log log = new Log(DateTime.Now, (uint)FrtvLogLevel.FrtvError, $"드라이버 통신 실패 (HRESULT: 0x{hr:X8})");
                        log.AddAsync().GetAwaiter();
                    }
                    else if (result != 0)
                    {
                        await MainWindow.Wnd.ShowMessageAsync("Error", $"실시간 백업 설정 변경에 실패했습니다.\r\nError Code: {result}", settings: MainWindow.DialogSettings);
                        Log log = new Log(DateTime.Now, (uint)FrtvLogLevel.FrtvError, $"실시간 백업 설정 변경 실패 (Error Code: {result})");
                        log.AddAsync().GetAwaiter();
                    }

                    // 최종 레지스트리 값으로 UI와 우클릭 메뉴를 동기화한다.
                    UpdateBackupSettingsUI();
                }
            }
            catch (Exception ex)
            {
                await MainWindow.Wnd.ShowMessageAsync("Error", $"백업 설정 변경 중 예외가 발생했습니다.\r\n{ex.GetType().Name}: {ex.Message}", settings: MainWindow.DialogSettings);
                await Log.PrintExceptionLogFileAsync(ex);
            }
        }

        private async void BackupPathBrowseButton_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                var dialog = new CommonOpenFileDialog();
                dialog.IsFolderPicker = true;

                if (dialog.ShowDialog() == CommonFileDialogResult.Ok)
                {
                    // TODO: 파일이 존재할 경우 백업 저장소 파일을 옮길지 물어보기 (옮겨야만함)
                    int hr = 0;
                    int result = BridgeFunctions.UpdateBackupFolder(dialog.FileName, out hr);
                    if (result == 0 && hr == 0)
                    {
                        Settings.SetBackupPath(dialog.FileName);
                        UpdateBackupPathUI();
                        Log log = new Log(DateTime.Now, (uint)FrtvLogLevel.FrtvNormal, $"백업 저장소 변경: {dialog.FileName}");
                        log.AddAsync().GetAwaiter();
                        await MainWindow.Wnd.ShowMessageAsync("성공", $"백업 저장소가 변경되었습니다.\r\n{dialog.FileName}", settings: MainWindow.DialogSettings);
                    }
                    else if (hr != 0)
                    {
                        // 드라이버 통신에 실패한 경우 HRESULT 반환
                        await MainWindow.ShowHresultError(hr);
                        Log log = new Log(DateTime.Now, (uint)FrtvLogLevel.FrtvError, $"드라이버 통신 실패 (HRESULT: 0x{hr:X8})");
                        log.AddAsync().GetAwaiter();
                    }
                    else
                    {
                        // 그 외의 경우 정상적인 과정에서 실패함
                        await MainWindow.Wnd.ShowMessageAsync("실패", $"백업 저장소 변경에 실패했습니다.\r\nError Code: {result}", settings: MainWindow.DialogSettings);
                        Log log = new Log(DateTime.Now, (uint)FrtvLogLevel.FrtvError, $"백업 저장소 변경 실패: {dialog.FileName} (Error Code: {result})");
                        log.AddAsync().GetAwaiter();
                    }
                }
            }
            catch (Exception ex)
            {
                await MainWindow.Wnd.ShowMessageAsync("Error", $"백업 저장소 변경 중 예외가 발생했습니다.\r\n{ex.GetType().Name}: {ex.Message}", settings: MainWindow.DialogSettings);
                await Log.PrintExceptionLogFileAsync(ex);
            }
        }

        private void AddExtensionButton_Click(object sender, RoutedEventArgs e)
        {
            var flyout = new AddExtensionFlyout();

            // FlyOut이 닫힌 경우 실행되는 이벤트 콜백 함수
            RoutedEventHandler? closingFinishedHandler = null;
            closingFinishedHandler = (o, args) =>
            {
                flyout.ClosingFinished -= closingFinishedHandler;
                MainWindow.Wnd.flyoutsControl.Items.Remove(flyout);
            };

            // FlyOut ClosingFinished 이벤트 등록 및 FlyOut 열기
            flyout.ClosingFinished += closingFinishedHandler;
            MainWindow.Wnd.flyoutsControl.Items.Add(flyout);
            flyout.IsOpen = true;
        }

        private async void RemoveExtensionButton_Click(object sender, RoutedEventArgs e)
        {
            int targetCount = ExtensionDataGrid.SelectedItems.Count, successCount = 0;
            if (targetCount < 1)
            {
                MainWindow.ShowAppBar("ERROR: 적어도 한 개의 확장자를 선택해야합니다.", System.Windows.Media.Brushes.Red);
                return;
            }

            var progressDialog = await MainWindow.Wnd.ShowProgressAsync("Please wait", "확장자를 삭제하고있습니다. 잠시만 기다려주세요.", settings: MainWindow.DialogSettings);
            progressDialog.SetIndeterminate();
            try
            {
                await Task.Run(async () =>
                {
                    // foreach문 사용 시 삭제 중 리스트가 변경되어 예외가 발생하기 때문에 역순으로 뒤에서부터 처리해야함.
                    for (int i = targetCount - 1, count = 1; i >= 0; i--, count++)
                    {
                        BackupExtension? extension = null;
                        Dispatcher.Invoke(DispatcherPriority.Normal, new Action(delegate
                        {
                            extension = ExtensionDataGrid.SelectedItems[i] as BackupExtension;
                        }));

                        if (extension != null)
                        {
                            progressDialog.SetMessage($"확장자 삭제: {extension.Extension}");

                            int hr = 0;
                            int result = BridgeFunctions.RemoveExtension(extension.Extension, out hr);
                            if (result == 0 && hr == 0)
                            {
                                await extension.RemoveAsync();
                                successCount++;
                                Log log = new Log(DateTime.Now, (uint)FrtvLogLevel.FrtvError, $"백업 확장자 삭제: {extension.Extension}");
                                log.AddAsync().GetAwaiter();
                            }
                            else if (hr != 0)
                            {
                                // 드라이버 통신에 실패한 경우 HRESULT 반환
                                await MainWindow.ShowHresultError(hr);
                                Log log = new Log(DateTime.Now, (uint)FrtvLogLevel.FrtvError, $"드라이버 통신 실패 (HRESULT: 0x{hr:X8})");
                                log.AddAsync().GetAwaiter();
                                break;
                            }
                            else
                            {
                                // 그 외의 경우 정상적인 과정에서 실패함
                                // for문은 작업 진행 중 실패 시 메세지를 출력하지는 않음
                                Log log = new Log(DateTime.Now, (uint)FrtvLogLevel.FrtvError, $"백업 확장자 삭제 실패: {extension.Extension} (Error Code: {result})");
                                log.AddAsync().GetAwaiter();
                            }
                        }
                    }
                });
            }
            catch (Exception ex)
            {
                await MainWindow.Wnd.ShowMessageAsync("Error", $"백업 확장자 삭제 중 예외가 발생했습니다.\r\n{ex.GetType().Name}: {ex.Message}", settings: MainWindow.DialogSettings);
                await Log.PrintExceptionLogFileAsync(ex);
            }
            finally
            {
                if (progressDialog != null)
                    await progressDialog.CloseAsync();
            }

            if (successCount == targetCount)
            {
                MainWindow.ShowAppBar($"{targetCount}개의 확장자 삭제에 성공했습니다.", System.Windows.Media.Brushes.YellowGreen);
            }
            else
            {
                MainWindow.ShowAppBar($"{targetCount}개의 확장자 중 {targetCount - successCount}개의 확장자 삭제에 실패했습니다.\r\n자세한 내용은 로그를 확인하세요.", System.Windows.Media.Brushes.Orange);
            }
        }

        private async void AddExceptionPathButton_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                // 폴더 추가 버튼 클릭 이벤트 처리
                var dialog = new CommonOpenFileDialog();
                dialog.IsFolderPicker = true; // 폴더 선택 모드 설정

                // 대화 상자를 표시하고 사용자가 확인을 클릭한 경우 진행
                if (dialog.ShowDialog() == CommonFileDialogResult.Ok)
                {
                    int hr = 0;
                    int result = BridgeFunctions.AddExceptionPath(dialog.FileName, out hr);
                    if (result == 0 && hr == 0)
                    {
                        var path = new ExceptionPath(dialog.FileName);
                        await path.AddAsync(); Log log = new Log(DateTime.Now, (uint)FrtvLogLevel.FrtvNormal, $"백업 예외 경로 추가: {dialog.FileName}");
                        log.AddAsync().GetAwaiter();
                    }
                    else if (hr != 0)
                    {
                        // 드라이버 통신에 실패한 경우 HRESULT 반환
                        await MainWindow.ShowHresultError(hr);
                        Log log = new Log(DateTime.Now, (uint)FrtvLogLevel.FrtvError, $"드라이버 통신 실패 (HRESULT: 0x{hr:X8})");
                        log.AddAsync().GetAwaiter();
                    }
                    else
                    {
                        // 그 외의 경우 정상적인 과정에서 실패함
                        await MainWindow.Wnd.ShowMessageAsync("실패", $"백업 예외 경로 등록에 실패했습니다.\r\nError Code: {result}", settings: MainWindow.DialogSettings);
                        Log log = new Log(DateTime.Now, (uint)FrtvLogLevel.FrtvError, $"백업 예외 경로 추가 실패: {dialog.FileName} (Error Code: {result})");
                        log.AddAsync().GetAwaiter();
                    }
                }
            }
            catch (Exception ex)
            {
                await MainWindow.Wnd.ShowMessageAsync("Error", $"백업 예외 경로 등록 중 예외가 발생했습니다.\r\n{ex.GetType().Name}: {ex.Message}", settings: MainWindow.DialogSettings);
                await Log.PrintExceptionLogFileAsync(ex);
            }
        }

        private async void RemoveExceptionPathButton_Click(object sender, RoutedEventArgs e)
        {
            int targetCount = ExceptionPathDataGrid.SelectedItems.Count, successCount = 0;
            if (targetCount < 1)
            {
                MainWindow.ShowAppBar("ERROR: 적어도 한 개의 경로를 선택해야합니다.", System.Windows.Media.Brushes.Red);
                return;
            }

            var progressDialog = await MainWindow.Wnd.ShowProgressAsync("Please wait", "백업 예외 경로를 삭제하고있습니다. 잠시만 기다려주세요.", settings: MainWindow.DialogSettings);
            progressDialog.SetIndeterminate();
            try
            {
                await Task.Run(async () =>
                {
                    // foreach문 사용 시 삭제 중 리스트가 변경되어 예외가 발생하기 때문에 역순으로 뒤에서부터 처리해야함.
                    for (int i = targetCount - 1, count = 1; i >= 0; i--, count++)
                    {
                        ExceptionPath? path = null;
                        Dispatcher.Invoke(DispatcherPriority.Normal, new Action(delegate
                        {
                            path = ExceptionPathDataGrid.SelectedItems[i] as ExceptionPath;
                        }));

                        if (path != null)
                        {
                            progressDialog.SetMessage($"백업 예외 경로 삭제: {path.Path}");

                            int hr = 0;
                            int result = BridgeFunctions.RemoveExceptionPath(path.Path, out hr);
                            if (result == 0 && hr == 0)
                            {
                                await path.RemoveAsync();
                                successCount++;
                                Log log = new Log(DateTime.Now, (uint)FrtvLogLevel.FrtvNormal, $"백업 예외 경로 삭제: {path.Path}");
                                log.AddAsync().GetAwaiter();
                            }
                            else if (hr != 0)
                            {
                                // 드라이버 통신에 실패한 경우 HRESULT 반환
                                await MainWindow.ShowHresultError(hr);
                                Log log = new Log(DateTime.Now, (uint)FrtvLogLevel.FrtvError, $"드라이버 통신 실패 (HRESULT: 0x{hr:X8})");
                                log.AddAsync().GetAwaiter();
                                break;
                            }
                            else
                            {
                                // 그 외의 경우 정상적인 과정에서 실패함
                                // for문은 작업 진행 중 실패 시 메세지를 출력하지는 않음
                                Log log = new Log(DateTime.Now, (uint)FrtvLogLevel.FrtvError, $"백업 예외 경로 삭제 실패: {path.Path} (Error Code: {result})");
                                log.AddAsync().GetAwaiter();
                            }
                        }
                    }
                });
            }
            catch (Exception ex)
            {
                await MainWindow.Wnd.ShowMessageAsync("Error", $"백업 예외 경로 삭제 중 예외가 발생했습니다.\r\n{ex.GetType().Name}: {ex.Message}", settings: MainWindow.DialogSettings);
                await Log.PrintExceptionLogFileAsync(ex);
            }
            finally
            {
                if (progressDialog != null)
                    await progressDialog.CloseAsync();
            }

            if (successCount == targetCount)
            {
                MainWindow.ShowAppBar($"{targetCount}개의 백업 예외 경로 삭제에 성공했습니다.", System.Windows.Media.Brushes.YellowGreen);
            }
            else
            {
                MainWindow.ShowAppBar($"{targetCount}개의 백업 예외 경로 중 {targetCount - successCount}개의 백업 예외 경로 삭제에 실패했습니다.\r\n자세한 내용은 로그를 확인하세요.", System.Windows.Media.Brushes.Orange);
            }
        }

        private void AddIncludePathButton_Click(object sender, RoutedEventArgs e)
        {
            var flyout = new AddIncludePathFlyout();

            // FlyOut이 닫힌 경우 실행되는 이벤트 콜백 함수
            RoutedEventHandler? closingFinishedHandler = null;
            closingFinishedHandler = (o, args) =>
            {
                flyout.ClosingFinished -= closingFinishedHandler;
                MainWindow.Wnd.flyoutsControl.Items.Remove(flyout);
            };

            // FlyOut ClosingFinished 이벤트 등록 및 FlyOut 열기
            flyout.ClosingFinished += closingFinishedHandler;
            MainWindow.Wnd.flyoutsControl.Items.Add(flyout);
            flyout.IsOpen = true;
        }

        private async void RemoveIncludePathButton_Click(object sender, RoutedEventArgs e)
        {
            int targetCount = IncludePathDataGrid.SelectedItems.Count, successCount = 0;
            if (targetCount < 1)
            {
                MainWindow.ShowAppBar("ERROR: 적어도 한 개의 경로를 선택해야합니다.", System.Windows.Media.Brushes.Red);
                return;
            }

            var progressDialog = await MainWindow.Wnd.ShowProgressAsync("Please wait", "백업 대상 폴더를 삭제하고있습니다. 잠시만 기다려주세요.", settings: MainWindow.DialogSettings);
            progressDialog.SetIndeterminate();
            try
            {
                await Task.Run(async () =>
                {
                    // foreach문 사용 시 삭제 중 리스트가 변경되어 예외가 발생하기 때문에 역순으로 뒤에서부터 처리해야함.
                    for (int i = targetCount - 1, count = 1; i >= 0; i--, count++)
                    {
                        IncludePath? path = null;
                        Dispatcher.Invoke(DispatcherPriority.Normal, new Action(delegate
                        {
                            path = IncludePathDataGrid.SelectedItems[i] as IncludePath;
                        }));

                        if (path != null)
                        {
                            progressDialog.SetMessage($"백업 대상 폴더 삭제: {path.Path}");

                            int hr = 0;
                            int result = BridgeFunctions.RemoveIncludePath(path.Path, out hr);
                            if (result == 0 && hr == 0)
                            {
                                await path.RemoveAsync();
                                successCount++;
                                Log log = new Log(DateTime.Now, (uint)FrtvLogLevel.FrtvNormal, $"백업 대상 폴더 삭제: {path.Path}");
                                log.AddAsync().GetAwaiter();
                            }
                            else if (hr != 0)
                            {
                                // 드라이버 통신에 실패한 경우 HRESULT 반환
                                await MainWindow.ShowHresultError(hr);
                                Log log = new Log(DateTime.Now, (uint)FrtvLogLevel.FrtvError, $"드라이버 통신 실패 (HRESULT: 0x{hr:X8})");
                                log.AddAsync().GetAwaiter();
                                break;
                            }
                            else
                            {
                                // 그 외의 경우 정상적인 과정에서 실패함
                                // for문은 작업 진행 중 실패 시 메세지를 출력하지는 않음
                                Log log = new Log(DateTime.Now, (uint)FrtvLogLevel.FrtvError, $"백업 대상 폴더 삭제 실패: {path.Path} (Error Code: {result})");
                                log.AddAsync().GetAwaiter();
                            }
                        }
                    }
                });
            }
            catch (Exception ex)
            {
                await MainWindow.Wnd.ShowMessageAsync("Error", $"백업 대상 폴더 삭제 중 예외가 발생했습니다.\r\n{ex.GetType().Name}: {ex.Message}", settings: MainWindow.DialogSettings);
                await Log.PrintExceptionLogFileAsync(ex);
            }
            finally
            {
                if (progressDialog != null)
                    await progressDialog.CloseAsync();
            }

            if (successCount == targetCount)
            {
                MainWindow.ShowAppBar($"{targetCount}개의 백업 대상 폴더 삭제에 성공했습니다.", System.Windows.Media.Brushes.YellowGreen);
            }
            else
            {
                MainWindow.ShowAppBar($"{targetCount}개의 백업 대상 폴더 중 {targetCount - successCount}개의 백업 대상 폴더 삭제에 실패했습니다.\r\n자세한 내용은 로그를 확인하세요.", System.Windows.Media.Brushes.Orange);
            }
        }
    }
}
