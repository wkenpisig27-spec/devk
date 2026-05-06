using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Net.Http;
using System.Security.Cryptography;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;

namespace SlimePiratesLauncher
{
    public enum UpdaterMode
    {
        Update,
        Check,
        Repair
    }

    public class UpdaterLogic
    {
        private MainWindow _window;
        private HttpClient _http;
        private string _basePath;
        
        // Config values
        public string UpdateUrl { get; private set; } = "http://127.0.0.1/update/";
        public string NewsUrl { get; private set; } = "";
        public string RegUrl { get; private set; } = "";
        public string WebUrl { get; private set; } = "";
        
        private List<FileInfoEntry> _serverFiles;
        private List<FileInfoEntry> _clientFiles;

        public UpdaterLogic(MainWindow window)
        {
            _window = window;
            _http = new HttpClient();
            _http.Timeout = TimeSpan.FromMinutes(5);
            _basePath = AppDomain.CurrentDomain.BaseDirectory;
            _serverFiles = new List<FileInfoEntry>();
            _clientFiles = new List<FileInfoEntry>();
            LoadConfig();
            
            // Apply any pending updates from previous session (files that couldn't be replaced while locked)
            ApplyPendingUpdates();
        }

        private void LoadConfig()
        {
            try 
            {
                string cfgPath = Path.Combine(_basePath, "updater", "updater.cfg");
                string logPath = Path.Combine(_basePath, "log", "config_debug.log");
                Directory.CreateDirectory(Path.Combine(_basePath, "log"));
                
                File.AppendAllText(logPath, 
                    $"[{DateTime.Now}] LoadConfig called\n" +
                    $"Base Path: {_basePath}\n" +
                    $"Config Path: {cfgPath}\n" +
                    $"Config Exists: {File.Exists(cfgPath)}\n");
                
                if (File.Exists(cfgPath))
                {
                    ConfigParser parser = new ConfigParser();
                    if (parser.Load(cfgPath))
                    {
                        UpdateUrl = parser.GetString("server", "update_url", UpdateUrl);
                        NewsUrl = parser.GetString("server", "news_url", NewsUrl);
                        RegUrl = parser.GetString("server", "reg_url", RegUrl);
                        WebUrl = parser.GetString("server", "web_url", WebUrl);
                        
                        File.AppendAllText(logPath, 
                            $"Config loaded successfully\n" +
                            $"UpdateUrl: {UpdateUrl}\n\n");
                    }
                    else
                    {
                        File.AppendAllText(logPath, "Config parser failed\n\n");
                    }
                }
                else
                {
                    File.AppendAllText(logPath, "Config file not found\n\n");
                }
            } 
            catch (Exception ex)
            {
                try
                {
                    string logPath = Path.Combine(_basePath, "log", "config_debug.log");
                    Directory.CreateDirectory(Path.Combine(_basePath, "log"));
                    File.AppendAllText(logPath, $"Exception: {ex.Message}\n\n");
                }
                catch { }
            }
            
            if (!UpdateUrl.EndsWith("/")) UpdateUrl += "/";
        }

        public async void StartUpdate()
        {
            await StartOperation(UpdaterMode.Update);
        }

        public async void StartCheck()
        {
            await StartOperation(UpdaterMode.Check);
        }

        public async void StartRepair()
        {
            await StartOperation(UpdaterMode.Repair);
        }

        private async Task StartOperation(UpdaterMode mode)
        {
            _window.EnablePlay(false);
            _window.SetStatus("Connecting to update server...");
            _window.SetStatusColor(System.Windows.Media.Colors.White);

            // Kill any running game processes before update/repair
            if (mode == UpdaterMode.Update || mode == UpdaterMode.Repair)
            {
                KillGameProcesses();
            }

            await Task.Run(async () => 
            {
                try 
                {
                    switch (mode)
                    {
                        case UpdaterMode.Update:
                            await UpdateClient();
                            break;
                        case UpdaterMode.Check:
                            await CheckClient();
                            break;
                        case UpdaterMode.Repair:
                            await RepairClient();
                            break;
                    }
                }
                catch (Exception ex)
                {
                    _window.SetStatusColor(System.Windows.Media.Colors.Orange);
                    _window.SetStatus("Error: " + ex.Message);
                    
                    // Log full exception details to file for debugging
                    try
                    {
                        string logPath = Path.Combine(_basePath, "log", "updater_error.log");
                        Directory.CreateDirectory(Path.Combine(_basePath, "log"));
                        File.AppendAllText(logPath, 
                            $"[{DateTime.Now}] Error:\n" +
                            $"Message: {ex.Message}\n" +
                            $"Type: {ex.GetType().Name}\n" +
                            $"StackTrace: {ex.StackTrace}\n" +
                            $"InnerException: {ex.InnerException?.Message}\n\n");
                    }
                    catch { }
                    
                    _window.EnablePlay(true);
                }
            });
        }

