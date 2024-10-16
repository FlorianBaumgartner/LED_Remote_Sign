import os
from PIL import Image
from pathlib import Path

def rgb_to_uint8_hex(rgb):
    """Converts RGB tuple to a string representation of an array with uint8_t values in hex format."""
    return f'{{0x{rgb[0]:02X}, 0x{rgb[1]:02X}, 0x{rgb[2]:02X}}}'

def unicode_to_utf8_dec(code_point_hex):
    """Converts a string of code points separated by dashes to UTF-8 decimal values."""
    utf8_dec_values = []
    code_points = code_point_hex.split('-')  # Handle multiple code points
    for cp in code_points:
        code_point = int(cp, 16)
        character = chr(code_point)
        utf8_bytes = character.encode('utf-8')
        utf8_dec_values.extend(utf8_bytes)  # Collect all UTF-8 bytes
    return utf8_dec_values


if __name__ == "__main__":
    root = Path(__file__).parent
    try:
        os.mkdir(root / "conv")
    except FileExistsError:
        pass

    # Delete all files in the "conv" folder
    for filename in os.listdir(root / "conv"):
        os.remove(root / "conv" / filename)
    

    size = 7
    gamma = 2.75
    header_file_path = root / "emoji_bitmaps.h"

    with open(header_file_path, 'w', encoding="utf-8") as header_file:
        header_file.write("#ifndef EMOJI_BITMAPS_H\n")
        header_file.write("#define EMOJI_BITMAPS_H\n\n")
        header_file.write("#include <Arduino.h>\n\n")
        header_file.write("// Emoji bitmaps in RGB format, size: 7x7 pixels\n\n")

        for filename in sorted(os.listdir(root / "export")):
            if filename.endswith(".png"):
                image = Image.open(root / "export" / filename)
                new_image = image.resize((size, size))
                new_image = new_image.convert('RGB')  # Ensure it's in RGB mode

                # Apply gamma correction
                new_image = new_image.point(lambda p: int(255 * (p / 255) ** gamma))

                # Save the resized image in the "conv" folder
                new_image.save(root / "conv" / filename)

                # Get UTF-8 hex representation for filename and comments
                emoji_name = filename.split('.')[0]
                emoji_name = emoji_name[4:]  # Remove the index and underscore
                utf8_symbols = unicode_to_utf8_dec(emoji_name)

                utf8_comment = f"({', '.join(f'{byte}' for byte in utf8_symbols)})"
                utf8_comment += f" [{', '.join(f'0x{byte:02X}' for byte in utf8_symbols)}]"

                array_field_name = f"emoji_{emoji_name}"
                # Replace all dashed with underscores
                array_field_name = array_field_name.replace('-', '_')

                code_points = emoji_name.split('-')
                # Convert each code point from hex to an integer, then to the corresponding character
                emoji_characters = ''.join(chr(int(cp, 16)) for cp in code_points)

                header_file.write(f"// Emoji: {emoji_characters} UTF-8: {utf8_comment}\n")
                header_file.write(f"const uint8_t {array_field_name}[7][7][3] = {{\n")
                
                for y in range(size):
                    row = []
                    for x in range(size):
                        rgb = new_image.getpixel((x, y))
                        row.append(rgb_to_uint8_hex(rgb))
                    header_file.write(f"  {{{', '.join(row)}}},\n")
                
                header_file.write("};\n\n")

        
        # Add global emoji table, contatining a uint32_t for the unicode index, and a pointer to the emoji bitmap 
        header_file.write("struct Emoji {\n")
        header_file.write("  uint32_t unicode;\n")
        header_file.write("  const uint8_t (*data)[7][3];\n")
        header_file.write("};\n\n")
        header_file.write("const Emoji emojis[] = {\n")
        for filename in sorted(os.listdir(root / "export")):
            if filename.endswith(".png"):
                emoji_name = filename.split('.')[0]
                emoji_name = emoji_name[4:]  # Remove the index and underscore
                code_points = emoji_name.split('-')
                emoji_unicode = int(code_points[0], 16)
                array_field_name = f"emoji_{emoji_name}"
                array_field_name = array_field_name.replace('-', '_')
                header_file.write(f"  {{0x{emoji_unicode:08X}, {array_field_name}}},\n")
        header_file.write("};\n\n")

        # Add COnstant that holds total emoji count
        header_file.write(f"const uint16_t emoji_count = {len(os.listdir(root / 'export'))};\n\n")


        header_file.write("#endif // EMOJI_BITMAPS_H\n")

    print(f"Header file generated: {header_file_path}")
