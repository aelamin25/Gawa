"""
make_speech.py - record every line Pengu can show, so it can SAY exactly what is on the screen.

How to use (from the main folder):
    g++ -std=c++17 -w -I firmware/Pengu tools/list_lines.cpp -o /tmp/list_lines && /tmp/list_lines > tools/speech_lines.txt
    python3 tools/make_speech.py

For each line in tools/speech_lines.txt it makes a small spoken clip with Kokoro (an offline voice engine) using
"Alpha", one of its Japanese anime-style voices, speaking the English lines (21 Oct, M: "I want it to sound like a
Japanese child anime"; the English voices, plain or pitched, all sounded robotic or wrong). Each line carries a feeling
tag (from list_lines.cpp: happy, sad, excited, sleepy...), and the voice ACTS it: TONES below sets, per feeling, the
pace, how high the voice sits, how loud, and how the pitch moves inside a sentence (questions rise, sad lines fall,
excited ones start bright). It cleans each clip for the tiny speaker (nothing below 200 Hz, gently compressed) and
packs all of them as 8 kHz IMA ADPCM
(4 bits per sample, about 4 KB per second) into one file:
    releases/speech.bin          the sound data, flashed to the "speech" partition (see partitions.csv)
    firmware/Pengu/speech_index.h  which line starts where, so Pengu can find the clip for the text it is showing
Clips are cached in tools/speech_cache/ (keyed by the text), so only new or changed lines are recorded again.
Needs: pip install kokoro-onnx soundfile, ffmpeg (with rubberband), and the two Kokoro files next to this script or in
../../Himalayan Hop/models/kokoro/ (kokoro-v1.0.onnx, voices-v1.0.bin, 350 MB, from github.com/thewh1teagle/kokoro-onnx releases).
"""
import os, sys, subprocess, struct, hashlib, re
import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__)); ROOT = os.path.dirname(HERE)
KOKORO_DIRS = [HERE, os.path.join(ROOT, "..", "Himalayan Hop", "models", "kokoro"), os.path.expanduser("~/.cache/himalayan-hop/kokoro")]
VOICE, SPEED = "jf_alpha", 0.95         # Alpha: young, clear, anime. 0.95 = a touch slower than the voice's own pace
LINES = os.path.join(HERE, "speech_lines.txt")
CACHE = os.path.join(HERE, "speech_cache")
OUT_BIN = os.path.join(ROOT, "releases", "speech.bin")
OUT_H = os.path.join(ROOT, "firmware", "Pengu", "speech_index.h")
PITCH, RATE = 1.0, 8000               # the voice as it is; raise a little (1.05) for smaller, lower for older
SR = 24000                             # Kokoro's own rate
# How each feeling is acted. speed: Kokoro's pace (1 = normal). pitch: semitones up (+) or down (-) for the whole line.
# gain: dB. rise: semitones added over the last third of a question (0 = none). fall: semitones dropped over the last
# third of a statement. bright: semitones extra on an exclamation. pause: silence between sentences, seconds.
TONES = {
  "excited":    dict(speed=1.12, pitch=+1.0, gain=+1.5, rise=1.2, fall=0.0, bright=0.8, pause=0.06),
  "happy":      dict(speed=1.06, pitch=+0.6, gain=+0.5, rise=1.2, fall=0.0, bright=0.6, pause=0.07),
  "proud":      dict(speed=1.04, pitch=+0.3, gain=+1.0, rise=1.0, fall=0.0, bright=0.6, pause=0.07),
  "playful":    dict(speed=1.08, pitch=+0.8, gain=+0.5, rise=1.5, fall=0.0, bright=0.6, pause=0.06),
  "confident":  dict(speed=1.03, pitch=0.0,  gain=+1.0, rise=0.8, fall=0.3, bright=0.6, pause=0.07),
  "curious":    dict(speed=1.00, pitch=+0.3, gain=0.0,  rise=1.8, fall=0.0, bright=0.3, pause=0.07),
  "loving":     dict(speed=0.96, pitch=+0.3, gain=0.0,  rise=1.0, fall=0.3, bright=0.3, pause=0.10),
  "shy":        dict(speed=0.96, pitch=+0.6, gain=-1.0, rise=1.2, fall=0.3, bright=0.3, pause=0.10),
  "relieved":   dict(speed=0.96, pitch=0.0,  gain=0.0,  rise=0.8, fall=0.6, bright=0.3, pause=0.11),
  "calm":       dict(speed=0.94, pitch=0.0,  gain=-0.5, rise=0.8, fall=0.5, bright=0.0, pause=0.11),
  "thoughtful": dict(speed=0.95, pitch=-0.3, gain=-0.5, rise=1.0, fall=0.3, bright=0.0, pause=0.12),
  "sad":        dict(speed=0.91, pitch=-0.8, gain=-1.5, rise=0.8, fall=0.9, bright=0.0, pause=0.13),
  "sleepy":     dict(speed=0.89, pitch=-0.6, gain=-2.0, rise=0.4, fall=0.9, bright=0.0, pause=0.14),
  "nervous":    dict(speed=1.04, pitch=+0.6, gain=-0.5, rise=1.5, fall=0.0, bright=0.3, pause=0.07),
  "scared":     dict(speed=1.02, pitch=+0.8, gain=-0.5, rise=1.2, fall=0.0, bright=0.3, pause=0.08),
  "grumpy":     dict(speed=0.97, pitch=-0.6, gain=0.0,  rise=0.8, fall=0.6, bright=0.3, pause=0.10),
  "angry":      dict(speed=1.02, pitch=-0.3, gain=+1.0, rise=1.0, fall=0.3, bright=0.6, pause=0.07),
}
MAX_BYTES = 0x1E0000          # the size of the "speech" partition in partitions.csv
SQUEEZE = 1.04                # 1.0 = as recorded; 1.04 = four percent quicker, which is what fits all 292 lines in the partition

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
    """the words as the voice should read them: sentence case (capitals read as shouting), the name said right"""
    t = text.lower().replace("...", ".")
    t = t.replace("pengu", "Pengu").replace("mm hmm", "mm-hmm").replace("grr", "grrr")
    t = ". ".join(p[:1].upper() + p[1:] for p in t.split(". "))
    return t[:1].upper() + t[1:]

