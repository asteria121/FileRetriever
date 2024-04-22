using System.Windows.Controls;
using System.Windows.Media;
using System.Windows;

namespace FrtvGUI.Views
{
    // DPI 크기에 상관 없이 컨트롤들을 고정된 크기로 만들어주는 클래스
    // local:DpiDecorator 태그로 씌우기
    // https://github.com/mesta1/DPIHelper
    public partial class DpiDecorator : Decorator
    {
        public DpiDecorator()
        {
            Loaded += OnLoaded;
            Unloaded -= OnLoaded;
        }

        private void OnLoaded(object sender, RoutedEventArgs e)
        {
            if (null != e)
                e.Handled = true;

            var source = PresentationSource.FromVisual(this);

            if (null != source && null != source.CompositionTarget)
            {
                var matrix = source.CompositionTarget.TransformToDevice;

                var dpiTransform = new ScaleTransform(1 / matrix.M11, 1 / matrix.M22);

                if (dpiTransform.CanFreeze)
                    dpiTransform.Freeze();

                LayoutTransform = dpiTransform;
            }
        }
    }
}
