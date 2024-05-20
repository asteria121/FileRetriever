﻿using FrtvGUI.Database;
using FrtvGUI.Enums;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Data.SQLite;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Threading;

namespace FrtvGUI.Elements
{
    public class ExceptionPath : INotifyPropertyChanged
    {
        // NotifyPropertyChanged 인터페이스의 이벤트 선언
        public event PropertyChangedEventHandler? PropertyChanged;
        private void NotifyPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }

        private string path;
        public string Path
        {
            get { return path; }
            set { path = value; NotifyPropertyChanged(nameof(Path)); }
        }

        private static ObservableCollection<ExceptionPath> instance = new ObservableCollection<ExceptionPath>();
        public static ObservableCollection<ExceptionPath> GetInstance()
        {
            if (instance == null)
                instance = new ObservableCollection<ExceptionPath>();

            return instance;
        }

        public ExceptionPath(string path)
        {
            this.path = path;
        }

        // 최초 로딩을 제외하고 드라이버로부터 작업 결과 수신을 위해 데이터베이스와 드라이버 데이터 추가는 따로 진행한다.
        public static async Task LoadDatabaseAsync()
        {
            using (var cmd = new SQLiteCommand("SELECT * FROM PATHS", RtvDB.Connection))
            using (var reader = await cmd.ExecuteReaderAsync())
            {
                while (await reader.ReadAsync())
                {
                    string? path = reader["PATH"].ToString();
                    int hr = 0;

                    if (!string.IsNullOrEmpty(path))
                    {
                        // 이 함수는 재연결 콜백 함수에서도 호출되기 때문에 중복되어 추가되지 않도록 주의해야한다.
                        if (GetInstance().Where(x => string.Equals(x.Path, path)).Count() == 0)
                        {
                            // 데이터 바인딩으로 연결되어 있어 UI 쓰레드에서 추가하는게 좋음.
                            Views.MainWindow.Wnd.Dispatcher.Invoke(DispatcherPriority.Normal, new Action(delegate
                            {
                                GetInstance().Add(new ExceptionPath(path));
                            }));
                        }

                        int result = BridgeFunctions.AddExceptionPath(path, out hr);
                        if (result != 0 && hr != 0)
                        {
                            System.Windows.MessageBox.Show($"드라이버와 통신에 실패했습니다.\r\nHRESULT: 0x{hr:X8}", "FileRetriever", MessageBoxButton.OK, MessageBoxImage.Error);
                            Log log = new Log(DateTime.Now, (uint)FrtvLogLevel.FrtvError, $"드라이버 통신 실패 (HRESULT: 0x{hr:X8})");
                            log.AddAsync().GetAwaiter();
                            break;
                        }
                        else if (result != 0)
                        {
                            System.Windows.MessageBox.Show($"드라이버와 예외 경로 설정 동기화 중 오류가 발생했습니다.\r\nError Code: {result}", "FileRetriever", MessageBoxButton.OK, MessageBoxImage.Error);
                            Log log = new Log(DateTime.Now, (uint)FrtvLogLevel.FrtvError, $"백업 예외 경로 설정 동기화 실패 (Error Code: {result})");
                            log.AddAsync().GetAwaiter();
                            break;
                        }
                    }
                }
            }
        }

        public async Task AddAsync()
        {
            using (var cmd = new SQLiteCommand("INSERT INTO PATHS([PATH]) VALUES(@PATH)", RtvDB.Connection))
            {
                cmd.Parameters.AddWithValue("@PATH", Path);
                await cmd.ExecuteNonQueryAsync();

                if (GetInstance().Where(x => string.Equals(x.Path, Path)).Count() == 0)
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
            using (var cmd = new SQLiteCommand("DELETE FROM PATHS WHERE [PATH]=@PATH", RtvDB.Connection))
            {
                cmd.Parameters.AddWithValue("@PATH", Path);
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
