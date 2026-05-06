"""
GFT to FNT Converter
Converts proprietary .gft font files to BMFont .fnt text format

Usage: python gft_to_fnt.py input.gft output.fnt
"""

import struct
import sys
import os

def read_gft(filepath):
    """Parse a .gft file and extract character data"""
    with open(filepath, 'rb') as f:
        data = f.read()
    
    # Header analysis (based on reverse engineering)
    # Offset 0-3: seems to be 0
    # Offset 4-7: float - possibly line height (60.0)
    # Offset 8-11: int - seems to be texture width (512)
    # Offset 12-15: int - seems to be texture height (512)
    
    header_size = 40  # Approximate header size
    entry_size = 44   # Each character entry appears to be 44 bytes
    
    # Try to detect structure
    offset = 0
    
    # Read potential header
    line_height = struct.unpack_from('<f', data, 4)[0]
    tex_width = struct.unpack_from('<i', data, 8)[0]
    tex_height = struct.unpack_from('<i', data, 12)[0]
    
    # If values don't make sense, try alternate interpretation
    if tex_width > 4096 or tex_width <= 0:
        tex_width = 512
    if tex_height > 4096 or tex_height <= 0:
        tex_height = 512
    if line_height <= 0 or line_height > 200:
        line_height = 60
    
    print(f"Detected: line_height={line_height}, tex={tex_width}x{tex_height}")
    
    chars = []
    
    # Scan for character entries
    # Pattern: small int (char id), then floats for coords
    offset = header_size
    
    while offset + entry_size <= len(data):
        try:
            # Try reading as: charId (4 bytes), then floats
            char_id = struct.unpack_from('<I', data, offset)[0]
            
            # Skip if char_id looks invalid (too high for ASCII/Unicode BMP)
            if char_id > 65535:
                offset += 4
                continue
            
            # Read UV coordinates and metrics
            x = struct.unpack_from('<f', data, offset + 4)[0]
            y = struct.unpack_from('<f', data, offset + 8)[0]
            x2 = struct.unpack_from('<f', data, offset + 12)[0]
            y2 = struct.unpack_from('<f', data, offset + 16)[0]
            xoffset = struct.unpack_from('<f', data, offset + 20)[0]
            yoffset = struct.unpack_from('<f', data, offset + 24)[0]
            xadvance = struct.unpack_from('<f', data, offset + 28)[0]
            
            # Validate - coordinates should be reasonable
            if 0 <= x <= tex_width and 0 <= y <= tex_height:
                width = abs(x2 - x) if x2 > x else abs(x - x2)
                height = abs(y2 - y) if y2 > y else abs(y - y2)
                
                if width > 0 and height > 0 and width < 200 and height < 200:
                    chars.append({
                        'id': char_id,
                        'x': int(min(x, x2)),
                        'y': int(min(y, y2)),
                        'width': int(width),
                        'height': int(height),
                        'xoffset': int(xoffset) if abs(xoffset) < 100 else 0,
                        'yoffset': int(yoffset) if abs(yoffset) < 100 else 0,
                        'xadvance': int(xadvance) if 0 < xadvance < 200 else int(width),
                        'page': 0
                    })
            
            offset += entry_size
            
        except struct.error:
            break
    
    return {
        'line_height': int(line_height),
        'base': int(line_height * 0.8),
        'tex_width': tex_width,
        'tex_height': tex_height,
        'chars': chars
    }

def write_fnt(font_data, texture_name, output_path):
    """Write BMFont text format .fnt file"""
    with open(output_path, 'w') as f:
        # Info line
        f.write(f'info face="Converted" size={font_data["line_height"]} bold=0 italic=0 ')
        f.write('charset="" unicode=1 stretchH=100 smooth=1 aa=1 padding=0,0,0,0 spacing=1,1 outline=0\n')
        
        # Common line
        f.write(f'common lineHeight={font_data["line_height"]} base={font_data["base"]} ')
        f.write(f'scaleW={font_data["tex_width"]} scaleH={font_data["tex_height"]} pages=1 packed=0 ')
        f.write('alphaChnl=0 redChnl=4 greenChnl=4 blueChnl=4\n')
        
        # Page line
        f.write(f'page id=0 file="{texture_name}"\n')
        
        # Chars count
        f.write(f'chars count={len(font_data["chars"])}\n')
        
        # Character entries
        for char in font_data['chars']:
            f.write(f'char id={char["id"]:4d} ')
            f.write(f'x={char["x"]:4d} y={char["y"]:4d} ')
            f.write(f'width={char["width"]:4d} height={char["height"]:4d} ')
            f.write(f'xoffset={char["xoffset"]:4d} yoffset={char["yoffset"]:4d} ')
            f.write(f'xadvance={char["xadvance"]:4d} page={char["page"]} chnl=15\n')

def convert_gft_to_fnt(gft_path, fnt_path=None):
    """Main conversion function"""
    if fnt_path is None:
        fnt_path = gft_path.replace('.gft', '.fnt')
    
    # Determine texture filename (same name but .tga or .png)
    base_name = os.path.splitext(os.path.basename(gft_path))[0]
    
    # Check for existing texture
    gft_dir = os.path.dirname(gft_path)
    if os.path.exists(os.path.join(gft_dir, f'{base_name}.tga')):
        texture_name = f'{base_name}_0.tga'
    elif os.path.exists(os.path.join(gft_dir, f'{base_name}.png')):
        texture_name = f'{base_name}_0.png'
    else:
        texture_name = f'{base_name}_0.tga'  # Default to TGA
    
    print(f"Converting: {gft_path}")
    print(f"Output: {fnt_path}")
    print(f"Texture: {texture_name}")
    
    font_data = read_gft(gft_path)
    
    print(f"Found {len(font_data['chars'])} characters")
    
    if len(font_data['chars']) == 0:
        print("WARNING: No characters found! The GFT format may be different.")
        return False
    
    write_fnt(font_data, texture_name, fnt_path)
    print("Conversion complete!")
    return True

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: python gft_to_fnt.py input.gft [output.fnt]")
        sys.exit(1)
    
    gft_path = sys.argv[1]
    fnt_path = sys.argv[2] if len(sys.argv) > 2 else None
    
    if not os.path.exists(gft_path):
        print(f"Error: File not found: {gft_path}")
        sys.exit(1)
    
    success = convert_gft_to_fnt(gft_path, fnt_path)
    sys.exit(0 if success else 1)
