"""
crop_sheet.py - cut the 24 penguin drawings out of the emotions sheet.

How to use (from the main folder):
    python3 tools/crop_sheet.py

Reads artwork/originals/penguin_emotions_sheet.jpg and the boxes in tools/crop_boxes.txt (one line per face:
name left top right bottom, in pixels on the sheet), writes one PNG per face into artwork/sketches/, doubled in size
so the dithering in make_sketches.py has more to work with. The boxes stop just above each handwritten label and leave
room for the flippers, so nothing is cut off on the screen. silly.png comes from the first sheet and is not touched.
Then run: python3 tools/make_sketches.py
"""
from PIL import Image
import os
HERE = os.path.dirname(os.path.abspath(__file__)); ROOT = os.path.dirname(HERE)
im = Image.open(os.path.join(ROOT, "artwork", "originals", "penguin_emotions_sheet.jpg")).convert("L")
for line in open(os.path.join(HERE, "crop_boxes.txt")):
    if line.startswith("#") or not line.strip(): continue
    name, l, t, r, b = line.split(); box = tuple(int(v) for v in (l, t, r, b))
    c = im.crop(box); c = c.resize((c.width * 2, c.height * 2), Image.LANCZOS)
    c.save(os.path.join(ROOT, "artwork", "sketches", name + ".png")); print("cut", name)
