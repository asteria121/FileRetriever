using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Data.SQLite;

namespace FrtvGUI.Database
{
    public static class RtvDB
    {
        private static SQLiteConnection connection;
        public static SQLiteConnection Connection
        {
            get { return connection; }
        }

        public async static Task<bool> InitializeDatabaseAsync()
        {
            connection = new SQLiteConnection(@"Data Source=GoldenRetriever.db");
            await connection.OpenAsync();

            // TODO: 연결 안된 상황 대비

            using (SQLiteCommand cmd = new SQLiteCommand())
            {
                cmd.Connection = connection;
                cmd.CommandText = "CREATE TABLE IF NOT EXISTS STORAGE (CRC32 INT PRIMARY KEY, ORIGINALPATH VARCHAR(260), FILESIZE INTEGER, BACKUPDATE INTEGER, EXPIRATIONDATE INTEGER)";
                await cmd.ExecuteNonQueryAsync();

                cmd.CommandText = "CREATE TABLE IF NOT EXISTS EXTENSIONS (EXTENSION VARCHAR(30) PRIMARY KEY, MAXIMUMSIZE INTEGER, EXPIRATION INTEGER)";
                await cmd.ExecuteNonQueryAsync();

                cmd.CommandText = "CREATE TABLE IF NOT EXISTS PATHS (PATH VARCHAR(260) PRIMARY KEY)";
                await cmd.ExecuteNonQueryAsync();

                cmd.CommandText = "CREATE TABLE IF NOT EXISTS LOG (DATE INTEGER PRIRMARY KEY, LOGLEVEL INT, MESSAGE VARCHAR(500))";
                await cmd.ExecuteNonQueryAsync();
            }

            return true;
        }
    }
}
