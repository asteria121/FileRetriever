using FrtvGUI.Elements;
using MahApps.Metro.Controls.Dialogs;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.IO;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Windows.Threading;

namespace FrtvGUI.Views
{
    /// <summary>
    /// FilelistView.xaml에 대한 상호 작용 논리
    /// </summary>
    public partial class FilelistView : System.Windows.Controls.UserControl
    {
        public FilelistView()
        {
            InitializeComponent();

            fileDataGrid.ItemsSource ??= BackupFile.GetInstance();
        }

        private async void RemoveFile_Click(object sender, RoutedEventArgs e)
        {
            int targetCount = fileDataGrid.SelectedItems.Count, successCount = 0;
            if (targetCount < 1)
            {
                MainWindow.ShowAppBar("ERROR: 적어도 한 개의 파일을 선택해야합니다.", System.Windows.Media.Brushes.Red);
                return;
            }

            var dialogResult = await MainWindow.Wnd.ShowMessageAsync("파일 삭제", $"정말 {targetCount}개의 백업 파일을 삭제하겠습니까?\r\n이 작업은 되돌릴 수 없습니다.", MessageDialogStyle.AffirmativeAndNegative, settings: MainWindow.DialogSettings);
            if (dialogResult == MessageDialogResult.Affirmative)
            {
                ProgressDialogController progressDialog = await MainWindow.Wnd.ShowProgressAsync("Please wait", "파일들을 삭제중입니다. 잠시만 기다려주세요.", settings: MainWindow.DialogSettings);
                progressDialog.SetProgress(0);

                try
                {
                    await Task.Run(async () =>
                    {
                        // foreach문 사용 시 삭제 중 리스트가 변경되어 예외가 발생하기 때문에 역순으로 뒤에서부터 처리해야함.
                        for (int i = targetCount - 1, count = 1; i >= 0; i--, count++)
                        {
                            BackupFile? file = null;
                            Dispatcher.Invoke(DispatcherPriority.Normal, new Action(delegate
                            {
                                file = fileDataGrid.SelectedItems[i] as BackupFile;
                            }));

                            if (file != null)
                            {
                                progressDialog.SetMessage($"파일 삭제: {System.IO.Path.GetFileName(file.OriginalPath)}");

                                int hr = 0;
                                int result = BridgeFunctions.DeleteBackupFile(file.Crc32, out hr);
                                if (result == 0)
                                {
                                    await file.RemoveAsync();
                                    successCount++;
                                    // TODO: 로그를 남긴다
                                }
                                else
                                {
                                    // TODO: 로그를 남긴다
                                }
                            }

                            double progressRate = (double)count / (double)targetCount;
                            progressDialog.SetProgress(progressRate);
                        }
                    });
                }
                catch (Exception ex)
                {
                    await MainWindow.Wnd.ShowMessageAsync("Error", $"파일 삭제 중 예외가 발생했습니다. 진행되지 않은 작업은 취소됩니다.{ex.GetType().Name}: {ex.Message}");
                    // TODO: 로그를 남긴다
                }
                finally
                {
                    if (progressDialog != null)
                        await progressDialog.CloseAsync();
                }

                if (successCount == targetCount)
                {
                    MainWindow.ShowAppBar($"{targetCount}개의 파일 삭제에 성공했습니다.", System.Windows.Media.Brushes.YellowGreen);
                }
                else
                {
                    MainWindow.ShowAppBar($"{targetCount}개의 파일 중 {targetCount - successCount}개의 파일 삭제에 실패했습니다.\r\n자세한 내용은 로그를 확인하세요.", System.Windows.Media.Brushes.Orange);
                }
            }
        }

