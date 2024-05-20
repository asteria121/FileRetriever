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
using System.Windows.Threading;
using System.Windows;
using FrtvGUI.Enums;

namespace FrtvGUI.Elements
{
    public class BackupExtension : INotifyPropertyChanged
    {
        // NotifyPropertyChanged 인터페이스의 이벤트 선언
        public event PropertyChangedEventHandler? PropertyChanged;
        private void NotifyPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }

        private string extension;
        public string Extension
        {
            get { return extension; }
            set { extension = value; NotifyPropertyChanged(nameof(Extension)); }
        }

        private long maximumSize;
        public long MaximumSize
        {
            get { return maximumSize; }
            set { maximumSize = value; NotifyPropertyChanged(nameof(MaximumSize)); }
        }

        private TimeSpan expiration;
        public TimeSpan Expiration
        {
            get { return expiration; }
            set { expiration = value; NotifyPropertyChanged(nameof(Expiration)); }
        }

        private static ObservableCollection<BackupExtension> instance = new ObservableCollection<BackupExtension>();
        public static ObservableCollection<BackupExtension> GetInstance()
        {
            if (instance == null)
                instance = new ObservableCollection<BackupExtension>();

            return instance;
        }

        public BackupExtension(string extension, long maximumSize, TimeSpan expiration)
        {
            this.extension = extension;
            this.maximumSize = maximumSize; 
            this.expiration = expiration;
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
                        {
                            // 데이터 바인딩으로 연결되어 있어 UI 쓰레드에서 추가하는게 좋음.
                            Views.MainWindow.Wnd.Dispatcher.Invoke(DispatcherPriority.Normal, new Action(delegate
                            {
                                GetInstance().Add(new BackupExtension(extension, maximumSize, expiration));
                            }));
                        }

                        int result = BridgeFunctions.AddExtension(extension, maximumSize, out hr);
                        if (result != 0 && hr != 0)
                        {
                            System.Windows.MessageBox.Show($"드라이버와 통신에 실패했습니다.\r\nHRESULT: 0x{hr:X8}", "FileRetriever", MessageBoxButton.OK, MessageBoxImage.Error);
                            Log log = new Log(DateTime.Now, (uint)FrtvLogLevel.FrtvError, $"드라이버 통신 실패 (HRESULT: 0x{hr:X8})");
                            log.AddAsync().GetAwaiter();
                            break;
                        }
                        else if (result != 0)
                        {
                            System.Windows.MessageBox.Show($"드라이버와 확장자 설정 동기화 중 오류가 발생했습니다.\r\nError Code: {result}", "FileRetriever", MessageBoxButton.OK, MessageBoxImage.Error);
                            Log log = new Log(DateTime.Now, (uint)FrtvLogLevel.FrtvError, $"백업 확장자 설정 동기화 실패 (Error Code: {result})");
                            log.AddAsync().GetAwaiter();
                            break;
                        }
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
                    // 데이터 바인딩으로 연결되어 있어 UI 쓰레드에서 추가하는게 좋음.
                    Views.MainWindow.Wnd.Dispatcher.Invoke(DispatcherPriority.Normal, new Action(delegate
                    {
                        GetInstance().Add(this);
                    }));
                }
            }
        }

        public async Task RemoveAsync()
        {
            using (var cmd = new SQLiteCommand("DELETE FROM EXTENSIONS WHERE [EXTENSION]=@EXTENSION", RtvDB.Connection))
            {
                cmd.Parameters.AddWithValue("@EXTENSION", Extension);
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
