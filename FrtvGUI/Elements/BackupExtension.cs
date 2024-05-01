using System;
using System.Collections.Generic;
using System.Data.Common;
using System.Data.SQLite;
using System.Linq;
using System.Runtime.Intrinsics.Arm;
using System.Text;
using System.Threading.Tasks;
using FrtvGUI.Database;
using FrtvGUI;
using System.ComponentModel;
using System.Collections.ObjectModel;

namespace FrtvGUI.Elements
{
    public class BackupExtension : IBackupExtension
    {
        public string Extension { get; }
        public long MaximumSize { get; }
        public TimeSpan Expiration { get; }
        private static List<BackupExtension>? _Instance = null;
        public static List<BackupExtension> GetInstance()
        {
            if (_Instance == null)
                _Instance = new List<BackupExtension>();

            return _Instance;
        }

        public BackupExtension(string extension, long maximumSize, TimeSpan expiration)
        {
            Extension = extension;
            MaximumSize = maximumSize; 
            Expiration = expiration;
        }

        // 최초 로딩을 제외하고 드라이버로부터 작업 결과 수신을 위해 데이터베이스와 드라이버 데이터 추가는 따로 진행한다.
        public static async Task LoadDatabaseAsync()
        {
            using (var cmd = new SQLiteCommand("SELECT * FROM EXTENSIONS", RtvDB.Connection))
            using (var reader = await cmd.ExecuteReaderAsync())
            {
                while (await reader.ReadAsync())
                {
                    string? extension = reader["EXTENSION"].ToString();
                    long maximumSize = Convert.ToInt64(reader["MAXIMUMSIZE"]);
                    int hr = 0;
                    TimeSpan expiration = TimeSpan.FromSeconds(Convert.ToInt64(reader["EXPIRATION"]));

                    if (!string.IsNullOrEmpty(extension))
                    {
                        // 이 함수는 재연결 콜백 함수에서도 호출되기 때문에 중복되어 추가되지 않도록 주의해야한다.
                        if (GetInstance().Where(x => string.Equals(x.Extension, extension)).Count() == 0)
                            GetInstance().Add(new BackupExtension(extension, maximumSize, expiration));

                        BridgeFunctions.AddExtension(extension, maximumSize, out hr);
                    }
                }
            }
        }

        public async Task AddAsync()
        {
            using (var cmd = new SQLiteCommand("INSERT INTO EXTENSIONS([EXTENSION], [MAXIMUMSIZE], [EXPIRATION]) VALUES(@EXTENSION, @MAXIMUMSIZE, @EXPIRATION)",
                RtvDB.Connection))
            {
                cmd.Parameters.AddWithValue("@EXTENSION", Extension);
                cmd.Parameters.AddWithValue("@MAXIMUMSIZE", MaximumSize);
                cmd.Parameters.AddWithValue("@EXPIRATION", Convert.ToInt64(Expiration.TotalSeconds));
                await cmd.ExecuteNonQueryAsync();

                if (GetInstance().Where(x => string.Equals(x.Extension, Extension)).Count() == 0)
                {
                    GetInstance().Add(this);
                }
            }
        }

        public async Task RemoveAsync()
        {
            using (var cmd = new SQLiteCommand("DELETE FROM EXTENSIONS WHERE [EXTENSION]=@EXTENSION", RtvDB.Connection))
            {
                cmd.Parameters.AddWithValue("@EXTENSION", Extension);
                await cmd.ExecuteNonQueryAsync();

                GetInstance().Remove(this);
            }
        }
    }
}
