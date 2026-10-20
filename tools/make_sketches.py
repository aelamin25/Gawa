"""
make_sketches.py - turn drawings into sketches.h for Gawa.

How to use (from the main Gawa folder):
    python3 tools/make_sketches.py

It looks in the "artwork/sketches" folder for these files and makes firmware/Gawa/sketches.h:
    one PNG per name in NAMES below (happy.png, confused.png, ... appreciative.png), 99 faces in all
Each picture can be any size; it is shrunk to fit 200 x 150 and turned black-and-white with
dithering (so pencil shading keeps its texture). Light paper (anything brighter than 215/255) becomes white.

To change a face, replace its PNG and run this again, then upload the sketch from Arduino IDE.
Needs: pip install pillow
"""
from PIL import Image, ImageOps, ImageEnhance
import os, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
NAMES = [  # the first 24 are from the first two sheets, the rest from the big 112-face sheet
    "happy", "confused", "excited", "angry", "sad", "surprised", "sleepy", "silly",
    "scared", "grumpy", "grateful", "proud", "curious", "confident", "peaceful", "mischievous",
    "thoughtful", "thoughtful2", "relief", "shocked", "disgusted", "nervous", "tired", "shy",
    "elated", "cheerful", "delighted", "serene", "optimistic", "humble", "empowered", "triumphant",
    "melancholic", "dejected", "hopeless", "sorrowful", "frustrated", "resentful", "bitter", "hostile",
    "detailed", "ashamed", "jealous", "envious", "protective", "distrustful", "rivalrous", "empathetic",
    "sympathetic", "wired", "energetic", "compassionate", "over_positive", "hyper", "fatigued", "stressed",
    "hungry", "thirsty", "nauseated", "repulsed", "bored", "indifferent", "listless", "inquisitive",
    "focused", "concentrating", "dazed", "mystified", "insecure", "self_doubting", "decisive", "indecisive",
    "resigned", "nostalgic", "bittersweet", "contented", "affectionate", "loved", "passionate", "admiring",
    "appreciative", "guilt_ridden", "hopeful", "terrified", "alarmed", "apprehensive", "uneasy", "burned_out",
    "devastated", "dread_filled", "perplexed", "bewildered", "lost", "astounded", "stunned", "speechless",
    "awkward", "content", "embarrassed",
]
W, H = 200, 150

def to_bits(path):
    im = Image.open(path).convert("L")
    im = ImageOps.autocontrast(im, cutoff=1)
    im = im.point(lambda v: 255 if v > 215 else v)
    im = ImageEnhance.Contrast(im).enhance(1.3)
    im.thumbnail((W, H), Image.LANCZOS)
    canvas = Image.new("L", (W, H), 255)
    canvas.paste(im, ((W - im.width) // 2, (H - im.height) // 2))
    bw = canvas.convert("1")
    px = bw.load()
    out = bytearray()
    for y in range(H):
        row = bytearray(W // 8)
        for x in range(W):
            if px[x, y] == 0:
                row[x >> 3] |= 0x80 >> (x & 7)
        out += row
    return bytes(out)

def main():
    arrays = []
    for name in NAMES:
        path = os.path.join(ROOT, "artwork", "sketches", name + ".png")
        if not os.path.exists(path):
            sys.exit(f"missing {path}")
        arrays.append((name, to_bits(path)))
    with open(os.path.join(ROOT, "firmware", "Gawa", "sketches.h"), "w") as f:
        f.write("// sketches.h - pencil-sketch penguin faces, 200x150 pixels, 1 bit per pixel (1 = black), 25 bytes per row.\n")
        f.write("// Made by tools/make_sketches.py from the PNGs in the sketches folder. Do not edit by hand.\n#pragma once\n#include <stdint.h>\n\n")
        f.write("#define SKETCH_W 200\n#define SKETCH_H 150\n\n")
        for name, data in arrays:
            f.write(f"static const uint8_t SKETCH_{name.upper()}[{len(data)}] = {{\n")
            for i in range(0, len(data), 25):
                f.write("  " + ",".join(f"0x{b:02X}" for b in data[i:i + 25]) + ",\n")
            f.write("};\n\n")
        f.write("enum { " + ", ".join(("SK_" + n.upper() + (" = 0" if i == 0 else "")) for i, n in enumerate(NAMES)) + ", SK_COUNT };\n")
        f.write("static const uint8_t *SKETCHES[SK_COUNT] = { " + ", ".join("SKETCH_" + n.upper() for n in NAMES) + " };\n")
    print("wrote firmware/Gawa/sketches.h with", len(arrays), "faces")

if __name__ == "__main__":
    main()
