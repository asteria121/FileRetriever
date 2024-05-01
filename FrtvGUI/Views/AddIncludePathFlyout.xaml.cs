using FrtvGUI.Database;
using FrtvGUI.Elements;
using MahApps.Metro.Controls;
using MahApps.Metro.Controls.Dialogs;
using Microsoft.WindowsAPICodePack.Dialogs;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;

namespace FrtvGUI.Views
{
    public partial class AddIncludePathFlyout
    {
        private bool confirmed = false;
        public bool Confirmed
        {
            get { return confirmed; }
        }

        private long maximumFileSizeBytes = 0;
        public long MaximumFileSizeBytes
        {
            get { return maximumFileSizeBytes; }
        }

        TimeSpan expiration;
        public TimeSpan Expiration
        {
            get { return expiration; }
        }

        public AddIncludePathFlyout()
        {
            InitializeComponent();
        }

        private void Button_Click(object sender, RoutedEventArgs e)
        {
            // 이곳에 버튼 클릭 시 실행할 코드를 작성합니다.
            System.Windows.MessageBox.Show("TEST");
        }

        private async void confirmButton_Click(object sender, RoutedEventArgs e)
        {
            long totalSeconds = Convert.ToInt64((ExpirationYear.Value * 365 * 86400) + (ExpirationDay.Value * 86400) + (ExpirationHour.Value * 3600) + (ExpirationMinute.Value * 60));
            if (totalSeconds > 315360000) // 10년
                return;

            // 인덱스 넘버 만큼 1024를 곱해 GB, MB 단위의 파일을 B로 변환하여 사용하기에 편리하도록 함
            maximumFileSizeBytes = Convert.ToInt64(MaximumFileSize.Value);
            for (int i = 0; i < FileSizeUnit.SelectedIndex; i++)
                maximumFileSizeBytes *= 1024;

            expiration = TimeSpan.FromSeconds(totalSeconds);

            try
            {
                int hr = 0;
                int result = BridgeFunctions.AddIncludePath(Path.Text, MaximumFileSizeBytes, out hr);
                if (result == 0)
                {
                    var path = new IncludePath(Path.Text, MaximumFileSizeBytes, Expiration);
                    await path.AddAsync();

                    Path.Text = string.Empty;
                    MaximumFileSize.Value = null;
                    ExpirationYear.Value = null;
                    ExpirationDay.Value = null;
                    ExpirationHour.Value = null;
                    ExpirationMinute.Value = null;
                }
                else
                {
                    // TODO: 상위폴더 및 하위폴더 중복 불가 알리기
                    await MainWindow.Wnd.ShowMessageAsync("실패했습니다.", "실패", settings: MainWindow.DialogSettings);
                }
            }
            catch (Exception ex)
            {

            }
            finally
            {
                SettingsView.UpdateIncludePathListUI();
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
                    Path.Text = dialog.FileName;
                }
            }
            catch (Exception ex)
            {
                await MainWindow.Wnd.ShowMessageAsync("Error", $"경로를 가져오는 중 오류가 발생했습니다.\r\n{ex.Message}\r\n{ex.StackTrace}", settings: MainWindow.DialogSettings);
            }
        }
    }
}
