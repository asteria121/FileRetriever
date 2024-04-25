namespace FrtvGUI.Elements
{
    public interface ILog
    {
        DateTime Date { get; }
        uint LogLevel { get; }
        string Message { get; }
    }
}
