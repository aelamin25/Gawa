# Pengu handover (20 Oct 2026, end of chat 2)

Read this first in the next chat. It says where everything is, what is on the board, what the last tests showed, how
the loop and the voice work, and what to do next. The next job is a brainstorm: how to make Pengu fun and not just a
kid's toy.

## The one sentence

There is no low effort way to be heard for ten seconds without being managed, judged or ignored. Pengu is that way.
Everything serves that sentence. Do not overcomplicate it.

## What Pengu is today (v13, built, not yet flashed)

A pencil-sketch penguin on a Waveshare ESP32-S3-ePaper-1.54 (V2). Fully offline. You say "Hey Pengu" (or press the
gear), say how you feel, and it shows a face, writes one short line, and SAYS that line in a young anime voice that acts
the feeling. Most lines end with a little question it can actually hear the answer to ("WANT A HUG?", "REST WITH ME?",
"SHALL WE CELEBRATE?", "TELL ME MORE?") and "yes" or "no" gets a real reply (BIG HUG. SQUEEZE with a heart-eyes face;
OKAY. RESTING TOGETHER with Zs; WOO! DANCE TIME; GO ON. I AM LISTENING). When it hears you but catches nothing, it stays
present ("MM HMM. GO ON", then "I HEAR YOU. I AM HERE" with a heart). The face only changes when a feeling is understood.
It never advises, judges or nags.

Numbers: 66 feeling families, 399 chunks (the chip holds 400), 24 pencil faces, 292 spoken lines, 17 feeling tags.

## Where things are

- Mac folder `~/Desktop/Tamagotchi`, pushed to https://github.com/aelamin25/gawa (repo still named gawa; renaming it
  on GitHub is optional, old links redirect). Last commit at the time of writing: check `git log`; v13 may still be
  uncommitted (M commits and pushes from Terminal; I never run git on the Mac).
- `README.md` (kid-readable overview, folder map, how to flash, how to change things).
- `docs/BUILD-LOG.md`: Steps 1 to 18, each with What we wanted / What we did / Did it work / So what. Step 18 is v13.
- `docs/TEST-SHEET.md`: lines 1 to 90. The ones that matter now: 62 to 69 (calm loop), 70 to 78 (questions and yes/no),
  79 to 81 (the name), 87 to 90 (voice and the "S" corner number).
- `docs/VOICE-TAGS.md`: every spoken line under its feeling tag.
- `docs/DEMO-SCRIPT.md`, `docs/HOW-TO-GITHUB.md`, the brief (`docs/Pengu-Brief.docx`).
- `firmware/Pengu/`: `Pengu.ino` (the loop), `phrases.h` (what it hears and says, the families, the questions and
  answers, which face each family uses), `faces.h` (drawing, animations, the "(((" mark and the heart), `sketches.h`
  (24 faces, generated), `sounds.h` (3 chiptune blips, generated), `speech.h` (the voice player: ADPCM decoder,
  background task), `speech_index.h` (where each spoken line lives, generated), `audio_bsp.c/h` (mic and speaker),
  `partitions.csv` (the flash map), `src/` (Waveshare display and codec drivers).
- `releases/`: `Pengu_v13_full_8MB.bin` (everything, flash at 0x0 after Erase Flash), `speech.bin` (the voice on its
  own, flash at 0x620000 if you upload the program from Arduino IDE), older versions (v2 to v12; up to v11 named Gawa).
- `artwork/`: `originals/penguin_emotions_sheet.jpg` (the 24-face sheet), `sketches/` (the 24 cut-outs, plus
  `extra-75/` from the big sheet, not compiled), `sounds/` (the blips, plus `old-words/`), `previews/`.
- `tools/`: `crop_sheet.py` + `crop_boxes.txt` (cut faces from the sheet), `make_sketches.py` (PNG to sketches.h,
  12 px margin), `make_sounds.py` (blips), `list_lines.cpp` (every line with its feeling tag), `make_speech.py` (records
  the lines with Kokoro, acts the tags, packs speech.bin + speech_index.h), `speech_lines.txt`.
- The brief doc: https://claude.ai/code/artifact/7a717ea6-ebef-4ac3-b0da-1671e3aa07ad
- The progress deck: https://claude.ai/artifact/BHGcGnigrFFFk6ZU2QGeRM. Sections 1 and 2 published (cover, hook, agenda,
  goal, who, hard, rules), still says Gawa; sections 3 and 4 (how we built it, steps, sentences, faces, talks; test,
  next, log) wait for M to say "go". Follows the `presentation-format` skill.

## How to build and flash (what the chat does, then what M does)

The chat builds in its own workspace: `firmware/Pengu/*` copied to a folder named `Tamagotchi/` with the .ino renamed
`Tamagotchi.ino`; `arduino-cli compile --fqbn "esp32:esp32:esp32s3:PSRAM=opi,FlashSize=8M,PartitionScheme=custom,
CDCOnBoot=cdc,USBMode=hwcdc,FlashMode=qio"`; then esptool `merge-bin` with bootloader 0x0, partitions 0x8000,
boot_app0 0xe000, app 0x10000, srmodels.bin 0x2F0000, speech.bin 0x620000 (flash map: app0 0x10000 size 0x2E0000,
model 0x2F0000 size 0x330000, speech 0x620000 size 0x1E0000). Older versions had the model at 0x410000, so a flash of
v13 or later MUST start with Erase Flash. The chat writes files to the Mac folder and M flashes with
https://espressif.github.io/esptool-js/ : hold the gear while plugging in, Connect, Erase Flash, address 0x0, Program.

