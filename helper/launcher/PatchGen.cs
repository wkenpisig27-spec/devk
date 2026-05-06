using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Security.Cryptography;
using System.Text;

namespace SlimePiratesPatchGen
{
    class Program
    {
        static void Main(string[] args)
        {
            Console.WriteLine("=== Slime Pirates Patch Generator ===\n");
            
            if (args.Length < 1)
            {
                Console.WriteLine("Usage: PatchGen.exe <client_directory>");
                Console.WriteLine("Example: PatchGen.exe C:\\game\\client");
                return;
            }

            string clientDir = args[0];
            if (!Directory.Exists(clientDir))
            {
                Console.WriteLine("ERROR: Directory not found: " + clientDir);
                return;
            }

            try
            {
                GeneratePatchFiles(clientDir);
                Console.WriteLine("\n✓ Patch files generated successfully!");
            }
            catch (Exception ex)
            {
                Console.WriteLine("\nERROR: " + ex.Message);
            }
        }

        static void GeneratePatchFiles(string clientDir)
        {
            Console.WriteLine("Scanning directory: " + clientDir + "\n");

            // Get all files recursively
            var allFiles = Directory.GetFiles(clientDir, "*.*", SearchOption.AllDirectories)
                .Where(f => !f.Contains("\\updater\\", StringComparison.OrdinalIgnoreCase) && 
                            !f.Contains("\\log\\", StringComparison.OrdinalIgnoreCase) &&
                            !f.Contains("\\log1\\", StringComparison.OrdinalIgnoreCase) &&
                            !f.Contains("\\log2\\", StringComparison.OrdinalIgnoreCase) &&
                            !f.Contains("\\user\\", StringComparison.OrdinalIgnoreCase) &&
                            !f.Contains("\\patcher\\", StringComparison.OrdinalIgnoreCase) &&
                            !f.EndsWith(".log", StringComparison.OrdinalIgnoreCase) &&
                            !f.EndsWith(".cfg", StringComparison.OrdinalIgnoreCase) && 
                            !f.EndsWith("PatchGen.exe", StringComparison.OrdinalIgnoreCase) &&
                            !f.EndsWith("Slime Pirates.exe", StringComparison.OrdinalIgnoreCase) &&
                            !f.EndsWith("SlimePiratesLauncher.exe", StringComparison.OrdinalIgnoreCase))
                .ToList();

            Console.WriteLine("Found " + allFiles.Count + " files to process...\n");

            List<FileEntry> fileList = new List<FileEntry>();

            for (int i = 0; i < allFiles.Count; i++)
            {
                string fullPath = allFiles[i];
                string relativePath = fullPath.Substring(clientDir.Length + 1).Replace("\\", "/");
                
                FileInfo info = new FileInfo(fullPath);
                string hash = ComputeMD5(fullPath);

                fileList.Add(new FileEntry
                {
                    Path = relativePath,
                    Size = info.Length,
                    Hash = hash
                });

                if ((i + 1) % 100 == 0 || i == allFiles.Count - 1)
                {
                    Console.WriteLine("Processing: " + (i + 1) + "/" + allFiles.Count + " - " + relativePath);
                }
            }

            // Generate ver.dat (MD5 of all file hashes combined)
            string versionHash = ComputeVersionHash(fileList);
            
            // Create patcher/updater directory structure for server upload
            string patcherDir = Path.Combine(clientDir, "patcher");
            string updaterDir = Path.Combine(patcherDir, "updater");
            Directory.CreateDirectory(updaterDir);

            // Write ver.dat
            string verDatPath = Path.Combine(updaterDir, "ver.dat");
            File.WriteAllText(verDatPath, versionHash);
            Console.WriteLine("\n✓ Created: " + verDatPath);
            Console.WriteLine("  Version Hash: " + versionHash);

            // Write vercomp.dat (binary format)
            string vercompDatPath = Path.Combine(updaterDir, "vercomp.dat");
            WriteVercompDat(vercompDatPath, fileList);
            Console.WriteLine("✓ Created: " + vercompDatPath);
            Console.WriteLine("  Files: " + fileList.Count);

            // Copy all client files to patcher directory for server upload
            Console.WriteLine("\nCopying files to patcher directory...");
            foreach (var file in fileList)
            {
                string sourcePath = Path.Combine(clientDir, file.Path.Replace("/", "\\"));
                string destPath = Path.Combine(patcherDir, file.Path.Replace("/", "\\"));
                string destDir = Path.GetDirectoryName(destPath);
                
                if (!Directory.Exists(destDir))
                    Directory.CreateDirectory(destDir);
                
                File.Copy(sourcePath, destPath, true);
            }
            Console.WriteLine("✓ Copied " + fileList.Count + " files to: " + patcherDir);
            
            Console.WriteLine("\n========================================");
            Console.WriteLine("Upload the entire 'patcher' folder to your web server root.");
            Console.WriteLine("Server structure should be: http://yourserver.com/patcher/");
            Console.WriteLine("========================================");
        }

        static void WriteVercompDat(string path, List<FileEntry> files)
        {
            using (FileStream fs = new FileStream(path, FileMode.Create))
            using (BinaryWriter bw = new BinaryWriter(fs))
            {
                // Write file count
                bw.Write((uint)files.Count);

                // Write each file entry
                foreach (var file in files)
                {
                    // Path length and path
                    byte[] pathBytes = Encoding.UTF8.GetBytes(file.Path);
                    bw.Write((ushort)pathBytes.Length);
                    bw.Write(pathBytes);

                    // File size
                    bw.Write((ulong)file.Size);

                    // MD5 hash (32 characters)
                    byte[] hashBytes = Encoding.ASCII.GetBytes(file.Hash);
                    bw.Write(hashBytes); // 32 bytes
                }
            }
        }

        static string ComputeMD5(string filePath)
        {
            using (var md5 = MD5.Create())
            using (var stream = File.OpenRead(filePath))
            {
                byte[] hash = md5.ComputeHash(stream);
                return BitConverter.ToString(hash).Replace("-", "").ToLowerInvariant();
            }
        }

        static string ComputeVersionHash(List<FileEntry> files)
        {
            // Combine all file hashes and compute MD5
            StringBuilder combined = new StringBuilder();
            foreach (var file in files.OrderBy(f => f.Path))
            {
                combined.Append(file.Hash);
            }

            using (var md5 = MD5.Create())
            {
                byte[] hash = md5.ComputeHash(Encoding.UTF8.GetBytes(combined.ToString()));
                return BitConverter.ToString(hash).Replace("-", "").ToLowerInvariant();
            }
        }

        class FileEntry
        {
            public string Path { get; set; }
            public long Size { get; set; }
            public string Hash { get; set; }
        }
    }
}
