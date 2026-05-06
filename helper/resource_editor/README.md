# PKODev Resource Editor

A lightweight editor for the tab-delimited `.txt` resource files in `server/resource/`.  
Available in two modes: **browser** (original) and **native Windows desktop app**.

---

## Quick Start

### Browser mode (original)
Double-click `start.bat` — installs dependencies and opens the editor at `http://localhost:5500`.

### Windows desktop app *(native window, no browser needed)*
Double-click `start_win.bat` — installs dependencies and opens a native Edge WebView2 window.

Requires **Python 3.10+** and **Windows 10 / 11** (WebView2 is built-in).

---

## Build a Standalone EXE

Run `build.bat` once to produce `dist\ResourceEditor\ResourceEditor.exe` via PyInstaller.  
The entire `dist\ResourceEditor\` folder must stay at the same relative depth as `server\resource\`
(i.e. in place of `helper\resource_editor\`), or adjust `RESOURCE_DIR` in `app.py`.

```batch
# One-command build
build.bat
```

---

## Files

| File | Purpose |
|------|---------|
| `app.py` | Flask backend (shared by both modes) |
| `app_win.py` | Windows desktop entry-point (pywebview wrapper) |
| `start.bat` | Dev launcher – browser mode |
| `start_win.bat` | Dev launcher – native window mode |
| `build.bat` | PyInstaller build script → `ResourceEditor.exe` |
| `requirements.txt` | Browser-mode dependencies (`flask`) |
| `requirements_win.txt` | Windows-app dependencies (`flask`, `pywebview`, `pyinstaller`) |

---

## Features

- **Sidebar** — browse all `.txt` files with sizes
- **Table view** — paginated grid with inline cell editing (click any cell)
- **Raw view** — plain text editing for non-tabular files
- **Search** — filter rows by text, optionally scoped to a specific column
- **Sort** — click any column header to sort (current page)
- **Save** — `Ctrl+S` or the Save button; unsaved changes show "UNSAVED" badge
- **Pagination** — 100 / 200 / 500 / 1000 rows per page

---

## Keyboard Shortcuts

| Key | Action |
|-----|--------|
| `Ctrl+S` | Save file |
| `Enter` (in cell) | Commit edit, move to next row |
| `Tab` / `Shift+Tab` | Move to next/prev column |
| `Escape` (in cell) | Cancel edit |

---

## Manual Start

```powershell
cd helper\resource_editor

# Browser mode
pip install -r requirements.txt
python app.py

# Windows native window
pip install -r requirements_win.txt
python app_win.py
```
