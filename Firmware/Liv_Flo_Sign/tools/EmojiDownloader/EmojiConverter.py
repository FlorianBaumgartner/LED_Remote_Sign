import os
from PIL import Image
from pathlib import Path

def rgb_to_uint8_hex(rgb):
    """Converts RGB tuple to a string representation of an array with uint8_t values in hex format."""
    return f'{{0x{rgb[0]:02X}, 0x{rgb[1]:02X}, 0x{rgb[2]:02X}}}'

if __name__ == "__main__":
    root = Path(__file__).parent
    try:
        os.mkdir(root / "conv")
    except FileExistsError:
        pass

    size = 7
    header_file_path = root / "emoji_bitmaps.h"

    with open(header_file_path, 'w', encoding="utf-8") as header_file:
        header_file.write("#ifndef EMOJI_BITMAPS_H\n")
        header_file.write("#define EMOJI_BITMAPS_H\n\n")

        header_file.write("#include <Arduino.h>\n\n")

        header_file.write("// Emoji bitmaps in RGB format, size: 7x7 pixels\n\n")

        for filename in os.listdir(root / "export"):
            if filename.endswith(".png"):
                image = Image.open(root / "export" / filename)
                new_image = image.resize((size, size))
                new_image = new_image.convert('RGB')  # Ensure it's in RGB mode

                # Save the resized image in the "conv" folder
                new_image.save(root / "conv" / filename)

                # Get UTF-8 hex representation for filename and comments
                emoji_name = filename.split('.')[0]
                utf8_codepoints = '-'.join(f'{ord(char):x}' for char in emoji_name)
                utf8_comment = ' '.join(f'U+{ord(char):04X}' for char in emoji_name)

                array_field_name = f"emoji_{emoji_name}"
                # Replace all dashed with underscores
                array_field_name = array_field_name.replace('-', '_')

                code_points = emoji_name.split('-')
                # Convert each code point from hex to an integer, then to the corresponding character
                emoji_characters = ''.join(chr(int(cp, 16)) for cp in code_points)

                header_file.write(f"// Emoji: {emoji_characters} (UTF-8: {utf8_comment})\n")
                header_file.write(f"const uint8_t {array_field_name}[7][7][3] = {{\n")
                
                for y in range(size):
                    row = []
                    for x in range(size):
                        rgb = new_image.getpixel((x, y))
                        row.append(rgb_to_uint8_hex(rgb))
                    header_file.write(f"  {{{', '.join(row)}}},\n")
                
                header_file.write("};\n\n")

        header_file.write("#endif // EMOJI_BITMAPS_H\n")

    print(f"Header file generated: {header_file_path}")
