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
using System.Windows.Threading;

namespace FrtvGUI.Views
{
    /// <summary>
    /// LogWindow.xaml에 대한 상호 작용 논리
    /// </summary>
    public partial class LogWindow : MetroWindow
    {
        private static DataGrid _LogDataGrid;
        public LogWindow()
        {
            InitializeComponent();
            _LogDataGrid = LogDataGrid;

            Log.GetInstance().Clear();
            Log.LoadDatabaseAsync().GetAwaiter();
            UpdateLogListUI();
        }

        public static void UpdateLogListUI()
        {
            if (Thread.CurrentThread == MainWindow.Wnd.Dispatcher.Thread) // UI THREAD
            {
                _LogDataGrid.ItemsSource ??= Log.GetInstance();
                _LogDataGrid.Items.Refresh();
            }
            else // OTHER THREAD
            {
                MainWindow.Wnd.Dispatcher.Invoke(DispatcherPriority.Normal, new Action(delegate
                {
                    _LogDataGrid.ItemsSource ??= Log.GetInstance();
                    _LogDataGrid.Items.Refresh();
                }));
            }
        }

        private async void ClearLogButton_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                await Log.RemoveAllAsync();
                UpdateLogListUI();
            }
            catch (Exception ex)
            {
                await this.ShowMessageAsync("Error", ex.Message, settings: MainWindow.DialogSettings);
            }
        }
    }
}
