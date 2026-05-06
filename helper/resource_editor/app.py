"""
PKODev Resource Editor
Flask web app for editing .txt resource files in server/resource/
"""

import os
import sys
import json
import re
from pathlib import Path
from flask import Flask, render_template, request, jsonify, send_from_directory

app = Flask(__name__)

# Resource directory - works both in dev mode and when frozen by PyInstaller.
# When frozen (--onedir), sys.executable lives next to ../../server/resource.
# When running from source, __file__ is used as the anchor.
if getattr(sys, "frozen", False):
    # Running as a PyInstaller bundle; exe is at helper/resource_editor/
    SCRIPT_DIR = Path(sys.executable).parent
else:
    SCRIPT_DIR = Path(__file__).parent

RESOURCE_DIR = (SCRIPT_DIR / "../../server/resource").resolve()

# ─── Display-friendly column names ───────────────────────────────────────────
# Maps raw header names (as in .txt) → readable display names for the editor UI.
# The original file headers are NEVER modified; this is UI-only.
COLUMN_DISPLAY_NAMES = {
    # ItemInfo.txt columns
    "Model1": "Model 1", "Model2": "Model 2", "Model3": "Model 3",
    "Model4": "Model 4", "Model5": "Model 5",
    "ShipFlag": "Ship Flag", "ShipType": "Ship Type",
    "PrefixChance": "Prefix Chance", "SetNum": "Set Number",
    "ForgeLv": "Forge Level", "ForgeSteady": "Forge Steady",
    "ExclusiveID": "Exclusive ID",
    "IsTrade": "Is Trade", "IsPick": "Is Pick", "IsThrow": "Is Throw",
    "IsDel": "Is Delete", "PileMax": "Pile Max",
    "NeedLv": "Need Level", "TitleReq": "Title Req", "FameReq": "Fame Req",
    "AbleLink": "Able Link", "NeedLink": "Need Link", "PickTo": "Pick To",
    "StrCoef": "Str Coef", "AgiCoef": "Agi Coef", "DexCoef": "Dex Coef",
    "ConCoef": "Con Coef", "StaCoef": "Sta Coef", "LukCoef": "Luk Coef",
    "ASpdCoef": "ASpd Coef", "ADisCoef": "ADis Coef",
    "MnAtkCoef": "Min Atk Coef", "MxAtkCoef": "Max Atk Coef",
    "DefCoef": "Def Coef", "MxHpCoef": "Max HP Coef", "MxSpCoef": "Max SP Coef",
    "FleeCoef": "Flee Coef", "HitCoef": "Hit Coef", "CrtCoef": "Crit Coef",
    "MfCoef": "MF Coef", "HRecCoef": "HP Rec Coef", "SRecCoef": "SP Rec Coef",
    "MSpdCoef": "MSpd Coef", "ColCoef": "Col Coef",
    "StrValu": "Str Value", "AgiValu": "Agi Value", "DexValu": "Dex Value",
    "ConValu": "Con Value", "StaValu": "Sta Value", "LukValu": "Luk Value",
    "ASpdValu": "ASpd Value", "ADisValu": "ADis Value",
    "MnAtkValu": "Min Atk Value", "MxAtkValu": "Max Atk Value",
    "DefValu": "Def Value", "MxHpValu": "Max HP Value", "MxSpValu": "Max SP Value",
    "FleeValu": "Flee Value", "HitValu": "Hit Value", "CrtValu": "Crit Value",
    "MfValu": "MF Value", "HRecValu": "HP Rec Value", "SRecValu": "SP Rec Value",
    "MSpdValu": "MSpd Value", "ColValu": "Col Value",
    "PDefValu": "PDef Value", "LHandValu": "LHand Value",
    "ShipEndureRecover": "Ship Endure Recover",
    "ShipCannonCount": "Ship Cannon Count", "ShipCrewCount": "Ship Crew Count",
    "ShipCrewStandard": "Ship Crew Standard",
    "ShipCargoCapacity": "Ship Cargo Capacity",
    "ShipSupplyConsume": "Ship Supply Consume",
    "ShipCannonFlySpeed": "Ship Cannon Fly Speed",
    "ShipMoveSpeed": "Ship Move Speed",
    "AttrEffect": "Attr Effect", "Drap": "Drop",
    "BindEffects": "Bind Effects", "BindEffectDummy": "Bind Effect Dummy",
    "ItemEffect": "Item Effect", "AreaEffect": "Area Effect",
    "UseItemEffect": "Use Item Effect",
}


def get_display_headers(raw_headers):
    """Return display-friendly header names (for UI only). Original names untouched in file."""
    return [COLUMN_DISPLAY_NAMES.get(h, h) for h in raw_headers]


