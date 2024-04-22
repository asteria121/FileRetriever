using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Data.SQLite;
using Microsoft.Win32;
using System.Windows;

namespace FrtvGUI.Database
{
    public static class Settings
    {
        public static bool GetBackupEnabled()
        {
            RegistryKey? reg = Registry.CurrentUser;
            reg = reg.OpenSubKey("Software\\FileRetriever", true);

            if (reg == null)
            {
                // 없을 경우 키 생성
                Registry.CurrentUser.CreateSubKey("Software").CreateSubKey("FileRetriever");
            }

            if (reg != null)
            {
                var value = reg.GetValue("BackupEnabled");
                if (value != null)
                {
                    return Convert.ToBoolean(value);
                }
            }

            return false;
        }

        public static void SetBackupEnabled(bool isBackupEnabled)
        {
            RegistryKey? reg = Registry.CurrentUser;
            reg = reg.OpenSubKey("Software\\FileRetriever", true);

            if (reg == null)
            {
                // 없을 경우 키 생성
                Registry.CurrentUser.CreateSubKey("Software").CreateSubKey("FileRetriever");
            }

            if (reg != null)
            {
                reg.SetValue("BackupEnabled", isBackupEnabled, RegistryValueKind.DWord);
            }
        }

        public static string GetBackupPath()
        {
            RegistryKey? reg = Registry.CurrentUser;
            reg = reg.OpenSubKey("Software\\FileRetriever", true);

            if (reg == null)
            {
                // 없을 경우 키 생성
                Registry.CurrentUser.CreateSubKey("Software").CreateSubKey("FileRetriever");
            }

            if (reg != null)
            {
                string? path = reg.GetValue("BackupPath", "").ToString();
                if (!string.IsNullOrEmpty(path))
                {
                    return path;
                }
            }

            return string.Empty;
        }

        public static void SetBackupPath(string backupPath)
        {
            RegistryKey? reg = Registry.CurrentUser;
            reg = reg.OpenSubKey("Software\\FileRetriever", true);

            if (reg == null)
            {
                // 없을 경우 키 생성
                Registry.CurrentUser.CreateSubKey("Software").CreateSubKey("FileRetriever");
            }

            if (reg != null)
            {
                reg.SetValue("BackupPath", backupPath, RegistryValueKind.String);
            }
        }
    }
}
