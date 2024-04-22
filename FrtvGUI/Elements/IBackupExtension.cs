namespace FrtvGUI.Elements
{
    public interface IBackupExtension
    {
        string Extension { get; }
        long MaximumSize { get; }
        TimeSpan Expiration { get; }
    }
}
