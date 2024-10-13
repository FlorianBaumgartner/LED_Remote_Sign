import os
from PIL import Image
from pathlib import Path

if __name__ == "__main__":
    root = Path(__file__).parent
    try:
        os.mkdir(root/"conv")
    except FileExistsError:
        pass

    size = 7

    for filename in os.listdir(root / "raw"):
        if filename.endswith(".png"):
            image = Image.open(root / "raw" / filename)
            new_image = image.resize((size, size))
            filename = filename[:5]
            new_image.save(root / "conv" / f"{filename}.png")
            print(f"{filename}")
