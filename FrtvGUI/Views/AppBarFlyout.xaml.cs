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
    /// <summary>
    /// AppBarFlyout.xaml에 대한 상호 작용 논리
    /// </summary>
    public partial class AppBarFlyout
    {
        public AppBarFlyout()
        {
            InitializeComponent();
        }

        public AppBarFlyout(string message, System.Windows.Media.Brush brush)
        {
            InitializeComponent();
            NotificationMessageBlock.Text = message;
            Background = brush;
        }
    }
}
