using FrtvGUI.Database;
using FrtvGUI.Elements;
using MahApps.Metro.Controls;
using MahApps.Metro.Controls.Dialogs;
using Microsoft.WindowsAPICodePack.Dialogs;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Threading;

namespace FrtvGUI.Views
{
    /// <summary>
    /// SettingsView.xaml에 대한 상호 작용 논리
    /// </summary>
    public partial class SettingsView : System.Windows.Controls.UserControl
    {
        private static DataGrid ExtDataGrid;
        private static DataGrid ExpathDataGrid;
        private static ToggleSwitch _BackupToggleSwtch;
        public static ToggleSwitch BackupToggleSwtch
        {
            get { return _BackupToggleSwtch; }
            private set { _BackupToggleSwtch = value; }
        }
        private static System.Windows.Controls.TextBox BackupPathTxtBox;
        public static bool ToggleMenuProgramChanged = false;

        public SettingsView()
        {
            InitializeComponent();
            ExtDataGrid = ExtensionDataGrid;
            ExpathDataGrid = ExceptionPathDataGrid;
            BackupToggleSwtch = BackupToggleSwitch;
            BackupPathTxtBox = BackupPathTextBox;
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
                BackupPathTxtBox.Text = Settings.GetBackupPath();
                MainWindow.ToggleMenu.Checked = BackupToggleSwtch.IsOn;
            }
            else
            {
                MainWindow.Wnd.Dispatcher.Invoke(DispatcherPriority.Normal, new Action(delegate
                {
                    BackupToggleSwtch.IsOn = Settings.GetBackupEnabled();
                    BackupPathTxtBox.Text = Settings.GetBackupPath();
                    MainWindow.ToggleMenu.Checked = BackupToggleSwtch.IsOn;
                }));
            }
        }

        public static void UpdateExtensionListUI()
        {
            if (Thread.CurrentThread == MainWindow.Wnd.Dispatcher.Thread) // UI THREAD
            {
                ExtDataGrid.ItemsSource ??= BackupExtension.GetInstance();
                ExtDataGrid.Items.Refresh();
            }
            else
            {
                MainWindow.Wnd.Dispatcher.Invoke(DispatcherPriority.Normal, new Action(delegate
                {
                    ExtDataGrid.ItemsSource ??= BackupExtension.GetInstance();
                    ExtDataGrid.Items.Refresh();
                }));
            }
        }

        public static void UpdateExceptionPathListUI()
        {
            if (Thread.CurrentThread == MainWindow.Wnd.Dispatcher.Thread) // UI THREAD
            {
                ExpathDataGrid.ItemsSource ??= ExceptionPath.GetInstance();
                ExpathDataGrid.Items.Refresh();
            }
            else
            {
                MainWindow.Wnd.Dispatcher.Invoke(DispatcherPriority.Normal, new Action(delegate
                {
                    ExpathDataGrid.ItemsSource ??= ExceptionPath.GetInstance();
                    ExpathDataGrid.Items.Refresh();
                }));
            }
        }