def get_txt_files():
    """Return sorted list of .txt files in the resource directory."""
    files = []
    for f in sorted(RESOURCE_DIR.iterdir()):
        if f.is_file() and f.suffix.lower() == ".txt":
            size = f.stat().st_size
            files.append({
                "name": f.name,
                "size": size,
                "size_kb": round(size / 1024, 1),
            })
    return files


def parse_tab_file(path: Path):
    """
    Parse a tab-delimited PKO resource file.
    First non-empty line starting with '//' is the header.
    Returns dict: {headers, rows, has_header, total, row_line_map}

    row_line_map: list mapping data-row-index → line-index in the file,
    so we can surgically replace lines on save without touching the rest.
    """
    raw_lines = path.read_text(encoding="utf-8-sig", errors="replace").splitlines()

    headers = []
    rows = []
    row_line_map = []   # data row index → original file line index
    has_header = False

    # Detect if it's a tab-delimited file with // header
    for line in raw_lines:
        stripped = line.strip()
        if stripped == "":
            continue
        if stripped.startswith("//"):
            tab_count = stripped.count("\t")
            if tab_count >= 2:
                has_header = True
            break
        else:
            break

    if not has_header:
        sample = "\n".join(raw_lines[:20])
        if sample.count("\t") < 3:
            has_header = False

    if has_header:
        header_found = False
        for i, line in enumerate(raw_lines):
            stripped = line.strip()
            if stripped == "":
                continue
            if not header_found and stripped.startswith("//"):
                header_raw = stripped[2:].lstrip("\t")
                headers = header_raw.split("\t")
                headers = [h.strip() for h in headers]
                header_found = True
                continue
            # Skip comment lines after header
            if stripped.startswith("//") or stripped.startswith(";") or stripped.startswith("#"):
                continue
            if stripped == "":
                continue
            parts = line.rstrip("\r\n").split("\t")
            rows.append(parts)
            row_line_map.append(i)
    else:
        has_header = False

    return {
        "headers": headers,
        "rows": rows,
        "has_header": has_header,
        "total": len(rows),
        "row_line_map": row_line_map,
    }


def read_raw_bytes(path: Path):
    """Read file as raw bytes to detect BOM and line endings."""
    raw = path.read_bytes()
    has_bom = raw[:3] == b"\xef\xbb\xbf"
    if b"\r\n" in raw:
        line_ending = "\r\n"
    elif b"\r" in raw:
        line_ending = "\r"
    else:
        line_ending = "\n"
    return has_bom, line_ending


def save_tab_file(path: Path, headers, rows):
    """
    Save a tab-delimited file, preserving BOM, line endings,
    blank lines, and comment lines from the original.
    Only the header and data rows are updated; everything else is kept.
    """
    has_bom, line_ending = read_raw_bytes(path)
    original_text = path.read_text(encoding="utf-8-sig", errors="replace")
    original_lines = original_text.splitlines()

    # Build the new file from original lines, replacing header + data rows
    data = parse_tab_file(path)

    new_lines = list(original_lines)  # copy

    # Replace header line (find it)
    header_line_idx = None
    for i, line in enumerate(original_lines):
        stripped = line.strip()
        if stripped == "":
            continue
        if stripped.startswith("//") and stripped.count("\t") >= 2:
            header_line_idx = i
            break

    if header_line_idx is not None:
        new_lines[header_line_idx] = "//" + "\t".join(headers)

    # Replace data rows at their original line positions
    row_line_map = data["row_line_map"]
    for ri, row in enumerate(rows):
        if ri < len(row_line_map):
            line_idx = row_line_map[ri]
            new_lines[line_idx] = "\t".join(str(c) for c in row)

    # Handle added rows (more rows than original)
    if len(rows) > len(row_line_map):
        for ri in range(len(row_line_map), len(rows)):
            new_lines.append("\t".join(str(c) for c in rows[ri]))

    # Handle deleted rows (fewer rows than original) - remove from end
    if len(rows) < len(row_line_map):
        # Remove lines for deleted rows (from the end, to keep indices stable)
        lines_to_remove = set()
        for ri in range(len(rows), len(row_line_map)):
            lines_to_remove.add(row_line_map[ri])
        new_lines = [line for i, line in enumerate(new_lines) if i not in lines_to_remove]

    content = line_ending.join(new_lines)
    if not content.endswith(line_ending):
        content += line_ending

    encoding = "utf-8-sig" if has_bom else "utf-8"
    path.write_text(content, encoding=encoding, newline="")


