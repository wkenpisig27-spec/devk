# Slime Pirates Online Launcher (C# Version)

## Overview
This is a modern C# WPF launcher for Slime Pirates Online, ported from the original C++ launcher with enhanced design and functionality.

## Features

### Core Functionality (Ported from C++)
- ✅ **Auto-Update System**: Downloads and applies game updates automatically
- ✅ **File Integrity Check**: Verifies all game files are correct and not corrupted
- ✅ **Auto-Repair**: Automatically repairs corrupted or missing files
- ✅ **Configuration Support**: Reads URLs from `Updater.cfg` file
- ✅ **Binary File Format**: Compatible with the original updater server format
- ✅ **MD5 Hash Verification**: Ensures file integrity during downloads

### Enhanced UI Features (C# Improvements)
- ✨ **Modern WPF Design**: Beautiful gradient buttons with animations
- ✨ **Video Background**: Supports MP4 background video playback
- ✨ **Two-Panel Layout**: Left panel for controls, right panel for news
- ✨ **Integrated Web Browser**: Displays news directly in the launcher
- ✨ **Smooth Animations**: Button hover effects and transitions
- ✨ **Color-Coded Status**: Green for success, orange for errors
- ✨ **Glass-Style Buttons**: Semi-transparent modern button design

## Configuration

The launcher reads settings from `Updater.cfg`:

```ini
[server]
update_url=http://your-server.com/
news_url=http://your-server.com/news.html
reg_url=http://your-server.com/register
web_url=http://your-server.com/
```

## File Structure

### Required Files
- `Updater.cfg` - Configuration file with server URLs
- `Updater/ver.dat` - Current version hash
- `Updater/vercomp.dat` - File list with hashes and sizes
- `Updater/repair.dat` - List of files to repair (created by Check function)
- `updater/assets/bg.mp4` - Background video (optional)

### Server File Format
The updater expects binary files in this format:

**ver.dat**: Plain text MD5 hash of the current version

**vercomp.dat** (Binary):
```
[4 bytes] Number of files (uint32)
For each file:
  [2 bytes] Path length (uint16)
  [n bytes] Path string
  [4 bytes] File size (uint32)
  [33 bytes] MD5 hash (32 chars + null terminator)
```

## Buttons

1. **PLAY**: Launches the game (system/Game.exe)
2. **WEBSITE**: Opens the main website
3. **REGISTER**: Opens the registration page
4. **CHECK FILES**: Verifies game file integrity
5. **EXIT**: Closes the launcher

## Update Process

### Normal Update (Automatic on launch)
1. Connects to update server
2. Downloads version hash (ver.dat)
3. Compares with local version
4. If different, downloads file list (vercomp.dat)
5. Compares server files with local files
6. Downloads new/modified files
7. Deletes obsolete files
8. Saves new version information

### File Check (Manual via CHECK FILES button)
1. Downloads server file list
2. Checks each local file:
   - Exists?
   - Correct size?
   - Correct MD5 hash?
3. If corrupted files found:
   - Saves list to repair.dat
   - Prompts user to restart

### Repair Mode (Automatic if repair.dat exists)
1. Reads repair.dat file list
2. Downloads all listed files
3. Updates version information
4. Deletes repair.dat
5. Ready to play

## Technical Details

### Classes

**ConfigParser.cs**: INI file parser for Updater.cfg
**UpdaterLogic.cs**: Main update/check/repair logic
**MainWindow.xaml.cs**: UI controller and event handlers

### Key Methods

```csharp
// Start operations
StartUpdate()      // Normal update process
StartCheck()       // File integrity check
StartRepair()      // Repair corrupted files

// Helper methods
ParseFilesList()   // Parse binary file list
DeleteFiles()      // Remove obsolete files
DownloadFiles()    // Download new/updated files
SaveRepairList()   // Save corrupted file list
GetMD5()          // Calculate file MD5 hash
```

## Building

1. Open `launcher_csharp.csproj` in Visual Studio 2019+
2. Build for Release configuration
3. Output will be in `bin/Release/net472/`
4. Copy `Updater.cfg` to the output directory
5. Optionally add `updater/assets/bg.mp4` for background video

## Differences from C++ Version

### Improvements
- ✅ Modern WPF UI instead of Win32
- ✅ Integrated news browser
- ✅ Async/await for better responsiveness
- ✅ No DirectShow complexity (uses MediaElement)
- ✅ Easier to customize and maintain
- ✅ Better error handling with colored messages

### Compatibility
- ✅ 100% compatible with original server format
- ✅ Uses same binary file formats
- ✅ Same MD5 verification
- ✅ Same update algorithm

## Requirements

- .NET Framework 4.7.2 or higher
- Windows 7 or higher
- Internet connection for updates

## License

Same as original C++ launcher project.
