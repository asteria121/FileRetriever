namespace FrtvGUI.Elements
{
    public interface IBackupFile
    {
        uint Crc32 { get; }
        string OriginalPath { get; }
        long FileSize { get; }
        DateTime BackupDate { get; }
        DateTime ExpirationDate { get; }
    }
}
