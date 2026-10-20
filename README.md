# Pengu, the desk pet that listens

![Pengu on its screen](artwork/previews/v7_preview.png)

## The problem, in one line

**There is no low effort way to be heard for ten seconds without being managed, judged or ignored.**

Texting a friend means waiting and feeling like a bother. A journal feels like homework.
A phone assistant hands you a list of tasks. So the feeling stays unsaid, and small things pile up.

## What Pengu is

Pengu is a tiny penguin that lives on your desk. You talk to it out loud. It listens, makes a face,
and says something kind. It never gives advice, never nags, never needs an app or a login.
It runs on a small board with a microphone and a black and white screen, with no internet at all.

## How you talk to it

1. Say **"Hey Pengu"**. Its ears go up.
2. Say your thing. "I passed my driving test." "My cat is sick." "I am so tired."
3. It makes a face and answers. "YOU PASSED YOUR DRIVING TEST!" "SO SORRY ABOUT YOUR CAT." "REST, FRIEND."
4. Keep talking. For ten seconds after every answer it keeps listening (a little "(((" mark shows).
5. Say "thank you", "okay stop", "that is all" or "good night" to finish. Or just go quiet.

**How it understands you (with no internet):** it listens for the short chunk that carries the feeling, like "was bad",
"so tired", "cat died" or "passed my", and spots it inside whatever you say. So "my day was bad", "honestly it was a bad day"
and "today was pretty bad" all get the sad face. It knows 399 chunks in 66 feeling families, with 24 hand drawn faces, and asks little questions back that it can hear the answer to. When it hears you but
catches nothing, it says "MM HMM. GO ON" and keeps listening, so you never feel ignored.
You can also ask it things: "how are you?", "what is your name?", "do you like me?", "tell me a joke".

## What is in this folder

| Folder | What is inside | Who it is for |
|---|---|---|
| `docs/` | The brief, the build diary (`BUILD-LOG.md`), the test sheet, how to use GitHub | Everyone. Start with the build diary. |
| `releases/` | Ready made files to load onto the board. Use the newest one. | People who want to run Pengu. |
| `docs/VOICE-TAGS.md` | Every line Pengu says, under the feeling it is said with. | Anyone tuning the voice. |
| `docs/HANDOVER.md` | Where we are and what to build next. Read this first when picking the project back up. | Anyone continuing the work. |
| `firmware/Pengu/` | The program itself (Arduino). `phrases.h` is what it understands, `faces.h` is how it draws. | People who want to change Pengu. |
| `artwork/` | The blips (`sounds/`), the 24 penguin drawings (plus 75 spare ones in `extra-75/`), the original sheets, and preview pictures | Anyone who wants to draw a new face. |
| `tools/` | `make_sketches.py` turns drawings into something the board can show | People who add drawings. |
| `extras/` | A parked "cloud brain" version that talks to the internet. Not used right now. | Later. |

## Load Pengu onto the board (2 minutes, no software to install)

1. Unplug the board. Hold the **gear** button, plug the cable in, let go.
2. In Chrome open https://espressif.github.io/esptool-js/ → **Connect** → pick the "USB JTAG" port.
3. Click **Erase Flash**, wait for "completed".
4. Flash Address: **0x0**. Choose File → `releases/` → the newest `Pengu_v…_full_8MB.bin`.
5. Click **Program**. Wait for 100% and "Hash of data verified".
6. Unplug, plug back in. It says "SAY HEY PENGU".

## Change what it understands (Arduino IDE)

Open `firmware/Pengu/Pengu.ino`. Board **ESP32S3 Dev Module**, USB CDC On Boot Enabled, Flash 8MB, Flash Mode QIO,
PSRAM OPI, Partition Scheme **Custom**, Erase All Flash **Disabled**. Upload.

- New sentences: add a word to a list in `phrases.h`. One word teaches a whole family of sentences.
- New faces: put a PNG in `artwork/sketches/`, add its name to `NAMES` in `tools/make_sketches.py`, run
  `python3 tools/make_sketches.py`, then add `SK_YOURNAME` to a family's face list (the `F_...` lines in `phrases.h`), upload.
- No voice? Press the gear button once: it starts a conversation, just like "Hey Pengu". Hold it for a second to see demo moods.
- Stuck? Hold the power button for a moment: Pengu restarts from scratch.
- Pengu asks little questions back ("WANT A HUG?"). Answer "yes" or "no". Yes and no only count right after a question.
- Timings (listening window, rest time) and the voice volume (`VOICE_VOLUME`, 0 = silent): the `#define` lines at the top of `Pengu.ino`.
- The voice: Pengu says exactly what the screen says. After changing any line in `phrases.h`, run the two commands at the top of `tools/make_speech.py` (needs `pip install kokoro-onnx soundfile`, ffmpeg, and the two Kokoro voice files; see the top of that script), then flash `releases/speech.bin` at `0x620000` (or rebuild the full file).
- New blips: `tools/make_sounds.py`.

## The board

Waveshare ESP32-S3-ePaper-1.54 (V2): a 1.54 inch e-paper screen (200 x 200 dots), a microphone, a small speaker,
two buttons, 8 MB of storage. The speech recognition is Espressif's ESP-SR and runs on the chip.
