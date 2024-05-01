using FrtvGUI.Database;
using System;
using System.Collections.Generic;
using System.Data.SQLite;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace FrtvGUI.Elements
{
    public class IncludePath : IIncludePath
    {
        public string Path { get; }
        public long MaximumSize { get; }
        public TimeSpan Expiration { get; }
        private static List<IncludePath>? _Instance = null;
        public static List<IncludePath> GetInstance()
        {
            if (_Instance == null)
                _Instance = new List<IncludePath>();

            return _Instance;
        }

        public IncludePath(string path, long maximumSize, TimeSpan expiration)
        {
            Path = path;
            MaximumSize = maximumSize;
            Expiration = expiration;
        }

        // 최초 로딩을 제외하고 드라이버로부터 작업 결과 수신을 위해 데이터베이스와 드라이버 데이터 추가는 따로 진행한다.
        public static async Task LoadDatabaseAsync()
        {
            using (var cmd = new SQLiteCommand("SELECT * FROM INCLUDEPATHS", RtvDB.Connection))
            using (var reader = await cmd.ExecuteReaderAsync())
            {
                while (await reader.ReadAsync())
                {
                    string? path = reader["PATH"].ToString();
                    long maximumSize = Convert.ToInt64(reader["MAXIMUMSIZE"]);
                    int hr = 0;
                    TimeSpan expiration = TimeSpan.FromSeconds(Convert.ToInt64(reader["EXPIRATION"]));

                    if (!string.IsNullOrEmpty(path))
                    {
                        // 이 함수는 재연결 콜백 함수에서도 호출되기 때문에 중복되어 추가되지 않도록 주의해야한다.
                        if (GetInstance().Where(x => string.Equals(x.Path, path)).Count() == 0)
                            GetInstance().Add(new IncludePath(path, maximumSize, expiration));

                        BridgeFunctions.AddIncludePath(path, maximumSize, out hr);
                    }
                }
            }
        }

        public async Task AddAsync()
        {
            using (var cmd = new SQLiteCommand("INSERT INTO INCLUDEPATHS([PATH], [MAXIMUMSIZE], [EXPIRATION]) VALUES(@PATH, @MAXIMUMSIZE, @EXPIRATION)",
                RtvDB.Connection))
            {
                cmd.Parameters.AddWithValue("@PATH", Path);
                cmd.Parameters.AddWithValue("@MAXIMUMSIZE", MaximumSize);
                cmd.Parameters.AddWithValue("@EXPIRATION", Convert.ToInt64(Expiration.TotalSeconds));
                await cmd.ExecuteNonQueryAsync();

                if (GetInstance().Where(x => string.Equals(x.Path, Path)).Count() == 0)
                {
                    GetInstance().Add(this);
                }
            }
        }

        public async Task RemoveAsync()
        {
            using (var cmd = new SQLiteCommand("DELETE FROM INCLUDEPATHS WHERE [PATH]=@PATH", RtvDB.Connection))
            {
                cmd.Parameters.AddWithValue("@PATH", Path);
                await cmd.ExecuteNonQueryAsync();

                GetInstance().Remove(this);
            }
        }
    }
}
