#!/usr/bin/env python3
"""make_font.py - builds font.bin, the Chinese + pinyin font Pengu draws with (flashed at 0x620000).

    python3 tools/make_font.py build/font.bin            # uses Noto Sans CJK SC from the system
    python3 tools/make_font.py build/font.bin --font /path/NotoSansCJK-Medium.ttc

Four faces in one file:
  huge  (36 px): the same characters as big, for short lines (5 characters or fewer), so they read from across a desk
  big   (24 px): the 3,755 most common characters (GB2312 level 1), Chinese punctuation, ASCII
  small (20 px): ASCII and every tone-marked pinyin vowel (pinyin and English that need two lines)
  latin (26 px): the same as small, bigger, for pinyin and English that fit on one line

File layout (all little endian):
  "PFN2"  u32 face_count  u32 offset_of_face[face_count]     (order: big, small, huge, latin)
  face:   "PFNT"  u32 count  u8 height  u8 0  u16 0
          count x {u32 codepoint, u16 advance, u16 bitmap_width, u32 bitmap_offset}   (sorted by codepoint)
          bitmaps: height rows of ceil(bitmap_width/8) bytes, MSB = leftmost pixel, 1 = black
The bitmap offset counts from the first byte after the entry table.
"""
import argparse, struct, sys
from PIL import Image, ImageDraw, ImageFont

DEFAULT_FONT = "/usr/share/fonts/opentype/noto/NotoSansCJK-Medium.ttc"

def gb2312_level1():
    out = []
    for hi in range(0xB0, 0xD8):
        for lo in range(0xA1, 0xFF):
            if hi == 0xD7 and lo > 0xF9:
                break
            out.append(bytes([hi, lo]).decode("gb2312"))
    return out

CN_PUNCT = "。，、；：？！“”‘’（）《》【】…—·～「」『』０１２３４５６７８９"
# words the course needs that GB2312 level 1 might not have (added if missing)
COURSE_EXTRA = "饺渴澡刷睡觉冰箱奶苹果茶包散步店馆菜块累晚安洗牙玩游戏电视音乐跳舞书休息无聊点儿鹅企"
ASCII = [chr(c) for c in range(32, 127)]
PINYIN = "āáǎàēéěèīíǐìōóǒòūúǔùǖǘǚǜüĀÁǍÀĒÉĚÈĪÍǏÌŌÓǑÒŪÚǓÙǕǗǙǛÜ’‘“”…–—·ńňǹḿ"

def load_sc(path, size):
    for i in range(12):
        try:
            f = ImageFont.truetype(path, size, index=i)
        except OSError:
            break
        if "SC" in " ".join(f.getname()):
            return f
    return ImageFont.truetype(path, size)

def render(font, ch, height, top, adv):
    """Draws one character into a height x adv cell. top = where the font's em box starts (pixels, may be negative)."""
    img = Image.new("L", (adv + 4, height), 255)
    d = ImageDraw.Draw(img)
    x = 0
    if adv >= height:                                   # a full-width character: centre it in the cell
        w = font.getlength(ch)
        x = int(round((adv - w) / 2))
    d.text((x, top), ch, font=font, fill=0)
    img = img.crop((0, 0, adv, height))
    bw = adv
    rowbytes = (bw + 7) // 8
    data = bytearray()
    px = img.load()
    for y in range(height):
        row = bytearray(rowbytes)
        for xx in range(bw):
            if px[xx, y] < 128:
                row[xx >> 3] |= 0x80 >> (xx & 7)
        data += row
    return bw, bytes(data)

def build_face(font, chars, height, top, fixed_cjk_adv=None):
    chars = sorted(set(chars), key=ord)
    entries, bitmaps = [], bytearray()
    for ch in chars:
        cp = ord(ch)
        if fixed_cjk_adv and cp >= 0x2E80:
            adv = fixed_cjk_adv
        else:
            adv = max(1, int(round(font.getlength(ch))))
        bw, bits = render(font, ch, height, top, adv)
        entries.append((cp, adv, bw, len(bitmaps)))
        bitmaps += bits
    out = bytearray(b"PFNT" + struct.pack("<IBBH", len(entries), height, 0, 0))
    for cp, adv, bw, off in entries:
        out += struct.pack("<IHHI", cp, adv, bw, off)
    out += bitmaps
    return bytes(out), len(entries)

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("out")
    ap.add_argument("--font", default=DEFAULT_FONT)
    a = ap.parse_args()

    big_font = load_sc(a.font, 23)
    small_font = load_sc(a.font, 15)
    hanzi = gb2312_level1()
    missing = [c for c in COURSE_EXTRA if c not in hanzi]
    if missing:
        print("adding course characters not in GB2312 level 1:", "".join(missing))
    big_chars = hanzi + missing + list(CN_PUNCT) + ASCII
    small_chars = ASCII + list(PINYIN)

    big, nb = build_face(big_font, big_chars, 24, -7, fixed_cjk_adv=24)
    small, ns = build_face(small_font, small_chars, 20, -4)
    huge, nh = build_face(load_sc(a.font, 35), big_chars, 36, -10, fixed_cjk_adv=36)

    latin, nl = build_face(load_sc(a.font, 21), small_chars, 26, -5)

    faces = [big, small, huge, latin]
    head = b"PFN2" + struct.pack("<I", len(faces))
    offs, pos = [], len(head) + 4 * len(faces)
    for f in faces:
        offs.append(pos); pos += len(f)
    blob = head + struct.pack("<" + "I" * len(faces), *offs) + b"".join(faces)
    with open(a.out, "wb") as f:
        f.write(blob)
    print(f"font.bin: {nb} big (24 px), {ns} small (20 px), {nh} huge (36 px), {nl} latin (26 px) glyphs, {len(blob)/1024:.0f} KB")
    if len(blob) > 0x1E0000:
        sys.exit("font.bin is bigger than the 0x1E0000 partition")

if __name__ == "__main__":
    main()
