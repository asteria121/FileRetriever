using FrtvGUI.Database;
using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Data.SQLite;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Runtime.Intrinsics.Arm;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Threading;

namespace FrtvGUI.Elements
{
    public class BackupFile : INotifyPropertyChanged
    {
        // NotifyPropertyChanged 인터페이스의 이벤트 선언
        public event PropertyChangedEventHandler? PropertyChanged;
        private void NotifyPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }

        private uint crc32;
        public uint Crc32
        {
            get { return crc32; }
            set { crc32 = value; NotifyPropertyChanged(nameof(Crc32)); }
        }

        private string originalpath;
        public string OriginalPath
        {
            get { return originalpath; }
            set { originalpath = value; NotifyPropertyChanged(nameof(OriginalPath)); }
        }

        private long filesize;
        public long FileSize
        {
            get { return filesize; }
            set { filesize = value; NotifyPropertyChanged(nameof(FileSize)); }
        }

        private DateTime backupdate;
        public DateTime BackupDate
        {
            get { return backupdate; }
            set { backupdate = value; NotifyPropertyChanged(nameof(BackupDate)); }
        }

        private DateTime expirationdate;
        public DateTime ExpirationDate
        {
            get { return expirationdate; }
            set { expirationdate = value; NotifyPropertyChanged(nameof(ExpirationDate)); }
        }

        private static ObservableCollection<BackupFile> instance = new ObservableCollection<BackupFile>();
        public static ObservableCollection<BackupFile> GetInstance()
        {
            if (instance == null)
                instance = new ObservableCollection<BackupFile>();

            return instance;
        }

        public BackupFile(uint crc32, string originalPath, long fileSize, DateTime backupDate, DateTime expirationDate)
        {
            this.crc32 = crc32;
            this.originalpath = originalPath;
            this.filesize = fileSize;
            this.backupdate = backupDate;
            this.expirationdate = expirationDate;
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
                            // 재연결 시 모든 데이터를 초기화 후 다시 등록한다

                            if (GetInstance().Where(x => x.Crc32 ==crc32).Count() == 0)
                            {
                                // 데이터 바인딩으로 연결되어 있어 UI 쓰레드에서 추가하는게 좋음.
                                Views.MainWindow.Wnd.Dispatcher.Invoke(DispatcherPriority.Normal, new Action(delegate
                                {
                                    GetInstance().Add(new BackupFile(crc32, originalPath, fileSize, backupDate, expirationDate));
                                }));
                            }
                        }
                    }
                    catch (Exception ex)
                    {
                        System.Windows.MessageBox.Show(ex.Message);
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

                // 데이터 바인딩으로 연결되어 있어 UI 쓰레드에서 추가하는게 좋음.
                Views.MainWindow.Wnd.Dispatcher.Invoke(DispatcherPriority.Normal, new Action(delegate
                {
                    GetInstance().Add(this);
                }));

            }
        }

        public async Task RemoveAsync()
        {
            using (var cmd = new SQLiteCommand("DELETE FROM STORAGE WHERE [CRC32]=@CRC32", RtvDB.Connection))
            {
                cmd.Parameters.AddWithValue("@CRC32", Crc32);

                await cmd.ExecuteNonQueryAsync();

                // 데이터 바인딩으로 연결되어 있어 UI 쓰레드에서 추가하는게 좋음.
                Views.MainWindow.Wnd.Dispatcher.Invoke(DispatcherPriority.Normal, new Action(delegate
                {
                    GetInstance().Remove(this);
                }));
            }
        }
    }
}
