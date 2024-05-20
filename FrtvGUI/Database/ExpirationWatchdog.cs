using FrtvGUI.Elements;
using FrtvGUI.Enums;
using FrtvGUI.Views;
using System.Data.SQLite;

namespace FrtvGUI.Database
{
    public static class ExpirationWatchdog
    {
        public static async Task InitializeWatchdog(CancellationToken token)
        {
            while (true)
            {
                try
                {
                    // INTEGER 형태로 저장된 DATETIME 값은 1당 100ns가 증가된 시간을 나타낸다.
                    using (var cmd = new SQLiteCommand("SELECT CRC32 FROM STORAGE WHERE EXPIRATIONDATE <= @NOW", RtvDB.Connection))
                    {
                        cmd.Parameters.AddWithValue("@NOW", DateTime.Now.ToBinary());

                        // 데이터베이스에서 해당 파일들을 찾아 지우는 역할까지 해야하기 때문에 위의 쿼리에서 바로 지우지 않음.
                        using (var reader = await cmd.ExecuteReaderAsync())
                        {
                            while (await reader.ReadAsync())
                            {
                                uint crc32 = (uint)Convert.ToInt32(reader["CRC32"]);
                                int hr = 0;
                                int result = BridgeFunctions.DeleteBackupFile(crc32, out hr);
                                if (result == 0 && hr == 0)
                                {
                                    await BackupFile.GetInstance().Where(x => x.Crc32 == crc32).First().RemoveAsync();
                                    Log log = new Log(DateTime.Now, (uint)FrtvLogLevel.FrtvError, $"파일 자동 삭제: {crc32:X8}");
                                    log.AddAsync().GetAwaiter();
                                }
                                else if (hr != 0)
                                {
                                    Log log = new Log(DateTime.Now, (uint)FrtvLogLevel.FrtvError, $"드라이버 통신 실패 (HRESULT: 0x{hr:X8})");
                                    log.AddAsync().GetAwaiter();
                                }
                                else
                                {
                                    Log log = new Log(DateTime.Now, (uint)FrtvLogLevel.FrtvError, $"파일 자동 삭제 실패: {crc32:X8} (NTSTATUS: 0x{result:X8})");
                                    log.AddAsync().GetAwaiter();
                                }
                            }
                        }
                    }

                    // TODO: SET CUSTOMIZE POLLING RATE
                    for (int i = 0; i < 60; i++)
                    {
                        await Task.Delay(1000);
                        if (token.IsCancellationRequested == true)
                        {
                            break;
                        }
                    }
                }
                catch (Exception ex)
                {
                    await Log.PrintExceptionLogFileAsync(ex);
                }
            }
        }
    }
}