Making the voice again (after any change to a line in phrases.h): the two commands at the top of `tools/make_speech.py`.
Needs kokoro-onnx, soundfile, librosa, ffmpeg with rubberband, and the Kokoro files (kokoro-v1.0.onnx + voices-v1.0.bin,
350 MB, on the Mac in `Desktop/Himalayan Hop/models/kokoro/` or from the kokoro-onnx releases page). About 8 minutes
for all 292 lines; a cache under `tools/speech_cache/` makes small edits quick.

## The voice, and how we got here (so the next chat does not repeat it)

Rounds: piper "amy" pitched up (robotic, too fast); piper "hfc_female" (robotic); Kokoro "Heart" from Himalayan Hop and
seven English blends ("all terrible"); then Kokoro's Japanese voices speaking the English lines: M picked "Alpha" (jf_alpha)
from seven. Then "too monotone": every line got a feeling tag and the voice acts it (pace, a little pitch, loudness,
questions rise, sad lines fall, a breath between sentences). Then "better but less robotic": the pitch moves were
halved and done with a shifter that keeps the voice's colour (ffmpeg rubberband, formant preserved), not the
phase-vocoder one. That is what is in v13. M has not yet said it is good; the last sample sent was
`pengu_voice_acted_v2.wav`. Each tone is one row of `TONES` in `make_speech.py`; tune a feeling, not the whole voice.

## What the board has told us (hardware facts)

- Screen: partial refresh 0.3 s, no flashes between chats (full refresh only at boot, good night, falling asleep,
  hourly while asleep). Faces whole after the re-crop (the first crops cut the flippers).
- Speaker: works. The codec's volume scale is steep (50 means -25 dB); VOICE_VOLUME is 85. In v12 the boot sounds
  were heard but NOTHING after the speech engine started: still unexplained. v13 shows a tiny "S<number>" top right
  after each spoken line (S0 = every sample taken; anything else = failed writes or an error code). It is the one clue
  to collect first. `SHOW_AUDIO_DEBUG 0` in Pengu.ino turns it off once known.
- Recognition: short chunks inside sentences work ("how are you? bad, very bad" worked). A feeling said in the same
  breath right after a matched phrase ("how are you? it was good") can be lost: the chip re-arms after a match. Known,
  no fix; answer after the question.
- Gear: starts a chat the moment you press (v13); what you said in the 3 s before is answered when the chat opens.
- Power button: hold a moment = full restart (polarity learned at boot).

## The loop, exactly (Pengu.ino)

Start: "hey pengu" (or gear) -> same face, "(((" mark, "I AM LISTENING" spoken. Talk up to 10 s; the mark pulses while
you talk. A chunk matched -> that family's face (one drawing per family per conversation), the line shown then spoken
(voice starts as the words become readable), animation underneath; if the family asks (Q_DAY/Q_HUG/Q_REST/Q_TELL/Q_PARTY)
"yes"/"no"/"good"/"bad" count for the next 10 s only. Nothing matched -> "MM HMM. GO ON" (same face), then the heart:
"I HEAR YOU. I AM HERE" (or the sad/happy version), chat closed, face stays 2 min. Ends: "thank you", "bye bye",
"okay stop", "leave me alone", "good night" (sleeps). While Pengu speaks the mic's talk detector pauses and a phrase that
is a whole-word part of its own line is ignored (it cannot answer its own "GOOD OR BAD?"), but you talking over it counts.

## Open decisions for M

- Keep the quiet "crisis" family ("want to die" etc.: Pengu stays and says it cares, no advice, no number)?
- Demo cycle on a long gear press: keep?
- The voice: good enough, or tune a tone?
- Rename the GitHub repo to pengu?

## Next: the brainstorm (how to make it fun, not just a kid's toy)

Ground rules M has set: offline; simple; do not overcomplicate; cute = clear and useful, no emotions for the sake of it;
never advise, judge or nag; one feature at a time, grilled, then a video with transcript.

Seeds to start from (not decisions):
- A daily ritual: morning "GOOD DAY OR BAD DAY?", evening "HOW WAS TODAY?", and Pengu remembers the answer overnight
  ("YESTERDAY WAS ROUGH. HOW IS TODAY?"). One counter in flash; big change in how it feels.
- Pengu with a life of its own: naps, gets hungry at lunchtime, looks around when bored, reacts to a tap (the gear),
  celebrates a streak of good days. Tamagotchi without the guilt.
- Adult-grade honesty: lines for work stress, money, loneliness, decisions, hangovers, deadlines, dating. The chunk list
  is the product; 399 is the cap, so each new chunk replaces a weak one.
- Shared moments: "say something nice to someone", a nightly "one good thing", a mood tally shown as a tiny bar.
- The voice as character: catchphrases, a laugh, a yawn, a sigh (short clips, cheap).
- Hardware cheapness: a stand, a lanyard, a tiny case, a sticker sheet of the 24 faces.
- What to resist: a screen full of stats, cloud anything, reminders and nudges (those are the "managed" in the one sentence).

## First steps in the next chat, in order

1. Read this file and `docs/BUILD-LOG.md` Step 18; `git log` to see what M committed.
2. Ask M for the v13 test result if not in the chat yet: voice heard after boot? the S number? the gear + "I am sad" test.
3. Brainstorm with M in the chat first (no building): a shortlist of 5 ideas, each in one line with the "so what",
   pick one, write its Pass test, then build it as v14.
4. The chat's workspace is fresh: copy `firmware/Pengu/*` in, rename the .ino, and copy `releases/speech.bin` and the
   model partition as above before building.