def save_row_in_file(path: Path, row_index: int, new_row: list):
    """
    Save a single row by replacing just that line in the original file.
    Preserves everything else exactly as-is.
    """
    has_bom, line_ending = read_raw_bytes(path)
    original_text = path.read_text(encoding="utf-8-sig", errors="replace")
    original_lines = original_text.splitlines()

    data = parse_tab_file(path)
    if row_index < 0 or row_index >= len(data["row_line_map"]):
        return False

    line_idx = data["row_line_map"][row_index]
    original_lines[line_idx] = "\t".join(str(c) for c in new_row)

    content = line_ending.join(original_lines)
    if not content.endswith(line_ending):
        content += line_ending

    encoding = "utf-8-sig" if has_bom else "utf-8"
    path.write_text(content, encoding=encoding, newline="")
    return True


# ─── Routes ──────────────────────────────────────────────────────────────────

@app.route("/")
def index():
    return render_template("index.html")


@app.route("/api/files")
def api_files():
    return jsonify(get_txt_files())


@app.route("/api/file/<filename>", methods=["GET"])
def api_get_file(filename):
    # Security: only allow simple filenames, no path traversal
    if "/" in filename or "\\" in filename or ".." in filename:
        return jsonify({"error": "Invalid filename"}), 400

    path = RESOURCE_DIR / filename
    if not path.exists():
        return jsonify({"error": "File not found"}), 404

    page = int(request.args.get("page", 1))
    page_size = int(request.args.get("page_size", 200))
    search = request.args.get("search", "").strip().lower()
    col_filter = request.args.get("col", "").strip()  # column name to search in

    data = parse_tab_file(path)

    if not data["has_header"]:
        # Return raw text
        raw = path.read_text(encoding="utf-8-sig", errors="replace")
        return jsonify({"mode": "raw", "content": raw})

    rows = data["rows"]
    headers = data["headers"]
    display_headers = get_display_headers(headers)

    # Track original indices (needed for correct edit targeting after search)
    row_indices = list(range(len(rows)))  # default: identity mapping

    # Apply search filter (search against display headers for column filter)
    if search:
        col_idx = None
        if col_filter:
            # Try matching against display name or raw name
            if col_filter in display_headers:
                col_idx = display_headers.index(col_filter)
            elif col_filter in headers:
                col_idx = headers.index(col_filter)

        filtered = []
        filtered_indices = []
        for i, row in enumerate(rows):
            if col_idx is not None:
                cell = row[col_idx].lower() if col_idx < len(row) else ""
                if search in cell:
                    filtered.append(row)
                    filtered_indices.append(i)
            else:
                # Search all columns
                if any(search in cell.lower() for cell in row):
                    filtered.append(row)
                    filtered_indices.append(i)
        rows = filtered
        row_indices = filtered_indices

    total_filtered = len(rows)
    total_pages = max(1, (total_filtered + page_size - 1) // page_size)
    start = (page - 1) * page_size
    end = start + page_size
    page_rows = rows[start:end]
    page_indices = row_indices[start:end]

    return jsonify({
        "mode": "table",
        "headers": display_headers,
        "rows": page_rows,
        "row_indices": page_indices,
        "page": page,
        "page_size": page_size,
        "total": total_filtered,
        "total_all": data["total"],
        "total_pages": total_pages,
    })


@app.route("/api/file/<filename>", methods=["POST"])
def api_save_file(filename):
    if "/" in filename or "\\" in filename or ".." in filename:
        return jsonify({"error": "Invalid filename"}), 400

    path = RESOURCE_DIR / filename
    if not path.exists():
        return jsonify({"error": "File not found"}), 404

    body = request.get_json()
    if not body:
        return jsonify({"error": "No data"}), 400

    mode = body.get("mode", "table")

    if mode == "raw":
        content = body.get("content", "")
        # Preserve BOM if original had one
        try:
            has_bom, _ = read_raw_bytes(path)
            enc = "utf-8-sig" if has_bom else "utf-8"
        except Exception:
            enc = "utf-8"
        path.write_text(content, encoding=enc)
        return jsonify({"ok": True, "message": f"Saved {filename}"})

    # Table mode: save full file preserving original structure
    headers = body.get("headers", [])
    rows = body.get("rows", [])

    if not headers:
        return jsonify({"error": "No headers provided"}), 400

    save_tab_file(path, headers, rows)
    return jsonify({"ok": True, "message": f"Saved {len(rows)} rows to {filename}"})


@app.route("/api/file/<filename>/row", methods=["POST"])
def api_save_row(filename):
    """Save a single row update (for inline editing without full reload)."""
    if "/" in filename or "\\" in filename or ".." in filename:
        return jsonify({"error": "Invalid filename"}), 400

    path = RESOURCE_DIR / filename
    if not path.exists():
        return jsonify({"error": "File not found"}), 404

    body = request.get_json()
    row_index = body.get("row_index")  # 0-based index in full file
    new_row = body.get("row", [])

    if row_index is None:
        return jsonify({"error": "row_index required"}), 400

    data = parse_tab_file(path)
    if not data["has_header"]:
        return jsonify({"error": "File is not tabular"}), 400

    if row_index < 0 or row_index >= len(data["rows"]):
        return jsonify({"error": f"Row index {row_index} out of range"}), 400

    if not save_row_in_file(path, row_index, new_row):
        return jsonify({"error": "Failed to save row"}), 500
    return jsonify({"ok": True})


@app.route("/api/file/<filename>/patch", methods=["POST"])
def api_patch_rows(filename):
    """Patch only specific rows by their global index. Does NOT rewrite the whole file."""
    if "/" in filename or "\\" in filename or ".." in filename:
        return jsonify({"error": "Invalid filename"}), 400

    path = RESOURCE_DIR / filename
    if not path.exists():
        return jsonify({"error": "File not found"}), 404

    body = request.get_json()
    edits = body.get("edits", {})  # {"rowIndex": [col0, col1, ...], ...}

    if not edits:
        return jsonify({"ok": True, "message": "Nothing to save"})

    # Read original raw bytes to detect BOM and line endings
    raw_bytes = path.read_bytes()
    has_bom = raw_bytes[:3] == b"\xef\xbb\xbf"
    if has_bom:
        raw_bytes = raw_bytes[3:]

    raw_text = raw_bytes.decode("utf-8", errors="replace")

    # Detect line ending
    if "\r\n" in raw_text:
        line_ending = "\r\n"
    elif "\r" in raw_text:
        line_ending = "\r"
    else:
        line_ending = "\n"

    # Split using the same method as parse_tab_file (.splitlines())
    lines = raw_text.splitlines()

    # Build row_line_map the same way parse_tab_file does
    header_found = False
    row_line_map = []
    for i, line in enumerate(lines):
        stripped = line.strip()
        if stripped == "":
            continue
        if not header_found and stripped.startswith("//"):
            if stripped.count("\t") >= 2:
                header_found = True
            continue
        if not header_found:
            continue
        if stripped.startswith("//") or stripped.startswith(";") or stripped.startswith("#"):
            continue
        row_line_map.append(i)

    patched = 0
    for row_idx_str, new_row in edits.items():
        row_idx = int(row_idx_str)
        if 0 <= row_idx < len(row_line_map):
            line_idx = row_line_map[row_idx]
            lines[line_idx] = "\t".join(str(c) for c in new_row)
            patched += 1

    # Rejoin with original line ending
    content = line_ending.join(lines)
    if not content.endswith(line_ending):
        content += line_ending

    # Write back with BOM if original had one
    out_bytes = content.encode("utf-8")
    if has_bom:
        out_bytes = b"\xef\xbb\xbf" + out_bytes
    path.write_bytes(out_bytes)

    return jsonify({"ok": True, "message": f"Patched {patched} rows in {filename}"})


@app.route("/api/file/<filename>/raw", methods=["GET"])
def api_get_raw(filename):
    if "/" in filename or "\\" in filename or ".." in filename:
        return jsonify({"error": "Invalid filename"}), 400
    path = RESOURCE_DIR / filename
    if not path.exists():
        return jsonify({"error": "File not found"}), 404
    content = path.read_text(encoding="utf-8-sig", errors="replace")
    return jsonify({"content": content})


@app.route("/api/file/<filename>/raw", methods=["POST"])
def api_save_raw(filename):
    if "/" in filename or "\\" in filename or ".." in filename:
        return jsonify({"error": "Invalid filename"}), 400
    path = RESOURCE_DIR / filename
    if not path.exists():
        return jsonify({"error": "File not found"}), 404
    body = request.get_json()
    content = body.get("content", "")
    # Preserve BOM if original had one
    try:
        has_bom, _ = read_raw_bytes(path)
        enc = "utf-8-sig" if has_bom else "utf-8"
    except Exception:
        enc = "utf-8"
    path.write_text(content, encoding=enc)
    return jsonify({"ok": True, "message": f"Saved {filename}"})


@app.route("/api/file/<filename>/all", methods=["GET"])
def api_get_all_rows(filename):
    """Get ALL rows for a file (used before save operations)."""
    if "/" in filename or "\\" in filename or ".." in filename:
        return jsonify({"error": "Invalid filename"}), 400
    path = RESOURCE_DIR / filename
    if not path.exists():
        return jsonify({"error": "File not found"}), 404

    data = parse_tab_file(path)
    if not data["has_header"]:
        return jsonify({"error": "File is not tabular"}), 400

    return jsonify({
        "headers": data["headers"],
        "rows": data["rows"],
        "total": data["total"],
    })


if __name__ == "__main__":
    print(f"PKODev Resource Editor")
    print(f"Resource directory: {RESOURCE_DIR}")
    print(f"Open http://localhost:5500 in your browser")
    print()
    app.run(host="127.0.0.1", port=5500, debug=False)
