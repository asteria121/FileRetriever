namespace FrtvGUI.Elements
{
    public interface ILog
    {
        DateTime Date { get; }
        int LogLevel { get; }
        string Message { get; }
    }
}