_kokoro = None
def kokoro():
    global _kokoro
    if _kokoro is None:
        from kokoro_onnx import Kokoro
        for d in KOKORO_DIRS:
            if os.path.exists(os.path.join(d, "kokoro-v1.0.onnx")):
                _kokoro = Kokoro(os.path.join(d, "kokoro-v1.0.onnx"), os.path.join(d, "voices-v1.0.bin")); break
        if _kokoro is None: sys.exit("Kokoro model files not found; see the note at the top of this file")
    return _kokoro

def sentences(text):
    parts = re.findall(r"[^.!?]+[.!?]*", text)
    return [p.strip() for p in parts if p.strip()]

def shift(x, semis):
    """pitch up or down by semitones, keeping the voice's own colour (rubberband, formant preserved): the phase-vocoder
    way (librosa) left a metallic edge, which is what sounded robotic"""
    if abs(semis) < 0.05 or len(x) < 2400: return x
    ratio = 2 ** (semis / 12)
    raw = np.clip(x, -1, 1).astype(np.float32).tobytes()
    out = subprocess.run(["ffmpeg", "-v", "error", "-f", "f32le", "-ar", str(SR), "-ac", "1", "-i", "-",
                          "-af", f"rubberband=pitch={ratio}:tempo=1.0:formant=preserved:pitchq=quality",
                          "-f", "f32le", "-ar", str(SR), "-ac", "1", "-"], input=raw, check=True, capture_output=True).stdout
    return np.frombuffer(out, dtype=np.float32).copy()

def glide(x, semis):
    """move the last third of a sentence up (+) or down (-) by semis, with a short crossfade so there is no click"""
    if abs(semis) < 0.05 or len(x) < 4800: return x
    cut = int(len(x) * 0.65); fade = 960
    head, tail = x[:cut + fade], shift(x[cut - fade:], semis)
    n = min(fade * 2, len(tail), len(head))
    w = np.linspace(0, 1, n, dtype=np.float32)
    mixed = head[-n:] * (1 - w) + tail[:n] * w
    return np.concatenate([head[:-n], mixed, tail[n:]])

def act(text, tag):
    """the line, sentence by sentence, in the tone of its feeling"""
    t = TONES.get(tag, TONES["calm"]); out = []
    for s in sentences(spoken(text)):
        kind = "!" if s.endswith("!") else "?" if s.endswith("?") else "."
        sp = SPEED * t["speed"] * (1.04 if kind == "!" else 0.98 if kind == "?" else 1.0)
        x, _ = kokoro().create(s, voice=VOICE, speed=sp, lang="en-us"); x = x.astype(np.float32)
        import librosa; x, _ = librosa.effects.trim(x, top_db=40)
        x = shift(x, t["pitch"] + (t["bright"] if kind == "!" else 0.0))
        if kind == "?": x = glide(x, t["rise"])
        elif kind == ".": x = glide(x, -t["fall"])
        x = x * (10 ** (t["gain"] / 20))
        out.append(x); out.append(np.zeros(int(SR * t["pause"]), dtype=np.float32))
    return np.concatenate(out)

def record(text, tag="calm"):
    key = hashlib.md5((text + tag + str(PITCH) + VOICE + str(SPEED) + "kokoro-acted-v2").encode()).hexdigest()[:12]
    cached = os.path.join(CACHE, key + ".raw")
    if os.path.exists(cached): return open(cached, "rb").read()
    raw = os.path.join(CACHE, key + "_tts.wav")
    samples = act(text, tag)
    import soundfile as sf; sf.write(raw, samples, SR)
    af = (f"rubberband=pitch={PITCH}:tempo=1.0:formant=preserved:pitchq=quality,"
          "silenceremove=start_periods=1:start_threshold=-40dB,areverse,silenceremove=start_periods=1:start_threshold=-40dB,areverse,"
          "highpass=f=200,acompressor=threshold=-20dB:ratio=3:attack=5:release=90:makeup=7dB,alimiter=limit=0.85:level=false,"
          "afade=t=in:d=0.015,apad=pad_dur=0.03")
    pcm = subprocess.run(["ffmpeg", "-v", "error", "-y", "-i", raw, "-af", af, "-ac", "1", "-ar", str(RATE), "-f", "s16le", "-"], check=True, capture_output=True).stdout
    os.remove(raw); open(cached, "wb").write(pcm)
    return pcm

def main():
    os.makedirs(CACHE, exist_ok=True)
    lines = []; tags = {}
    for l in open(LINES):
        l = l.rstrip("\n")
        if not l: continue
        tag, text = l.split("\t", 1) if "\t" in l else ("calm", l)
        if text not in tags: lines.append(text); tags[text] = tag
    blob = bytearray(); index = []
    for i, text in enumerate(lines):
        pcm = record(text, tags[text])
        if SQUEEZE != 1.0:   # the whole set a touch quicker so it fits the partition (applied after the cache, so it is cheap to change)
            pcm = subprocess.run(["ffmpeg", "-v", "error", "-f", "s16le", "-ar", str(RATE), "-ac", "1", "-i", "-", "-af", f"atempo={SQUEEZE}",
                                  "-f", "s16le", "-ar", str(RATE), "-ac", "1", "-"], input=pcm, check=True, capture_output=True).stdout
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
