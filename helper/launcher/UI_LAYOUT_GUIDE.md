# C# Launcher UI Layout Guide

## Window Layout (1000x550px)

```
╔══════════════════════════════════════════════════════════════════════════════╗
║                        Slime Pirates Online Launcher                         ║
╠════════════════════════════════════╦═════════════════════════════════════════╣
║                                    ║                                         ║
║         LEFT PANEL (500px)         ║      RIGHT PANEL (News Browser)         ║
║                                    ║                                         ║
║  ┌──────────────────────────────┐  ║  ┌───────────────────────────────────┐ ║
║  │                              │  ║  │      📰 Latest News              │ ║
║  │   Slime Pirates Online       │  ║  ├───────────────────────────────────┤ ║
║  │     (Glowing Title)          │  ║  │                                   │ ║
║  │                              │  ║  │   [WebBrowser Control]            │ ║
║  │                              │  ║  │   Displays news_url content       │ ║
║  │   🎬 Video Background        │  ║  │   from Updater.cfg                │ ║
║  │                              │  ║  │                                   │ ║
║  │                              │  ║  │   - Server announcements          │ ║
║  │                              │  ║  │   - Update notes                  │ ║
║  │        (Empty Space)         │  ║  │   - Events                        │ ║
║  │                              │  ║  │   - Community news                │ ║
║  │                              │  ║  │                                   │ ║
║  └──────────────────────────────┘  ║  └───────────────────────────────────┘ ║
║                                    ║                                         ║
║  Status: "Checking files..."       ║         Semi-transparent glass          ║
║  ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓░░░░░ 75%         ║         background with border          ║
║                                    ║                                         ║
║  ┌──────────┐ ┌──────────────┐    ║                                         ║
║  │ REGISTER │ │ CHECK FILES  │    ║                                         ║
║  └──────────┘ └──────────────┘    ║                                         ║
║                                    ║                                         ║
║              ┌──────┐ ┌─────────┐ ┌─────────┐                              ║
║              │ EXIT │ │ WEBSITE │ │  PLAY!  │                              ║
║              └──────┘ └─────────┘ └─────────┘                              ║
╚════════════════════════════════════╩═════════════════════════════════════════╝
```

## Button Layout Details

### Top Row (Left Side)
```
┌───────────┐  ┌──────────────┐
│ REGISTER  │  │ CHECK FILES  │
│  100x40   │  │   120x40     │
└───────────┘  └──────────────┘
   Glass          Glass Style
```

### Bottom Row (Right Side)
```
┌────────┐  ┌──────────┐  ┌─────────────┐
│  EXIT  │  │ WEBSITE  │  │    PLAY!    │
│100x40 │  │  100x40  │  │   140x50    │
└────────┘  └──────────┘  └─────────────┘
  Glass       Glass         Slime Green
                           (Larger/Accent)
```

## Color Scheme

### Background
- **Video**: MP4 background video (updater/assets/bg.mp4)
- **Fallback**: Dark gradient (#141E32 → #05050A)

### Buttons
**Play Button:**
- Gradient: #4CDE4C → #00A000 (Green slime)
- Hover: Brighter green + scale 1.05x
- Press: Darker green + translate down
- Glow: White border + inner highlight

**Glass Buttons:**
- Background: #99101015 (semi-transparent dark)
- Border: #40FFFFFF (white 25% opacity)
- Hover: #CC202030 + white border
- Text: #CCCCCC → White on hover

### Status Text
- **Normal**: White
- **Success**: LimeGreen (#00FF00)
- **Error**: Orange (#FFA500)

### Progress Bar
- Track: #80000000 (dark transparent)
- Fill: Orange → Gold gradient (#FFA500 → #FFD700)
- Glow: Gold shadow effect

## Responsive Elements

### Video Background
- Stretches to fill entire window
- Maintains aspect ratio (UniformToFill)
- Loops seamlessly
- Muted audio

### WebBrowser
- Fills right panel dynamically
- Scrollable if content is long
- Maintains 5px margin
- Dark theme friendly

### Progress Bar
- Full width of left panel (minus margins)
- Updates in real-time during operations
- Smooth fill animation

## Status Messages Examples

```
✓ "Game is updated! Press the 'Play!' button to start the game." (Green)
↻ "Downloading: system/Game.exe (5/120)" (White)
⚠ "Error: Could not connect to update server!" (Orange)
🔍 "Checking: assets/texture.dat (89/200)" (White)
```

## Button States

### Play Button States
1. **Disabled** (during update):
   - Opacity: 60%
   - Blur effect
   - Not clickable

2. **Enabled** (ready to play):
   - Full opacity
   - Hover: Scale up + brighten
   - Click: Launch game

### Check Files Button
- Always enabled
- Starts file integrity check
- Shows progress in status bar

## Animations

### Button Hover
- Scale: 1.0 → 1.05 (0.1s ease)
- Brightness increase
- Border brightens

### Button Press
- TranslateY: 0 → 2px
- Darker color
- Instant feedback

### Progress Bar
- Smooth value changes
- Gradient shimmer effect
- Drop shadow animation

## Layout Grid Breakdown

```
Grid (Main)
├─ Column 0 (500px) - Left Panel
│  ├─ Row 0 (*) - Spacer + Title
│  ├─ Row 1 (Auto) - Status Text
│  ├─ Row 2 (Auto) - Progress Bar
│  └─ Row 3 (Auto) - Buttons
│     ├─ Register (Top Left)
│     ├─ Check Files (Top Right)
│     ├─ Exit (Bottom Left)
│     ├─ Website (Bottom Center)
│     └─ Play (Bottom Right)
│
└─ Column 1 (*) - Right Panel
   └─ Border (Glass)
      └─ Grid
         ├─ Row 0 (Auto) - "Latest News" Header
         └─ Row 1 (*) - WebBrowser
```

## Spacing & Margins

- **Window**: 1000x550px
- **Outer margins**: 20px all sides (left panel), 10px right panel
- **Button spacing**: 10px between buttons
- **Status to Progress**: 5px
- **Progress to Buttons**: 20px
- **Title margins**: 30px top
- **News header**: 15px horizontal, 10px vertical
- **WebBrowser**: 5px margin inside border

## Design Philosophy

✨ **Modern & Clean**: Minimal clutter, focus on content
🎮 **Gaming Aesthetic**: Green slime theme, dark colors
📱 **Intuitive**: Clear button labels, visual feedback
⚡ **Responsive**: Smooth animations, instant feedback
🔒 **Professional**: Glass effects, proper spacing
🌐 **Informative**: Integrated news keeps players updated

---

This layout combines the functionality of the C++ version with modern WPF design principles, creating a polished and user-friendly launcher experience! 🚀
