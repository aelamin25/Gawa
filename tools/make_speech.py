"""
make_speech.py - record every line Pengu can show, so it can SAY exactly what is on the screen.

How to use (from the main folder):
    g++ -std=c++17 -w -I firmware/Pengu tools/list_lines.cpp -o /tmp/list_lines && /tmp/list_lines > tools/speech_lines.txt
    python3 tools/make_speech.py

For each line in tools/speech_lines.txt it makes a small spoken clip (piper, the soft "amy" voice, pitched up a little),
cleans it for the tiny speaker (nothing below 250 Hz, compressed, loud), and packs all of them as 8 kHz IMA ADPCM
(4 bits per sample, about 4 KB per second) into one file:
    releases/speech.bin          the sound data, flashed to the "speech" partition (see partitions.csv)
    firmware/Pengu/speech_index.h  which line starts where, so Pengu can find the clip for the text it is showing
Clips are cached in tools/speech_cache/ (keyed by the text), so only new or changed lines are recorded again.
Needs: pip install piper-tts, ffmpeg, and the voice next to this script:  python3 -m piper.download_voices en_US-amy-medium
"""
import os, sys, subprocess, struct, hashlib

HERE = os.path.dirname(os.path.abspath(__file__)); ROOT = os.path.dirname(HERE)
VOICE = os.path.join(HERE, "en_US-amy-medium.onnx")
LINES = os.path.join(HERE, "speech_lines.txt")
CACHE = os.path.join(HERE, "speech_cache")
OUT_BIN = os.path.join(ROOT, "releases", "speech.bin")
OUT_H = os.path.join(ROOT, "firmware", "Pengu", "speech_index.h")
PITCH, RATE = 1.25, 8000
MAX_BYTES = 0x1D0000          # the size of the "speech" partition in partitions.csv

STEP = [7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,66,73,80,88,97,107,118,130,143,157,173,190,209,230,253,279,307,337,371,408,449,494,544,598,658,724,796,876,963,1060,1166,1282,1411,1552,1707,1878,2066,2272,2499,2749,3024,3327,3660,4026,4428,4871,5358,5894,6484,7132,7845,8630,9493,10442,11487,12635,13899,15289,16818,18500,20350,22385,24623,27086,29794,32767]
INDEX = [-1,-1,-1,-1,2,4,6,8]

def adpcm_encode(samples):
    """plain IMA ADPCM, no block headers: the decoder in speech.h starts from predictor 0, index 0"""
    pred, idx, out, nib = 0, 0, bytearray(), []
    for s in samples:
        step = STEP[idx]; diff = s - pred; code = 0
        if diff < 0: code = 8; diff = -diff
        d = step >> 3
        if diff >= step: code |= 4; diff -= step; d += step
        if diff >= step >> 1: code |= 2; diff -= step >> 1; d += step >> 1
        if diff >= step >> 2: code |= 1; d += step >> 2
        pred += -d if code & 8 else d
        pred = max(-32768, min(32767, pred))
        idx = max(0, min(88, idx + INDEX[code & 7]))
        nib.append(code)
        if len(nib) == 2: out.append(nib[0] | (nib[1] << 4)); nib = []
    if nib: out.append(nib[0])
    return bytes(out)

def spoken(text):
    t = text.replace("...", ".").lower().replace("pengu", "pen-goo")
    return t

def record(text):
    key = hashlib.md5((text + str(PITCH)).encode()).hexdigest()[:12]
    cached = os.path.join(CACHE, key + ".raw")
    if os.path.exists(cached): return open(cached, "rb").read()
    raw = os.path.join(CACHE, key + "_tts.wav")
    subprocess.run(["piper", "-m", VOICE, "-f", raw, "--length-scale", "1.0"], input=spoken(text).encode(), check=True, capture_output=True)
    af = (f"rubberband=pitch={PITCH}:tempo=1.0,"
          "silenceremove=start_periods=1:start_threshold=-40dB,areverse,silenceremove=start_periods=1:start_threshold=-40dB,areverse,"
          "highpass=f=250,acompressor=threshold=-20dB:ratio=4:attack=5:release=80:makeup=10dB,alimiter=limit=0.85:level=false,"
          "afade=t=in:d=0.015,apad=pad_dur=0.03")
    pcm = subprocess.run(["ffmpeg", "-v", "error", "-y", "-i", raw, "-af", af, "-ac", "1", "-ar", str(RATE), "-f", "s16le", "-"], check=True, capture_output=True).stdout
    os.remove(raw); open(cached, "wb").write(pcm)
    return pcm

def main():
    os.makedirs(CACHE, exist_ok=True)
    lines = []
    for l in open(LINES):
        l = l.rstrip("\n")
        if l and l not in lines: lines.append(l)
    blob = bytearray(); index = []
    for i, text in enumerate(lines):
        pcm = record(text)
        samples = struct.unpack(f"<{len(pcm) // 2}h", pcm)
        data = adpcm_encode(samples)
        index.append((text, len(blob), len(data))); blob += data
        if i % 25 == 0: print(f"{i}/{len(lines)} {len(blob) // 1024} KB")
    if len(blob) > MAX_BYTES: sys.exit(f"speech.bin is {len(blob)} bytes, more than the partition ({MAX_BYTES}). Shorten some lines.")
    open(OUT_BIN, "wb").write(blob)
    with open(OUT_H, "w") as f:
        f.write("// speech_index.h - where each spoken line lives in speech.bin. Made by tools/make_speech.py. Do not edit by hand.\n#pragma once\n#include <stdint.h>\n\n")
        f.write("struct SpeechLine { const char *text; uint32_t offset; uint32_t length; };\nstatic const SpeechLine SPEECH[] = {\n")
        for text, off, n in index:
            f.write('  { "%s", %d, %d },\n' % (text.replace('"', '\\"'), off, n))
        f.write("};\n#define SPEECH_COUNT (sizeof(SPEECH) / sizeof(SPEECH[0]))\n")
    print(f"wrote {OUT_BIN} ({len(blob) // 1024} KB, {len(lines)} lines, {len(blob) / 4000:.0f} s of speech) and {OUT_H}")

if __name__ == "__main__":
    main()
