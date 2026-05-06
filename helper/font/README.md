# Bitmap Font Generator for PKODev

## Key Concept

**TTF fonts are vectors** - they can be rendered at ANY size. The filename tells the generator what pixel size to create:

| Filename Contains | Generated Size | Use Case |
|-------------------|----------------|----------|
| `sm` or `small` | 12pt | Dialogs, chat, hints |
| `mid` or `medium` | 14pt | Buttons, menus |
| `big` | 20pt | Titles, player names |
| `huge` | 28pt | Splash screens |
| *(none of above)* | 16pt | Default |

## Quick Start

1. **Get ONE font file** (e.g., `GoogleSansFlex-Regular.ttf`)

2. **Copy it multiple times with different names**:
   ```
   input/
   ├── gamedefaultsm.ttf    ← same TTF, generates at 12pt
   ├── gamedefaultmid.ttf   ← same TTF, generates at 14pt
   ├── gamedefaultbig.ttf   ← same TTF, generates at 20pt
   └── gamedefaulthuge.ttf  ← same TTF, generates at 28pt
   ```

3. **Generate**: Run `generate_simple.bat`

4. **Deploy**: Run `copy_to_client.bat`

5. **Done!** The fonts in `font.clu` work:
   ```lua
   DEFAULT_FONT = UI_CreateFont( "gamedefaultsm", 12, 12, 0 )
   BIGFONT = UI_CreateFont( "gamedefaultbig", 20, 20, 0 )
   ```

## Workflow Example

You downloaded "Poppins-Regular.ttf" and want to use it:

```batch
cd helper\font\input

REM Copy the same file 4 times with different names:
copy "C:\Downloads\Poppins-Regular.ttf" gamedefaultsm.ttf
copy "C:\Downloads\Poppins-Regular.ttf" gamedefaultmid.ttf
copy "C:\Downloads\Poppins-Regular.ttf" gamedefaultbig.ttf
copy "C:\Downloads\Poppins-Regular.ttf" gamedefaulthuge.ttf

cd ..
generate_simple.bat
copy_to_client.bat
```

Or use a Bold variant for the larger sizes:
```batch
copy "C:\Downloads\Poppins-Regular.ttf" gamedefaultsm.ttf
copy "C:\Downloads\Poppins-Regular.ttf" gamedefaultmid.ttf
copy "C:\Downloads\Poppins-Bold.ttf" gamedefaultbig.ttf
copy "C:\Downloads\Poppins-Bold.ttf" gamedefaulthuge.ttf
```

## Why the Same TTF Multiple Times?

The **bitmap font** bakes the characters at a fixed pixel size:
- `gamedefaultsm.fnt` = 12-pixel tall glyphs
- `gamedefaultbig.fnt` = 20-pixel tall glyphs

If you only had one 12pt bitmap font and scaled it to 20pt, it would look blurry. By generating each size separately, you get crisp text at every size.

## Folder Structure

```
helper/font/
├── input/                      ← PUT RENAMED .TTF FILES HERE
│   ├── gamedefaultsm.ttf       ← Will generate at 12pt
│   ├── gamedefaultmid.ttf      ← Will generate at 14pt
│   ├── gamedefaultbig.ttf      ← Will generate at 20pt
│   └── gamedefaulthuge.ttf     ← Will generate at 28pt
├── output/                     ← GENERATED FILES GO HERE
├── generate_simple.bat         ← Run this!
├── copy_to_client.bat          ← Deploy to client
└── bmfont64.exe
```
├── bmfont64.exe
└── README.md
```

## Generated Font Files

The generator creates these font files for the game:

| File | Size | Usage |
|------|------|-------|
| `default.fnt` | 12pt | Default fallback font |
| `gamedefault_12.fnt` | 12pt | Main UI font |
| `gamedefault_13.fnt` | 13pt | Slightly larger UI |
| `gamedefault_14.fnt` | 14pt | Medium UI |
| `gamedefault_16.fnt` | 16pt | Large UI |
| `gamedefault_20.fnt` | 20pt | Headers |
| `gamedefault_28.fnt` | 28pt | Large headers/titles |
| `Arial_12.fnt` | 12pt | Legacy compatibility |

## Workflow

### Simple (Recommended)

```
1. Copy MyFont.ttf → input/
2. Double-click generate_all.bat
3. Double-click copy_to_client.bat
4. Rebuild client
```

### Manual

```
1. Copy MyFont.ttf → input/
2. Run generate_all.bat
3. Review files in output/
4. Copy output/*.fnt and output/*.png → client/font/
5. Rebuild client
```

## Troubleshooting

### "No font files found"
- Make sure you put your `.ttf` or `.otf` file in the `input/` folder
- Only one font file is recommended at a time

### "BMFont not found"
- Download BMFont from: https://www.angelcode.com/products/bmfont/
- Copy `bmfont64.exe` to this folder

### Fonts look wrong in game
- Try a different font file
- Some fonts may not have all required characters
- Check that all `.png` texture files were copied

### Missing characters
- The default config includes ASCII (32-126) and Extended Latin (160-255)
- For special characters, edit the `chars=` line in the generated config
- For Chinese/CJK support, add Unicode ranges: `chars=32-126,19968-40959`

## Advanced: Manual Configuration

If you need fine-tuned control, use the configs in `bmfont_config/`:

1. Open BMFont GUI: `bmfont64.exe`
2. Load a config: File → Load Configuration
3. Modify settings as needed
4. Save and export

## Legacy Scripts

- `generate_fonts.bat` - Old script (still works, uses bmfont_config/)

## Character Sets Reference

| Range | Characters |
|-------|-----------|
| 32-126 | Basic ASCII |
| 160-255 | Extended Latin |
| 8211-8212 | En/Em dash |
| 8216-8221 | Smart quotes |
| 8226 | Bullet • |
| 8230 | Ellipsis … |
| 8364 | Euro € |
| 19968-40959 | CJK Chinese |

## Requirements

- **BMFont** (bmfont64.exe or bmfont32.exe)
- Windows OS
- TTF or OTF font file

## BMFont Download

https://www.angelcode.com/products/bmfont/
