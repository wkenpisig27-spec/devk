using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace SlimePiratesLauncher
{
    /// <summary>
    /// Simple INI parser for Updater.cfg
    /// </summary>
    public class ConfigParser
    {
        private Dictionary<string, Dictionary<string, string>> _data;

        public ConfigParser()
        {
            _data = new Dictionary<string, Dictionary<string, string>>(StringComparer.OrdinalIgnoreCase);
        }

        public bool Load(string filePath)
        {
            try
            {
                if (!File.Exists(filePath))
                    return false;

                string[] lines = File.ReadAllLines(filePath, Encoding.UTF8);
                string currentSection = "";

                foreach (string rawLine in lines)
                {
                    string line = rawLine.Trim();

                    // Skip empty lines and comments
                    if (string.IsNullOrEmpty(line) || line.StartsWith(";") || line.StartsWith("#"))
                        continue;

                    // Section header [section]
                    if (line.StartsWith("[") && line.EndsWith("]"))
                    {
                        currentSection = line.Substring(1, line.Length - 2).ToLower();
                        if (!_data.ContainsKey(currentSection))
                            _data[currentSection] = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
                        continue;
                    }

                    // Key=Value
                    int eqPos = line.IndexOf('=');
                    if (eqPos > 0)
                    {
                        string key = line.Substring(0, eqPos).Trim().ToLower();
                        string value = line.Substring(eqPos + 1).Trim();

                        // Remove quotes if present
                        if (value.Length >= 2 && value.StartsWith("\"") && value.EndsWith("\""))
                            value = value.Substring(1, value.Length - 2);

                        if (!_data.ContainsKey(currentSection))
                            _data[currentSection] = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);

                        _data[currentSection][key] = value;
                    }
                }

                return true;
            }
            catch
            {
                return false;
            }
        }

        public string GetString(string section, string key, string defaultValue = "")
        {
            if (_data.ContainsKey(section) && _data[section].ContainsKey(key))
                return _data[section][key];
            return defaultValue;
        }

        public int GetInt(string section, string key, int defaultValue = 0)
        {
            string value = GetString(section, key, "");
            if (int.TryParse(value, out int result))
                return result;
            return defaultValue;
        }

        public bool GetBool(string section, string key, bool defaultValue = false)
        {
            string value = GetString(section, key, "").ToLower();
            if (value == "true" || value == "1" || value == "yes")
                return true;
            if (value == "false" || value == "0" || value == "no")
                return false;
            return defaultValue;
        }
    }
}