        private async Task UpdateClient()
        {
            _window.SetStatusColor(System.Windows.Media.Colors.LimeGreen);
            _window.SetStatus("Connecting to the auto-update server...");

            // 1. Get Server Hash
            string serverHash = await _http.GetStringAsync(UpdateUrl + "patcher/updater/ver.dat");
            serverHash = serverHash.Trim();

            // 2. Get Local Hash
            string localHash = "";
            string verPath = Path.Combine(_basePath, "updater", "ver.dat");
            if (File.Exists(verPath)) 
                localHash = File.ReadAllText(verPath).Trim();

            if (serverHash == localHash)
            {
                _window.SetStatus("Game is up to date! Press the \"Play!\" button to start the game.");
                _window.SetProgress(100, 100);
                _window.EnablePlay(true);
                return;
            }

            _window.SetStatus("New version found! Comparing files...");

            // 3. Download vercomp.dat
            byte[] verCompData = await _http.GetByteArrayAsync(UpdateUrl + "patcher/updater/vercomp.dat");
            _serverFiles = ParseFilesList(verCompData);
            
            // 4. Get client files if they exist
            string vercompPath = Path.Combine(_basePath, "updater", "vercomp.dat");
            if (File.Exists(vercompPath))
            {
                byte[] clientVerCompData = File.ReadAllBytes(vercompPath);
                _clientFiles = ParseFilesList(clientVerCompData);
            }
            else
            {
                _clientFiles = new List<FileInfoEntry>();
            }

            // 5. Find files to update and delete
            List<FileInfoEntry> filesToUpdate = new List<FileInfoEntry>();
            List<FileInfoEntry> filesToDelete = new List<FileInfoEntry>();

            // Find files to update (new or different)
            foreach (var serverFile in _serverFiles)
            {
                bool found = false;
                foreach (var clientFile in _clientFiles)
                {
                    if (serverFile.Path == clientFile.Path &&
                        serverFile.Size == clientFile.Size &&
                        serverFile.Hash == clientFile.Hash)
                    {
                        found = true;
                        break;
                    }
                }
                if (!found)
                {
                    filesToUpdate.Add(serverFile);
                }
            }

            // Find files to delete (exist in client but not in server)
            foreach (var clientFile in _clientFiles)
            {
                bool found = false;
                foreach (var serverFile in _serverFiles)
                {
                    if (clientFile.Path == serverFile.Path)
                    {
                        found = true;
                        break;
                    }
                }
                if (!found)
                {
                    filesToDelete.Add(clientFile);
                }
            }

            // Delete obsolete files
            await DeleteFiles(filesToDelete);
            
            // Download new/updated files
            await DownloadFiles(filesToUpdate);

            // 6. Save new Version
            Directory.CreateDirectory(Path.Combine(_basePath, "updater"));
            File.WriteAllText(verPath, serverHash);
            File.WriteAllBytes(vercompPath, verCompData);

            _window.SetStatus("Game is updated! Press the \"Play!\" button to start the game.");
            _window.SetProgress(100, 100);
            _window.EnablePlay(true);
        }

        private async Task CheckClient()
        {
            _window.SetStatusColor(System.Windows.Media.Colors.LimeGreen);
            _window.SetStatus("Checking game files...");

            // Download server files list if not already loaded
            if (_serverFiles.Count == 0)
            {
                byte[] verCompData = await _http.GetByteArrayAsync(UpdateUrl + "patcher/updater/vercomp.dat");
                _serverFiles = ParseFilesList(verCompData);
            }

            List<FileInfoEntry> corruptedFiles = new List<FileInfoEntry>();
            int total = _serverFiles.Count;

            for (int i = 0; i < total; i++)
            {
                var file = _serverFiles[i];
                _window.SetStatus($"Checking: {file.Path} ({i + 1}/{total})");
                _window.SetProgress(i + 1, total);

                // Convert forward slashes to backslashes for local file path
                string localPath = Path.Combine(_basePath, file.Path.Replace("/", "\\"));
                
                // Check if file exists
                if (!File.Exists(localPath))
                {
                    corruptedFiles.Add(file);
                    continue;
                }

                // Check file size
                if ((ulong)new FileInfo(localPath).Length != file.Size)
                {
                    corruptedFiles.Add(file);
                    continue;
                }

                // Check MD5 hash
                if (GetMD5(localPath) != file.Hash)
                {
                    corruptedFiles.Add(file);
                    continue;
                }
            }

            if (corruptedFiles.Count > 0)
            {
                // Save corrupted files list for repair
                SaveRepairList(corruptedFiles);
                _window.SetStatus("Restart the auto-update program to repair the game.");
                MessageBox.Show(
                    "Corrupted files have been found! Restart the auto-update program to repair the game.",
                    "Checking completed",
                    MessageBoxButton.OK,
                    MessageBoxImage.Information);
            }
            else
            {
                _window.SetStatus("The game is not damaged!");
                MessageBox.Show(
                    "The game is not damaged!",
                    "Checking completed",
                    MessageBoxButton.OK,
                    MessageBoxImage.Information);
            }

            _window.EnablePlay(true);
        }

