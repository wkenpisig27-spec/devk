"""
PKODev Resource Editor - Windows Desktop App
=============================================
Wraps the Flask web app in a native Windows window via pywebview (Edge WebView2).

Usage:
    python app_win.py          # Run in dev mode (requires pip install -r requirements_win.txt)
    build.bat                  # Build a standalone ResourceEditor.exe with PyInstaller

Requirements:
    pip install -r requirements_win.txt
"""

import sys
import os
import threading
import socket
import time

# ─── Frozen-bundle path fix ───────────────────────────────────────────────────
# When packaged by PyInstaller (--onedir), sys.frozen is True.
# We set BASE_DIR so that RESOURCE_DIR in app.py resolves correctly relative
# to the .exe location rather than the PyInstaller temp extraction dir.
if getattr(sys, "frozen", False):
    # Running as compiled .exe  →  BASE_DIR = folder containing the .exe
    BASE_DIR = os.path.dirname(sys.executable)
    # Add bundle dir to path so Flask templates are found
    sys.path.insert(0, sys._MEIPASS)
    os.chdir(BASE_DIR)

import webview  # pywebview – must be imported after path fix

# Import the Flask application (we control when it starts, not at import time)
from app import app as flask_app

# ─── Config ───────────────────────────────────────────────────────────────────
APP_TITLE = "PKODev Resource Editor"
PREFERRED_PORT = 5500
WINDOW_WIDTH = 1400
WINDOW_HEIGHT = 860
WINDOW_MIN_W = 960
WINDOW_MIN_H = 580


# ─── Helpers ──────────────────────────────────────────────────────────────────

def find_free_port(start: int = PREFERRED_PORT, tries: int = 20) -> int:
    """Return the first unused TCP port starting from *start*."""
    for port in range(start, start + tries):
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            try:
                s.bind(("127.0.0.1", port))
                return port
            except OSError:
                continue
    return start  # hope for the best


def wait_for_server(port: int, timeout: float = 10.0) -> bool:
    """Poll localhost:port until a connection succeeds or *timeout* expires."""
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            with socket.create_connection(("127.0.0.1", port), timeout=0.3):
                return True
        except (ConnectionRefusedError, OSError):
            time.sleep(0.05)
    return False


def run_flask(port: int) -> None:
    """Start the Flask development server (blocking, runs in daemon thread)."""
    flask_app.run(
        host="127.0.0.1",
        port=port,
        debug=False,
        use_reloader=False,
        threaded=True,
    )


# ─── Entry point ──────────────────────────────────────────────────────────────

def main() -> None:
    port = find_free_port()

    # Start Flask in a background thread (daemon → auto-killed on exit)
    flask_thread = threading.Thread(target=run_flask, args=(port,), daemon=True)
    flask_thread.start()

    # Wait until Flask is ready to accept connections
    if not wait_for_server(port):
        # Server didn't come up in time – fall back to system browser
        import webbrowser
        webbrowser.open(f"http://127.0.0.1:{port}")
        flask_thread.join()
        return

    # Create the native window (Edge WebView2 on Windows 10 / 11)
    window = webview.create_window(
        title=APP_TITLE,
        url=f"http://127.0.0.1:{port}",
        width=WINDOW_WIDTH,
        height=WINDOW_HEIGHT,
        min_size=(WINDOW_MIN_W, WINDOW_MIN_H),
        resizable=True,
        text_select=True,
    )

    # gui='edgechromium' uses the built-in WebView2 runtime (Win 10/11)
    # Fallback to 'mshtml' on very old Windows (rare)
    try:
        webview.start(gui="edgechromium", debug=False)
    except Exception:
        webview.start(debug=False)  # auto-select best available backend


if __name__ == "__main__":
    main()
