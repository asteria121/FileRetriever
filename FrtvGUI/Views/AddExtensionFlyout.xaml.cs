using FrtvGUI.Elements;
using FrtvGUI.Enums;
using MahApps.Metro.Controls.Dialogs;
using System.Windows;

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
                if (result == 0 && hr == 0)
                {
                    var extension = new BackupExtension(Extension.Text, MaximumFileSizeBytes, Expiration);
                    await extension.AddAsync();
                    Log log = new Log(DateTime.Now, (uint)FrtvLogLevel.FrtvNormal, $"백업 확장자 추가: {Extension.Text}");
                    log.AddAsync().GetAwaiter();

                    Extension.Text = string.Empty;
                    MaximumFileSize.Value = null;
                    ExpirationYear.Value = null;
                    ExpirationDay.Value = null;
                    ExpirationHour.Value = null;
                    ExpirationMinute.Value = null;
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
                    await DisplayExtensionAddErrorByResult(result);
                    Log log = new Log(DateTime.Now, (uint)FrtvLogLevel.FrtvError, $"백업 확장자 추가 실패: {Extension.Text} (Error Code: {result})");
                    log.AddAsync().GetAwaiter();
                }
            }
            catch (Exception ex)
            {
                await MainWindow.Wnd.ShowMessageAsync("Error", $"백업 확장자 등록 중 예외가 발생했습니다.\r\n{ex.GetType().Name}: {ex.Message}", settings: MainWindow.DialogSettings);
                await Log.PrintExceptionLogFileAsync(ex);
            }
        }

        private async Task DisplayExtensionAddErrorByResult(int result)
        {
            if (result == (int)ExtensionResult.EXT_EXISTS)
            {
                await MainWindow.Wnd.ShowMessageAsync("Error", "이미 등록된 확장자입니다.", settings: MainWindow.DialogSettings);
            }
            else if (result == (int)IncludePathResult.INCPATH_TOO_LONG)
            {
                await MainWindow.Wnd.ShowMessageAsync("Error", "확장자가 너무 깁니다. (24바이트 초과)", settings: MainWindow.DialogSettings);
            }
            else if (result == (int)IncludePathResult.INCPATH_OUT_OF_MEMORY)
            {
                await MainWindow.Wnd.ShowMessageAsync("Error", "컴퓨터의 메모리가 부족합니다.", settings: MainWindow.DialogSettings);
            }
        }
    }
}
