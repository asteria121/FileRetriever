using System;
using System.Collections.Generic;
using System.Linq;
using System.ComponentModel;
using System.Text.RegularExpressions;
using System.Globalization;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Documents;
using System.Windows.Media;
using System.Windows.Input;
using MahApps.Metro.Controls;
using MahApps.Metro.Controls.Dialogs;
using System.Collections.ObjectModel;
using System.Windows.Data;
using ControlzEx.Theming;

namespace FrtvGUI.Views
{
    public class FlyoutValidation : IDataErrorInfo
    {
        private string extensionRequiredProperty = "";
        public string ExtensionRequiredProperty
        {
            get => this.extensionRequiredProperty;
            set
            {
                extensionRequiredProperty = value;
            }
        }

        private string maximumFileSizeRequiredProperty = "";
        public string MaximumFileSizeRequiredProperty
        {
            get => this.maximumFileSizeRequiredProperty;
            set
            {
                maximumFileSizeRequiredProperty = value;
            }
        }

        private string expirationYearRequiredProperty = "";
        public string ExpirationYearRequiredProperty
        {
            get => this.expirationYearRequiredProperty;
            set
            {
                expirationYearRequiredProperty = value;
            }
        }

        private string expirationDayRequiredProperty = "";
        public string ExpirationDayRequiredProperty
        {
            get => this.expirationDayRequiredProperty;
            set
            {
                expirationDayRequiredProperty = value;
            }
        }

        private string expirationHourRequiredProperty = "";
        public string ExpirationHourRequiredProperty
        {
            get => this.expirationHourRequiredProperty;
            set
            {
                expirationHourRequiredProperty = value;
            }
        }

        private string expirationMinuteRequiredProperty = "";
        public string ExpirationMinuteRequiredProperty
        {
            get => this.expirationMinuteRequiredProperty;
            set
            {
                expirationMinuteRequiredProperty = value;
            }
        }

        public bool IsOver10Years(int year, int day, int hour, int minute)
        {
            return ((year * 31536000) + (day * 86400) + (hour * 3600) + (minute * 60) > 315360000);
        }

        public string this[string columnName]
        {
            get
            {
                string result = "";
                if (string.Equals(columnName, "ExtensionRequiredProperty"))
                {
                    if (string.IsNullOrEmpty(extensionRequiredProperty))
                        result = "해당 필드는 반드시 입력해야합니다.";
                }
                else if (string.Equals(columnName, "MaximumFileSizeRequiredProperty"))
                {
                    if (string.IsNullOrEmpty(maximumFileSizeRequiredProperty))
                        result = "해당 필드는 반드시 입력해야합니다.";
                }
                else if (string.Equals(columnName, "ExpirationYearRequiredProperty"))
                {
                    if (string.IsNullOrEmpty(expirationYearRequiredProperty))
                        result = "해당 필드는 반드시 입력해야합니다. (0 ~ 10 사이)";
                }
                else if (string.Equals(columnName, "ExpirationDayRequiredProperty"))
                {
                    if (string.IsNullOrEmpty(expirationDayRequiredProperty))
                        result = "해당 필드는 반드시 입력해야합니다. (0 ~ 364 사이)";
                }
                else if (string.Equals(columnName, "ExpirationHourRequiredProperty"))
                {
                    if (string.IsNullOrEmpty(expirationHourRequiredProperty))
                        result = "해당 필드는 반드시 입력해야합니다. (0 ~ 23 사이)";
                }
                else if (string.Equals(columnName, "ExpirationMinuteRequiredProperty"))
                {
                    if (string.IsNullOrEmpty(expirationMinuteRequiredProperty))
                        result = "해당 필드는 반드시 입력해야합니다. (5 ~ 59 사이)";
                }

                return result;
            }
        }

        public string Error
        {
            get { return "ERROR"; }
        }
    }
}