        private async Task RepairClient()
        {
            _window.SetStatusColor(System.Windows.Media.Colors.LimeGreen);
            _window.SetStatus("Repairing the game...");

            string repairPath = Path.Combine(_basePath, "updater", "repair.dat");
            if (!File.Exists(repairPath))
            {
                throw new Exception("Could not open repair file");
            }

            byte[] repairData = File.ReadAllBytes(repairPath);
            List<FileInfoEntry> filesToDownload = ParseFilesList(repairData);

            await DownloadFiles(filesToDownload);

            // Get fresh version info
            string serverHash = await _http.GetStringAsync(UpdateUrl + "patcher/updater/ver.dat");
            byte[] verCompData = await _http.GetByteArrayAsync(UpdateUrl + "patcher/updater/vercomp.dat");

            // Save version files
            string verPath = Path.Combine(_basePath, "updater", "ver.dat");
            string vercompPath = Path.Combine(_basePath, "updater", "vercomp.dat");
            
            Directory.CreateDirectory(Path.Combine(_basePath, "updater"));
            File.WriteAllText(verPath, serverHash);
            File.WriteAllBytes(vercompPath, verCompData);

            // Delete repair file
            File.Delete(repairPath);

            _window.SetStatus("Game is repaired! Press the \"Play!\" button to start the game.");
            _window.SetProgress(100, 100);
            _window.EnablePlay(true);
        }

        private async Task DeleteFiles(List<FileInfoEntry> files)
        {
            if (files.Count == 0) return;

            int total = files.Count;
            for (int i = 0; i < total; i++)
            {
                var file = files[i];
                _window.SetStatus($"Deleting: {file.Path} ({i + 1}/{total})");
                _window.SetProgress(i + 1, total);

                // Convert forward slashes to backslashes for local file path
                string localPath = Path.Combine(_basePath, file.Path.Replace("/", "\\"));
                if (File.Exists(localPath))
                {
                    try
                    {
                        File.Delete(localPath);
                    }
                    catch { /* Ignore delete errors */ }
                }
            }
        }

        private async Task DownloadFiles(List<FileInfoEntry> files)
        {
            if (files.Count == 0) return;

            int total = files.Count;
            for (int i = 0; i < total; i++)
            {
                var file = files[i];
                _window.SetStatus($"Downloading: {file.Path} ({i + 1}/{total})");
                _window.SetProgress(i, total);

                // Convert forward slashes to backslashes for local file path
                string localPath = Path.Combine(_basePath, file.Path.Replace("/", "\\"));
                string dir = Path.GetDirectoryName(localPath);
                if (!Directory.Exists(dir)) 
                    Directory.CreateDirectory(dir);

                // Handle special URL characters (C++ logic replaced backslash with forward slash)
                string urlFile = file.Path.Replace("\\", "/");
                string fullUrl = UpdateUrl + "patcher/" + urlFile;
                
                try
                {
                    byte[] fileData = await _http.GetByteArrayAsync(fullUrl);
                    
                    // Try to write the file with retry logic for locked files
                    bool success = await WriteFileWithRetry(localPath, fileData, file.Path);
                    if (!success)
                    {
                        throw new Exception($"File is locked by another process. Please close the game and try again.");
                    }
                }
                catch (Exception ex)
                {
                    throw new Exception($"Failed to download {file.Path} from {fullUrl}: {ex.Message}", ex);
                }

                _window.SetProgress(i + 1, total);
            }
        }

        /// <summary>
        /// Attempts to write a file with retry logic for locked files.
        /// Will attempt to kill the game process if the file is locked.
        /// </summary>
        private async Task<bool> WriteFileWithRetry(string localPath, byte[] fileData, string displayPath)
        {
            const int maxRetries = 3;
            const int retryDelayMs = 1000;
            
            for (int attempt = 1; attempt <= maxRetries; attempt++)
            {
                try
                {
                    File.WriteAllBytes(localPath, fileData);
                    return true;
                }
                catch (IOException ex) when (IsFileLocked(ex))
                {
                    _window.SetStatus($"File locked: {displayPath} - Attempting to close game... (Attempt {attempt}/{maxRetries})");
                    
                    // Try to kill game processes
                    KillGameProcesses();
                    
                    // Wait a bit for the process to fully terminate
                    await Task.Delay(retryDelayMs);
                    
                    if (attempt == maxRetries)
                    {
                        // Last attempt - try writing to a temp file for later replacement
                        string tempPath = localPath + ".update";
                        try
                        {
                            File.WriteAllBytes(tempPath, fileData);
                            _window.SetStatus($"Saved update for {displayPath} - will be applied on next launch");
                            return true;
                        }
                        catch
                        {
                            return false;
                        }
                    }
                }
            }
            
            return false;
        }

