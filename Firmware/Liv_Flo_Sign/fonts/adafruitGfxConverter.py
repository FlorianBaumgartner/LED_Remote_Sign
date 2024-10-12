import freetype
import sys
import os
from pathlib import Path

DPI = 141  # Approximate resolution of Adafruit 2.8" TFT

# Global variables
FONT_DIR = Path(__file__).parent / "Grand9K_Pixel.ttf"  # Path to the font file
FONT_SIZE = 8  # Font size to use

# Output file path
output_file = Path(__file__).parent / f"{FONT_DIR.stem}{FONT_SIZE}.h"

def enbit(value, output):
    """Accumulate bits for output, with periodic hexadecimal byte write"""
    global row, sum_bits, bit, first_call
    if value:
        sum_bits |= bit
    bit >>= 1
    if bit == 0:
        if not first_call:
            if row >= 12:
                output.write(",\n  ")
                row = 0
            else:
                output.write(", ")
            row += 1
        output.write(f"0x{sum_bits:02X}")
        sum_bits = 0
        bit = 0x80
        first_call = False

def main():
    # Use global variables
    fontfile = FONT_DIR
    size = FONT_SIZE
    first = ord(' ')
    # last = ord('~')
    last = 0xFF

    font_name = fontfile.stem
    font_name = f"{font_name}{size}pt7b"

    global row, sum_bits, bit, first_call
    row = 0
    sum_bits = 0
    bit = 0x80
    first_call = True

    # Initialize FreeType and load the font
    face = freetype.Face(str(fontfile))
    face.set_char_size(size * 32, 0, DPI, 0)

    glyph_table = []

    # Open output file
    with output_file.open("w") as output:
        output.write(f"const uint8_t {font_name}Bitmaps[] PROGMEM = {{\n  ")

        bitmap_offset = 0
        for charcode in range(first, last + 1):
            face.load_char(charcode, freetype.FT_LOAD_RENDER | freetype.FT_LOAD_TARGET_MONO)
            bitmap = face.glyph.bitmap
            glyph = face.glyph

            glyph_info = {
                'bitmap_offset': bitmap_offset,
                'width': bitmap.width,
                'height': bitmap.rows,
                'x_advance': glyph.advance.x // 64,
                'x_offset': glyph.bitmap_left,
                'y_offset': 1 - glyph.bitmap_top,
            }

            for y in range(bitmap.rows):
                for x in range(bitmap.width):
                    byte = x // 8
                    bitmask = 0x80 >> (x & 7)
                    enbit(bitmap.buffer[y * bitmap.pitch + byte] & bitmask, output)

            # Pad to next byte boundary
            n = (bitmap.width * bitmap.rows) % 8
            if n:
                n = 8 - n
                while n:
                    enbit(0, output)
                    n -= 1

            bitmap_offset += (bitmap.width * bitmap.rows + 7) // 8
            glyph_table.append(glyph_info)

        output.write(" };\n\n")

        output.write(f"const GFXglyph {font_name}Glyphs[] PROGMEM = {{\n")
        for i, glyph in enumerate(glyph_table):
            output.write(
                f"  {{ {glyph['bitmap_offset']:5d}, {glyph['width']:3d}, {glyph['height']:3d}, "
                f"{glyph['x_advance']:3d}, {glyph['x_offset']:4d}, {glyph['y_offset']:4d} }}"
            )
            if i < len(glyph_table) - 1:
                output.write(f",   // 0x{first + i:02X}")
                if ' ' <= chr(first + i) <= '~':
                    output.write(f" '{chr(first + i)}'")
                output.write("\n")
        output.write(" };\n\n")

        output.write(f"const GFXfont {font_name} PROGMEM = {{\n")
        output.write(f"  (uint8_t  *){font_name}Bitmaps,\n")
        output.write(f"  (GFXglyph *){font_name}Glyphs,\n")
        output.write(f"  0x{first:02X}, 0x{last:02X}, {face.size.height // 64} }};\n\n")

        output.write(f"// Approx. {bitmap_offset + len(glyph_table) * 7 + 7} bytes\n")

    print(f"Font conversion complete. Output saved to {output_file}")

if __name__ == "__main__":
    main()
