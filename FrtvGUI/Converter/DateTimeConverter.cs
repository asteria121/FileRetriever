using System.Globalization;
using System.Windows.Data;

namespace FrtvGUI.Converter
{
    public class DateTimeConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            DateTime time = System.Convert.ToDateTime(value);

            return time.ToString("yyyy-MM-dd (ddd) HH:mm");
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
}