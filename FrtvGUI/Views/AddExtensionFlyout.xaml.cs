using FrtvGUI.Elements;
using MahApps.Metro.Controls;
using MahApps.Metro.Controls.Dialogs;
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
    public partial class AddExtensionFlyout
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

        public AddExtensionFlyout()
        {
            InitializeComponent();
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
                int result = BridgeFunctions.AddExtension(Extension.Text, MaximumFileSizeBytes, out hr);
                if (result == 0)
                {
                    var extension = new BackupExtension(Extension.Text, MaximumFileSizeBytes, Expiration);
                    await extension.AddAsync();

                    Extension.Text = string.Empty;
                    MaximumFileSize.Value = null;
                    ExpirationYear.Value = null;
                    ExpirationDay.Value = null;
                    ExpirationHour.Value = null;
                    ExpirationMinute.Value = null;
                }
                else
                {
                    await MainWindow.Wnd.ShowMessageAsync("실패했습니다.", "실패", settings: MainWindow.DialogSettings);
                }
            }
            catch (Exception ex)
            {

            }
            finally
            {
                SettingsView.UpdateExtensionListUI();
            }
        }
    }
}
