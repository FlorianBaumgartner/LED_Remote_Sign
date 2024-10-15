from PIL import Image, ImageDraw, ImageFont
from pathlib import Path

# Define the size of the images to match the font size

# Path to the emoji list and font
base_path = Path(__file__).parent
emoji_list_path = base_path / 'emoji_list.txt'
font_path = '/System/Library/Fonts/Apple Color Emoji.ttc'

font_sizes = range(5, 1000)

# Create export folder
export_folder_path = base_path / 'export'
export_folder_path.mkdir(exist_ok=True)

# Delete all files in the export folder
for filename in export_folder_path.iterdir():
    filename.unlink()

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

        # Process each emoji in the list
        for i, emoji in enumerate(emojis):
            # Create a new blank image with an RGB mode and black background
            im = Image.new("RGB", image_size, background_color)
            draw = ImageDraw.Draw(im)

            # Render the emoji at the defined position (0, 0) in the 40x40 box
            draw.text(emoji_position, emoji, font=font, embedded_color=True)  # Render emoji in its native color

            # Convert emoji to its hex code representation for the filename, ignoring ZWJ and variant selectors
            hex_filename = '-'.join(f'{ord(char):x}' for char in emoji if ord(char) not in [0x200D, 0xFE0F])

            # Save the image as PNG with the hex code representation as filename
            output_file_path = export_folder_path / f'{i:03d}_{hex_filename}.png'
            im.save(output_file_path)

            # print(f"Saved {output_file_path}")
        print(f"Generated {len(emojis)} emoji images with size {font_size}.")

    except Exception as e:
        print(f"Error: {e}")
        continue

print(f"Generated {len(emojis)} emoji images.")