        private async void BackupToggleSwitch_Toggled(object sender, RoutedEventArgs e)
        {
            // 프로그램이 상태를 변경해도 이벤트가 호출되지 않도록 함
            if (ToggleMenuProgramChanged == false)
            {
                if (BackupToggleSwitch.IsOn == true)
                {
                    int result = BridgeFunctions.ToggleBackupSwitch(1);
                    if (result == 0)
                    {
                        // 성공 시 레지스트리와 모든 UI 설정을 동기화한다.
                        Settings.SetBackupEnabled(true);
                    }
                    else
                    {
                        await MainWindow.Wnd.ShowMessageAsync("Error", $"실시간 백업 설정 변경에 실패했습니다.\r\nError Code: {result}", settings: MainWindow.DialogSettings);
                    }
                }
                else
                {
                    // 실시간 백업 OFF는 사용자에게 확인하는 절차를 갖는다.
                    var messageDialogResult = await MainWindow.Wnd.ShowMessageAsync("Info", $"실시간 백업 종료시 자동으로 백업되지 않습니다.\r\n계속 하시겠습니까?", MessageDialogStyle.AffirmativeAndNegative, settings: MainWindow.DialogSettings);
                    if (messageDialogResult == MessageDialogResult.Affirmative)
                    {
                        int result = BridgeFunctions.ToggleBackupSwitch(0);
                        if (result == 0)
                        {
                            // 성공 시 레지스트리와 모든 UI 설정을 동기화한다.
                            Settings.SetBackupEnabled(false);
                        }
                        else
                        {
                            await MainWindow.Wnd.ShowMessageAsync("Error", $"실시간 백업 설정 변경에 실패했습니다.\r\nError Code: {result}", settings: MainWindow.DialogSettings);
                        }
                    }
                }

                // 최종 레지스트리 값으로 UI와 우클릭 메뉴를 동기화한다.
                UpdateBackupSettingsUI();
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

                    int result = BridgeFunctions.UpdateBackupFolder(dialog.FileName);
                    if (result == 0)
                    {
                        BackupPathTextBox.Text = dialog.FileName;
                        Settings.SetBackupPath(dialog.FileName);
                        await MainWindow.Wnd.ShowMessageAsync("성공", $"경로가 변경되었습니다.\r\n{dialog.FileName}", settings: MainWindow.DialogSettings);
                    }
                    else
                    {
                        await MainWindow.Wnd.ShowMessageAsync("실패", $"백업 저장소 변경에 실패했습니다.\r\nError Code: {result}", settings: MainWindow.DialogSettings);
                    }
                }
            }
            catch (Exception ex)
            {
                await MainWindow.Wnd.ShowMessageAsync("Error", $"백업 저장소 변경 중 오류가 발생했습니다.\r\n{ex.Message}\r\n{ex.StackTrace}", settings: MainWindow.DialogSettings);
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
            var targetList = ExtensionDataGrid.SelectedItems;
            if (targetList.Count == 0)
            {
                MainWindow.ShowAppBar("ERROR: 적어도 한 개의 확장자를 선택해야합니다.");
                return;
            }

            var progressDialog = await MainWindow.Wnd.ShowProgressAsync("Please wait", "확장자를 삭제하고있습니다. 잠시만 기다려주세요.", settings: MainWindow.DialogSettings);
            progressDialog.SetIndeterminate();
            try
            {
                foreach (BackupExtension item in targetList)
                {
                    int result = BridgeFunctions.RemoveExtension(item.Extension);
                    if (result == 0)
                        await item.RemoveAsync();
                }
            }
            catch (Exception ex)
            {
                await MainWindow.Wnd.ShowMessageAsync("Error", ex.Message);
            }
            finally
            {
                if (progressDialog != null)
                    await progressDialog.CloseAsync();

                ExtensionDataGrid.Items.Refresh();
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
                    int result = BridgeFunctions.AddExceptionPath(dialog.FileName);
                    if (result == 0)
                    {
                        var path = new ExceptionPath(dialog.FileName);
                        await path.AddAsync();
                    }
                }
            }
            catch (Exception ex)
            {

            }
            finally
            {
                ExceptionPathDataGrid.Items.Refresh();
            }
        }

        private async void RemoveExceptionPathButton_Click(object sender, RoutedEventArgs e)
        {
            var targetList = ExceptionPathDataGrid.SelectedItems;
            if (targetList.Count == 0)
            {
                MainWindow.ShowAppBar("ERROR: 적어도 한 개의 경로를 선택해야합니다.");
                return;
            }

            try
            {
                foreach (ExceptionPath path in targetList)
                {
                    int result = BridgeFunctions.RemoveExceptionPath(path.Path);
                    if (result == 0)
                        await path.RemoveAsync();
                }
            }
            catch (Exception ex)
            {

            }
            finally
            {
                ExceptionPathDataGrid.Items.Refresh();
            }
        }
    }
}
