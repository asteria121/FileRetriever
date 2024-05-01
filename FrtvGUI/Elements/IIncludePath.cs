namespace FrtvGUI.Elements
{
    public interface IIncludePath
    {
        string Path { get; }
        long MaximumSize { get; }
        TimeSpan Expiration { get; }
    }
}