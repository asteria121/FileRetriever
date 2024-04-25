using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO;
using FrtvGUI.Database;
using System.Data.SQLite;
using System.Runtime.Intrinsics.Arm;

namespace FrtvGUI.Elements
{
    public class Log : ILog
    {
        public DateTime Date { get; }
        public uint LogLevel { get; }
        public string Message { get; }

        public static List<Log>? instance;

        public static List<Log> GetInstance()
        {
            if (instance == null)
                instance = new List<Log>();

            return instance;
        }

        public Log(DateTime date, uint logLevel, string message)
        {
            Date = date;
            LogLevel = logLevel;
            Message = message;
        }

        public static async Task LoadDatabaseAsync()
        {
            using (var cmd = new SQLiteCommand("SELECT * FROM LOG", RtvDB.Connection))
            using (var reader = await cmd.ExecuteReaderAsync())
            {
                while (await reader.ReadAsync())
                {
                    try
                    {
                        DateTime date = DateTime.FromBinary(Convert.ToInt64(reader["DATE"]));
                        uint logLevel = Convert.ToUInt32(reader["LOGLEVEL"]);
                        string? message = reader["MESSAGE"].ToString();

                        if (!string.IsNullOrEmpty(message))
                        {
                            GetInstance().Add(new Log(date, logLevel, message));
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
            using (var cmd = new SQLiteCommand("INSERT INTO LOG([DATE], [LOGLEVEL], [MESSAGE]) VALUES(@DATE, @LOGLEVEL, @MESSAGE)"
                , RtvDB.Connection))
            {
                cmd.Parameters.AddWithValue("@DATE", Date.ToBinary());
                cmd.Parameters.AddWithValue("@LOGLEVEL", LogLevel);
                cmd.Parameters.AddWithValue("@MESSAGE", Message);
                await cmd.ExecuteNonQueryAsync();
            }
        }

        public async Task RemoveAsync()
        {
            using (var cmd = new SQLiteCommand("DELETE FROM LOG WHERE [DATE]=@DATE", RtvDB.Connection))
            {
                cmd.Parameters.AddWithValue("@DATE", Date);

                await cmd.ExecuteNonQueryAsync();
                GetInstance().Remove(this);
            }
        }

        public static async Task RemoveAllAsync()
        {
            using (var cmd = new SQLiteCommand("DELETE FROM LOG", RtvDB.Connection))
            {
                await cmd.ExecuteNonQueryAsync();
                GetInstance().Clear();
            }
        }
    }
}
