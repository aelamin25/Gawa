"""
make_sounds.py - turn little words into sounds.h for Pengu.

How to use (from the main folder):
    python3 tools/make_sounds.py

For each word in WORDS it makes a tiny spoken clip with piper (a free offline voice), pitches it up so it sounds
small and penguin-like, then crushes it to 8 bits like a 90s handheld game (think Animal Crossing voices on a Game Boy),
keeps it quiet, and writes the result into firmware/Pengu/sounds.h as 16 kHz mono samples.
Pengu plays the clip whose word matches the FIRST WORD of what it says: "OH NO. WANT A HUG?" plays "oh",
"BIG HUG. SQUEEZE" plays "big". A reply that starts with a word that has no clip is silent.
BLIPS are pure chiptune: square waves with a quick fade, like a menu "select" or a coin: LISTEN (rising, when Pengu starts
listening), HEART (three-note jingle, the "I heard you" moment), SLEEP (falling, good night).

Needs: pip install piper-tts, and ffmpeg. The voice file en_US-lessac-medium.onnx is downloaded once with
    python3 -m piper.download_voices en_US-lessac-medium
The finished clips are also saved as .wav in artwork/sounds so you can listen to them.
"""
import os, subprocess, struct, sys

HERE = os.path.dirname(os.path.abspath(__file__)); ROOT = os.path.dirname(HERE)
VOICE = os.path.join(HERE, "en_US-amy-medium.onnx")   # a soft voice; pitched up a little, not chipmunked
OUT = os.path.join(ROOT, "firmware", "Pengu", "sounds.h")
WAVS = os.path.join(ROOT, "artwork", "sounds")
WORDS = {}   # since v13 the words are spoken whole lines (tools/make_speech.py); only the blips live here
PITCH = 1.25       # higher = smaller penguin (1.0 = the voice as it is). Pitch only: the words do not get faster
TEMPO = 0.9        # lower = slower, gentler words
BITS = 8           # 8-bit era texture, mixed in lightly so the words stay clear (fewer bits = crunchier)
CRUNCH = 0.15      # how much of the 8-bit texture to mix in (0 = clean voice, 1 = full crunch)
# the board's speaker is tiny: it cannot do deep sounds and it is quiet, so the words are squeezed (compressed) and
# pushed up close to full scale; VOICE_VOLUME in Pengu.ino then sets how loud it actually is
RATE = 16000
# chiptune blips: list of (frequency Hz, length s); 0 Hz = a rest
BLIPS = { "listen": [(587, 0.12), (880, 0.18)],
          "heart":  [(523, 0.14), (659, 0.14), (784, 0.30)],
          "sleep":  [(784, 0.16), (587, 0.16), (392, 0.34)] }

def blip(notes, level=0.45):
    """soft triangle-wave notes (a rounder, gentler chip sound than a square) with a slow fade, 16 kHz 16-bit mono"""
    out = []
    for freq, secs in notes:
        n = int(RATE * secs)
        for i in range(n):
            if freq == 0: out.append(0); continue
            phase = (i * freq / RATE) % 1.0
            tri = 4.0 * abs(phase - 0.5) - 1.0
            env = min(1.0, i / (RATE * 0.012)) * (1.0 - i / n) ** 1.2    # gentle attack, long fade
            out.append(int(32767 * level * env * tri))
    out += [0] * int(RATE * 0.05)
    return struct.pack(f"<{len(out)}h", *out)

def main():
    os.makedirs(WAVS, exist_ok=True)
    clips = []
    for name, text in WORDS.items():
        raw = os.path.join(WAVS, f"raw_{name}.wav"); wav = os.path.join(WAVS, f"{name}.wav")
        subprocess.run(["piper", "-m", VOICE, "-f", raw, "--length-scale", "1.1"], input=text.encode(), check=True, capture_output=True)
        af = (f"rubberband=pitch={PITCH}:tempo={TEMPO},aresample=16000,"
              "silenceremove=start_periods=1:start_threshold=-40dB,areverse,silenceremove=start_periods=1:start_threshold=-40dB,areverse,"
              f"acrusher=bits={BITS}:samples=2:mode=lin:mix={CRUNCH},highpass=f=250,lowpass=f=6000,"
              "acompressor=threshold=-20dB:ratio=4:attack=5:release=80:makeup=10dB,alimiter=limit=0.85:level=false,"
              "afade=t=in:d=0.015,apad=pad_dur=0.08")
        subprocess.run(["ffmpeg", "-v", "error", "-y", "-i", raw, "-af", af, "-ac", "1", "-ar", "16000", wav], check=True)
        pcm = subprocess.run(["ffmpeg", "-v", "error", "-i", wav, "-f", "s16le", "-ac", "1", "-ar", "16000", "-"], check=True, capture_output=True).stdout
        os.remove(raw)
        clips.append((name, pcm))
    for name, notes in BLIPS.items():
        pcm = blip(notes); clips.append((name, pcm))
        subprocess.run(["ffmpeg", "-v", "error", "-y", "-f", "s16le", "-ac", "1", "-ar", str(RATE), "-i", "-", os.path.join(WAVS, f"{name}.wav")], input=pcm, check=True)
    with open(OUT, "w") as f:
        f.write("// sounds.h - tiny penguin words, 16 kHz 16-bit mono. Made by tools/make_sounds.py. Do not edit by hand.\n#pragma once\n#include <stdint.h>\n\n")
        for name, pcm in clips:
            n = len(pcm) // 2
            f.write(f"static const int16_t SFX_{name.upper()}[{n}] = {{\n")
            vals = struct.unpack(f"<{n}h", pcm)
            for i in range(0, n, 20):
                f.write("  " + ",".join(str(v) for v in vals[i:i + 20]) + ",\n")
            f.write("};\n")
        f.write("\nstruct Sfx { const char *word; const int16_t *data; int n; };\nstatic const Sfx SOUNDS[] = {\n")
        for name, pcm in clips:
            f.write(f'  {{ "{name.upper()}", SFX_{name.upper()}, {len(pcm) // 2} }},\n')
        f.write("};\n")
    print(f"wrote {OUT} with {len(clips)} sounds")

if __name__ == "__main__":
    main()
