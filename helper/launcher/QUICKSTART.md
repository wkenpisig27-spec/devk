# 🚀 Quick Start Guide

## For Developers

### Building the Launcher

1. **Open the project**
   ```
   Double-click: launcher_csharp.csproj
   OR
   Open in Visual Studio 2019+
   ```

2. **Build**
   - Select "Release" configuration
   - Build > Build Solution (Ctrl+Shift+B)
   - Output: `bin\Release\net472\SlimePiratesLauncher.exe`

3. **Test locally**
   - Ensure `Updater.cfg` is in the same folder as .exe
   - Run the launcher

### Configuration

Edit `Updater.cfg`:
```ini
[server]
update_url=http://your-server.com/
news_url=http://your-server.com/news.html
reg_url=http://your-server.com/register
web_url=http://your-server.com/
```

### Optional: Add Background Video
1. Create folder: `updater\assets\`
2. Place video: `updater\assets\bg.mp4`
3. Recommended: 1920x1080, MP4 format

---

## For Server Administrators

### Server File Structure
```
your-server.com/
├── Updater/
│   ├── ver.dat          (version hash)
│   └── vercomp.dat      (file list with hashes)
├── system/
│   └── Game.exe         (and other game files)
└── news.html            (news page)
```

### Creating ver.dat
```bash
# Calculate MD5 of vercomp.dat
md5sum vercomp.dat > ver.dat
# OR use any MD5 hash as version identifier
```

### Creating vercomp.dat
Use a tool to generate binary format:
```
[uint32] file_count
For each file:
  [uint16] path_length
  [string] path (e.g., "system/Game.exe")
  [uint32] file_size
  [33 bytes] md5_hash + null
```

---

## For End Users

### Installation
1. Download launcher package
2. Extract to game folder
3. Run `SlimePiratesLauncher.exe`

### First Run
- Launcher will download/update game files
- Progress bar shows download status
- Wait until "Play!" button is enabled

### Features

**PLAY Button** - Launch the game
**WEBSITE Button** - Open game website
**REGISTER Button** - Create account
**CHECK FILES Button** - Verify game integrity
**EXIT Button** - Close launcher

### Troubleshooting

**"Could not connect to server"**
- Check internet connection
- Verify Updater.cfg URLs
- Contact server administrator

**"Corrupted files found"**
- Click "CHECK FILES" button
- Restart launcher
- Files will auto-repair

**"Game.exe not found"**
- Wait for update to complete
- Check `system/` folder exists
- Re-run file check

---

## Features at a Glance

### Automatic Updates ✅
- Checks version on startup
- Downloads only changed files
- Removes obsolete files
- Shows progress

### File Verification ✅
- MD5 hash checking
- Size validation
- Automatic repair

### News Integration ✅
- Built-in browser
- Latest announcements
- No need to open website

### Modern Design ✅
- Animated buttons
- Video background
- Glass effects
- Smooth transitions

---

## Development Tips

### Customizing the UI
- Edit `MainWindow.xaml` for layout
- Modify button styles in Window.Resources
- Change colors in gradient definitions

### Adding Features
- Updater logic: `Updater.cs`
- UI handlers: `MainWindow.xaml.cs`
- Config parsing: `ConfigParser.cs`

### Debugging
- Use Visual Studio debugger
- Check status messages
- Monitor network requests
- Verify file paths

---

## Support

For issues or questions:
1. Check IMPLEMENTATION_STATUS.md
2. Review README.md
3. Examine UI_LAYOUT_GUIDE.md
4. Check C++ original implementation

---

## Version History

**v2.0 (Current - C# Version)**
- Complete rewrite in C# WPF
- Modern UI design
- News browser integration
- All C++ features ported
- Enhanced error handling

**v1.0 (C++ Version)**
- Original Win32 implementation
- Basic GUI
- Core update functionality

---

**Ready to launch! 🎮**