        private async void RestoreButton_Click(object sender, RoutedEventArgs e)
        {
            int targetCount = fileDataGrid.SelectedItems.Count;
            if (targetCount < 1)
            {
                MainWindow.ShowAppBar("ERROR: 적어도 한 개의 파일을 선택해야합니다.", System.Windows.Media.Brushes.Red);
                return;
            }

            var dialogResult = await MainWindow.Wnd.ShowMessageAsync("파일 복원", $"정말 {targetCount}개의 백업 파일을 복원하겠습니까?", MessageDialogStyle.AffirmativeAndNegative, settings: MainWindow.DialogSettings);
            if (dialogResult == MessageDialogResult.Affirmative)
            {
                ProgressDialogController progressDialog = await MainWindow.Wnd.ShowProgressAsync("Please wait", "파일들을 복원중입니다. 잠시만 기다려주세요.", settings: MainWindow.DialogSettings);
                progressDialog.SetProgress(0);

                var overwriteDialogSettings = new MetroDialogSettings
                {
                    AffirmativeButtonText = "예",
                    NegativeButtonText = "아니오",
                    FirstAuxiliaryButtonText = $"모두 예",
                    SecondAuxiliaryButtonText = $"모두 아니오",
                    AnimateShow = false,
                    AnimateHide = false,
                    ColorScheme = MainWindow.Wnd.MetroDialogOptions.ColorScheme
                };

                // 덮어 씌울 파일의 개수를 미리 계산한다.
                int existsCount = 0;
                int successCount = 0;
                int passCount = 0;
                int failCount = 0;
                bool overwriteAll = false;
                bool passAll = false;

                try
                {
                    foreach (BackupFile file in fileDataGrid.SelectedItems)
                    {
                        if (File.Exists(file.OriginalPath))
                            existsCount++;
                    }

                    await Task.Run(async () =>
                    {
                        // foreach문 사용 시 삭제 중 리스트가 변경되어 예외가 발생하기 때문에 역순으로 뒤에서부터 처리해야함.
                        for (int i = targetCount - 1, count = 0; i >= 0; i--, count++)
                        {
                            BackupFile? file = null;
                            Dispatcher.Invoke(DispatcherPriority.Normal, new Action(delegate
                            {
                                file = fileDataGrid.SelectedItems[i] as BackupFile;
                            }));

                            if (file != null)
                            {
                                progressDialog.SetMessage($"파일 복원: {file.OriginalPath}");

                                // 파일 복원 중 똑같은 파일이 존재하는 경우
                                if (File.Exists(file.OriginalPath))
                                {
                                    MessageDialogResult dialogResult = MessageDialogResult.Negative;
                                    // 모두 덮어씌우기 또는 모두 건너뛰기를 선택하지 않은 경우 물어본다
                                    if (passAll == false && overwriteAll == false)
                                    {
                                        dialogResult = await Dispatcher.InvokeAsync(async () =>
                                        {
                                            return await MainWindow.Wnd.ShowMessageAsync("파일 복원", $"대상 폴더에 똑같은 이름의 파일이 {existsCount}개 존재합니다. 덮어쓸까요?\r\n{file.OriginalPath}",
                                                MessageDialogStyle.AffirmativeAndNegativeAndDoubleAuxiliary, settings: overwriteDialogSettings);
                                        }).Result;

                                        if (dialogResult == MessageDialogResult.FirstAuxiliary)
                                            overwriteAll = true;
                                        else if (dialogResult == MessageDialogResult.SecondAuxiliary)
                                            passAll = true;
                                    }

                                    // 덮어 씌우기 또는 모두 덮어씌우기
                                    if (overwriteAll == true || dialogResult == MessageDialogResult.Affirmative)
                                    {
                                        int hr = 0;
                                        int result = BridgeFunctions.RestoreBackupFile(file.OriginalPath, file.Crc32, true, out hr);
                                        if (result == 0)
                                        {
                                            await file.RemoveAsync();
                                            successCount++;
                                        }
                                        else
                                        {
                                            failCount++;
                                            // TODO: 로그를 남긴다
                                        }
                                    }
                                    // 건너뛰기 또는 모두 건너뛰기
                                    else if (passAll == true || dialogResult == MessageDialogResult.Negative)
                                    {
                                        passCount++;
                                    }

                                    existsCount--;
                                }
                                // 똑같은 파일이 존재하지 않는 경우
                                else
                                {
                                    int hr = 0;
                                    int result = BridgeFunctions.RestoreBackupFile(file.OriginalPath, file.Crc32, false, out hr);
                                    if (result == 0)
                                    {
                                        await file.RemoveAsync();
                                        successCount++;
                                    }
                                    else
                                    {
                                        failCount++;
                                        // TODO: 로그를 남긴다
                                    }
                                }
                            }

                            double progressRate = (double)count / (double)targetCount;
                            progressDialog.SetProgress(progressRate);
                        }
                    });
                }
                catch (Exception ex)
                {
                    await MainWindow.Wnd.ShowMessageAsync("Error", $"파일 복원 중 예외가 발생했습니다. 진행되지 않은 작업은 취소됩니다.{ex.GetType().Name}: {ex.Message}");
                    // TODO: 로그를 남긴다
                }
                finally
                {
                    if (progressDialog != null)
                        await progressDialog.CloseAsync();
                }

                if (failCount == 0)
                {
                    MainWindow.ShowAppBar($"{successCount}개의 파일 복원에 성공했습니다. (건너뜀: {passCount})", System.Windows.Media.Brushes.YellowGreen);
                }
                else
                {
                    MainWindow.ShowAppBar($"{targetCount}개의 파일 중 {failCount}개의 파일 복원에 실패했습니다. (건너뜀: {passCount})\r\n자세한 내용은 로그를 확인하세요.", System.Windows.Media.Brushes.Orange);
                }
            }
        }
    }
}
