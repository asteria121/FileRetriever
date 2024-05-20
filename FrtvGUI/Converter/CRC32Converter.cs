using System.Globalization;
using System.Runtime.Intrinsics.Arm;
using System.Windows.Data;

namespace FrtvGUI.Converter
{
    public class CRC32Converter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            return System.Convert.ToUInt32(value).ToString("X8");
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
}