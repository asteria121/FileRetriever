using System;
using System.Configuration;
using System.Data;
using System.Diagnostics;
using System.ServiceProcess;
using System.Threading;
using System.Text;
using System.Windows;

namespace FrtvGUI
{
    /// <summary>
    /// Interaction logic for App.xaml
    /// </summary>
    public partial class App : System.Windows.Application
    {
        public static void LoadKernelDriver()
        {
            ServiceController sc = new ServiceController("FileRetrieverKernel");

            try
            {
                sc.Start();

                if (sc.Status != ServiceControllerStatus.Running)
                {
                    System.Windows.MessageBox.Show($"커널 드라이버 로드에 실패했습니다.\r\n\r\nCODE: {sc.Status}", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                    System.Windows.Application.Current.Shutdown();
                }
            }
            catch (Exception ex)
            {
                System.Windows.MessageBox.Show($"커널 드라이버 로드중 오류가 발생했습니다.\r\n프로그램을 종료합니다.\r\n\r\n{ex.Message}", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                System.Windows.Application.Current.Shutdown();
            }
            finally
            {
                sc.Dispose();
            }
        }

        public static void UnloadKernelDriver()
        {
            ServiceController sc = new ServiceController("FileRetrieverKernel");

            try
            {
                sc.Stop();
            }
            catch (Exception ex)
            {
                System.Windows.MessageBox.Show($"커널 드라이버 언로드중 오류가 발생했습니다.\r\n프로그램을 종료합니다.\r\n\r\n{ex.Message}", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                System.Windows.Application.Current.Shutdown();
            }
            finally
            {
                sc.Dispose();
            }
        }
    }
}