        /// <summary>
        /// Checks if an IOException is due to a file being locked
        /// </summary>
        private bool IsFileLocked(IOException ex)
        {
            int errorCode = System.Runtime.InteropServices.Marshal.GetHRForException(ex) & 0xFFFF;
            return errorCode == 32 || errorCode == 33; // ERROR_SHARING_VIOLATION or ERROR_LOCK_VIOLATION
        }

        /// <summary>
        /// Kills any running game processes (Game.exe) to allow file updates
        /// </summary>
        private void KillGameProcesses()
        {
            string[] processNames = { "Game", "game" };
            
            foreach (var processName in processNames)
            {
                try
                {
                    var processes = Process.GetProcessesByName(processName);
                    foreach (var proc in processes)
                    {
                        try
                        {
                            // Check if this is our game process (in our directory)
                            string procPath = proc.MainModule?.FileName ?? "";
                            if (procPath.Contains(_basePath) || procPath.EndsWith("Game.exe", StringComparison.OrdinalIgnoreCase))
                            {
                                proc.Kill();
                                proc.WaitForExit(3000); // Wait up to 3 seconds
                            }
                        }
                        catch { /* Ignore individual process errors */ }
                        finally
                        {
                            proc.Dispose();
                        }
                    }
                }
                catch { /* Ignore enumeration errors */ }
            }
            
            // Give the system a moment to release file handles
            Thread.Sleep(500);
        }

        /// <summary>
        /// Applies any pending updates (files with .update extension)
        /// Call this on launcher startup before checking for updates
        /// </summary>
        public void ApplyPendingUpdates()
        {
            try
            {
                // Look for any .update files in the base directory and subdirectories
                var updateFiles = Directory.GetFiles(_basePath, "*.update", SearchOption.AllDirectories);
                
                foreach (var updateFile in updateFiles)
                {
                    string targetFile = updateFile.Substring(0, updateFile.Length - 7); // Remove ".update"
                    try
                    {
                        if (File.Exists(targetFile))
                            File.Delete(targetFile);
                        File.Move(updateFile, targetFile);
                    }
                    catch
                    {
                        // Couldn't apply this update - will try again next time
                    }
                }
            }
            catch { /* Ignore errors during pending update application */ }
        }

        private void SaveRepairList(List<FileInfoEntry> files)
        {
            if (files.Count == 0) return;

            string repairPath = Path.Combine(_basePath, "updater", "repair.dat");
            Directory.CreateDirectory(Path.GetDirectoryName(repairPath));

            using (var fs = new FileStream(repairPath, FileMode.Create))
            using (var bw = new BinaryWriter(fs))
            {
                bw.Write((uint)files.Count);

                foreach (var file in files)
                {
                    byte[] pathBytes = Encoding.Default.GetBytes(file.Path);
                    bw.Write((ushort)pathBytes.Length);
                    bw.Write(pathBytes);
                    bw.Write(file.Size);
                    
                    byte[] hashBytes = Encoding.ASCII.GetBytes(file.Hash);
                    bw.Write(hashBytes);
                }
            }
        }

        private List<FileInfoEntry> ParseFilesList(byte[] data)
        {
            List<FileInfoEntry> list = new List<FileInfoEntry>();
            using (var ms = new MemoryStream(data))
            using (var br = new BinaryReader(ms))
            {
                if (data.Length < 4) return list;
                uint count = br.ReadUInt32();
                
                for(int i=0; i<count; i++)
                {
                    if (ms.Position >= ms.Length) break;
                    
                    ushort pathLen = br.ReadUInt16();
                    byte[] pathBytes = br.ReadBytes(pathLen);
                    string path = Encoding.UTF8.GetString(pathBytes);
                    
                    ulong size = br.ReadUInt64();
                    byte[] hashBytes = br.ReadBytes(32);
                    // Hash is 32-char hex string
                    string hash = Encoding.ASCII.GetString(hashBytes);

                    list.Add(new FileInfoEntry { Path = path, Size = size, Hash = hash });
                }
            }
            return list;
        }

        private string GetMD5(string filename)
        {
            using (var md5 = MD5.Create())
            {
                using (var stream = File.OpenRead(filename))
                {
                    var hash = md5.ComputeHash(stream);
                    // Convert to hex string
                    return BitConverter.ToString(hash).Replace("-", "").ToLowerInvariant();
                }
            }
        }

        public class FileInfoEntry
        {
            public string Path { get; set; }
            public ulong Size { get; set; }
            public string Hash { get; set; }
        }
    }
}
