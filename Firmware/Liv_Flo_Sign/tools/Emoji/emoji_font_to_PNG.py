from PIL import Image, ImageDraw, ImageFont
from pathlib import Path
import os

# Define the size of the images to match the font size

# Path to the emoji list and font
base_path = Path(__file__).parent
emoji_list_path = base_path / 'emoji_list.txt'
font_path = '/System/Library/Fonts/Apple Color Emoji.ttc'

font_sizes = range(5, 1000)

# Create export folder
export_folder_path = base_path / 'export'
export_folder_path.mkdir(exist_ok=True)

# Delete all files in the "export" folder
for filename in os.listdir(base_path / "export"):
    os.remove(base_path / "export" / filename)


for font_size in font_sizes: # Font size for the emojis
    try:
        background_color = (0, 0, 0)  # Black background
        image_size = (font_size, font_size)  # Same as font size
        emoji_position = (0, 0)  # Position to start at top-left (centered in 40x40)

        # Load the font
        try:
            font = ImageFont.truetype(font_path, font_size)
        except IOError:
            # print(f"Error: Could not load font from {font_path}, Size: {font_size}")
            continue

        # Read the list of emojis from the file
        with emoji_list_path.open('r', encoding='utf-8') as f:
            emojis = [line.strip() for line in f if line.strip()]

        # Remove double emojis
        emojis = list(set(emojis))

        processed_emojis = []
        # Process each emoji in the list
        for i, emoji in enumerate(emojis):
            # Create a new blank image with an RGB mode and black background
            im = Image.new("RGB", image_size, background_color)
            draw = ImageDraw.Draw(im)

            # Render the emoji at the defined position (0, 0) in the 40x40 box
            draw.text(emoji_position, emoji, font=font, embedded_color=True)  # Render emoji in its native color

            # Convert emoji to its hex code representation for the filename, ignoring ZWJ and variant selectors
            hex_filename = '-'.join(f'{ord(char):x}' for char in emoji if ord(char) not in [0x200D, 0xFE0F])

            # Check if hex_filename conatins - (dash), ignore the emoji since it is not a single emoji
            if '-' in hex_filename:
                print(f"Ignored {emoji} with hex code {hex_filename}")
                continue

            # Save the image as PNG with the hex code representation as filename
            output_file_path = export_folder_path / f'{hex_filename}.png'
            im.save(output_file_path)
            processed_emojis.append(emoji)  # Add the emoji to the processed list

            # print(f"Saved {output_file_path}")
        print(f"Generated {len(emojis)} emoji images with size {font_size}.")

        # Write back the list of emojis to the file
        with emoji_list_path.open('w', encoding='utf-8') as f:
            f.write('\n'.join(processed_emojis))

    except Exception as e:
        print(f"Error: {e}")
        continue

print(f"Generated {len(emojis)} emoji images.")
