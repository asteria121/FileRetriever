using FrtvGUI.Database;
using System;
using System.Collections.Generic;
using System.Data.SQLite;
using System.Linq;
using System.Runtime.Intrinsics.Arm;
using System.Text;
using System.Threading.Tasks;
using System.Windows;

namespace FrtvGUI.Elements
{
    public class BackupFile : IBackupFile
    {
        public uint Crc32 { get; }
        public string OriginalPath { get; }
        public long FileSize { get; }
        public DateTime BackupDate { get; }
        public DateTime ExpirationDate { get; }
        private static List<BackupFile>? instance;
        public static List<BackupFile> GetInstance()
        {
            if (instance == null)
                instance = new List<BackupFile>();

            return instance;
        }

        public BackupFile(uint crc32, string originalPath, long fileSize, DateTime backupDate, DateTime expirationDate)
        {
            Crc32 = crc32;
            OriginalPath = originalPath;
            FileSize = fileSize;
            BackupDate = backupDate;
            ExpirationDate = expirationDate;
        }

        public static async Task LoadDatabaseAsync()
        {
            using (var cmd = new SQLiteCommand("SELECT * FROM STORAGE", RtvDB.Connection))
            using (var reader = await cmd.ExecuteReaderAsync())
            {
                while (await reader.ReadAsync())
                {
                    try
                    {
                        uint crc32 = (uint)Convert.ToInt32(reader["CRC32"]);
                        string? originalPath = reader["ORIGINALPATH"].ToString();
                        long fileSize = Convert.ToInt64(reader["FILESIZE"]);

                        DateTime backupDate = DateTime.FromBinary(Convert.ToInt64(reader["BACKUPDATE"]));
                        DateTime expirationDate = DateTime.FromBinary(Convert.ToInt64(reader["EXPIRATIONDATE"]));

                        if (!string.IsNullOrEmpty(originalPath))
                        {
                            // 이 함수는 재연결 콜백 함수에서도 호출되기 때문에 중복되어 추가되지 않도록 주의해야한다.
                            if (GetInstance().Where(x => x.Crc32 == crc32).Count() == 0)
                                GetInstance().Add(new BackupFile(crc32, originalPath, fileSize, backupDate, expirationDate));
                        }
                    }
                    catch (Exception ex)
                    {
                        System.Windows.MessageBox.Show(ex.StackTrace);
                    }
                }
            }
        }

        public async Task AddAsync()
        {
            using (var cmd = new SQLiteCommand("INSERT INTO STORAGE([CRC32], [ORIGINALPATH], [FILESIZE], [BACKUPDATE], [EXPIRATIONDATE]) VALUES(@CRC32, @ORIGINALPATH, @FILESIZE, @BACKUPDATE, @EXPIRATIONDATE)"
                , RtvDB.Connection))
            {
                cmd.Parameters.AddWithValue("@CRC32", Crc32);
                cmd.Parameters.AddWithValue("@ORIGINALPATH", OriginalPath);
                cmd.Parameters.AddWithValue("@FILESIZE", FileSize);
                cmd.Parameters.AddWithValue("@BACKUPDATE", BackupDate.ToBinary());
                cmd.Parameters.AddWithValue("@EXPIRATIONDATE", ExpirationDate.ToBinary());
                await cmd.ExecuteNonQueryAsync();

                if (GetInstance().Where(x => x.Crc32 == Crc32).Count() == 0)
                    GetInstance().Add(this);
            }
        }

        public async Task RemoveAsync()
        {
            using (var cmd = new SQLiteCommand("DELETE FROM STORAGE WHERE [CRC32]=@CRC32", RtvDB.Connection))
            {
                cmd.Parameters.AddWithValue("@CRC32", Crc32);

                await cmd.ExecuteNonQueryAsync();
                GetInstance().Remove(this);
            }
        }
    }
}
