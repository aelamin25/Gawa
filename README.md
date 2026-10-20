# Gawa – a little offline companion (Waveshare ESP32-S3-ePaper-1.54, V2 board)

Current version: **v8** (`Gawa_v8_full_8MB.bin`) – 22 moods, 24 drawings, and a real conversation. Fully offline, no accounts, no Wi-Fi.
See `LOG.md` for the step by step story and `GITHUB.md` for putting this on GitHub.
See `LOG.md` for the story of how it was built, step by step.

**How a conversation works (like a smart speaker):** say **"Hey Gawa"** once. Its ears go up. Say your thing.
After every reply Gawa keeps listening for 10 seconds (a small "(((" mark shows top left), so you can just keep talking.
The chat ends after 10 seconds of quiet, or when you say "thank you", "stop", "that is all" or "good night".
Then it is "Hey Gawa" again. Any sentence with its name in it ("hey gawa how are you") starts a chat by itself.

It understands *patterns*, so all of these work:
- "my cat died", "my dog is sick", "my gran is in hospital" → sad face, "SO SORRY ABOUT YOUR GRAN"
- "I passed my driving test", "I got the job", "I won the race" → happy face, "YOU PASSED YOUR DRIVING TEST!"
- "I am thinking about work", "should I quit" → thoughtful face, "HMM, WORK. GO ON"
- "I feel lonely", "today was rough", "I miss my mum" → sad face with a kind line
- "I am so annoyed", "that is not fair" → angry penguin with steam puffs
- "good morning Gawa", "I am home" → a hello back

- "I am scared of the dark", "I am grumpy", "I am nervous", "I am so tired", "I have a crush" → scared / grumpy / nervous / tired / shy faces
- "phew, it is over", "you will not believe this", "that is gross", "I can do this", "I have a secret" → relief / shocked / disgusted / confident / mischievous

**Talk to Gawa itself:** "how are you?" (it answers and asks you back), "what is your name?", "do you like me?",
"what are you doing?", "are you sleepy?", "tell me a joke", "thank you", "good night".

If you talk and it does not understand: puzzled face, "SORRY, SAY THAT AGAIN?" and it listens again.
If you say nothing: "I AM HERE WHEN YOU NEED ME" and it rests. Gear button = flip moods by hand.

All 397 sentences come from 52 patterns in `Tamagotchi/phrases.h` (the chip's limit is about 400). Add a word to a list and Gawa learns a whole new set; each pattern names its face.

## Load it (2 minutes, no software to install)

1. Unplug the pet. Hold the **gear** button, plug the cable in, let go.
2. In Chrome open https://espressif.github.io/esptool-js/ → **Connect** → pick the "USB JTAG" port.
3. Click **Erase Flash**, wait for "completed".
4. Flash Address: **0x0**. Choose File → **Gawa_v8_full_8MB.bin**.
5. Click **Program**. Wait for 100% and "Hash of data verified".
6. Unplug, plug back in. It says "SAY HEY GAWA".

## Test it

`TEST.md` has 30 sentences with tick boxes. Pass = 25 or more right. Run it after every change.

## Change it (Arduino IDE)

Board **ESP32S3 Dev Module**, USB CDC On Boot Enabled, Flash 8MB, Flash Mode QIO, PSRAM OPI,
Partition Scheme **Custom**, Erase All Flash **Disabled**. Open `Tamagotchi/Tamagotchi.ino`.
- Words and replies: `phrases.h` (plain English, no contractions; replies under 48 characters, `%s` = the word heard)
- Faces: the 24 drawings in `sketches/`. Swap a PNG, run `python3 tools/make_sketches.py`, upload.
- Animations and which drawing each mood uses: `draw_pet()` in `faces.h`
- Gawa blinks every 25 s while resting and falls asleep after 10 quiet minutes (`#define`s at the top of `Tamagotchi.ino`)
- Timings (listening window, rest time): the `#define`s at the top of `Tamagotchi.ino`
- The speech model lives in its own part of the flash; Arduino uploads keep it as long as "Erase All Flash" stays Disabled.

## Files

- `Gawa_v8_full_8MB.bin` – current image (program + speech model)
- `GITHUB.md` – how to put this folder on GitHub and keep it updated
- `sketches/` – the 24 penguin drawings (PNG) the faces are made from, plus the two original emotions sheets
- `tools/make_sketches.py` – turns the PNGs into `Tamagotchi/sketches.h` (run it after changing a drawing)
- `v7_preview.png` (one sample per pattern), `sketches_preview.png`, `penguin_anim.gif` – what the faces look like
- `Tamagotchi/` – the Arduino sketch and drivers
- `extras_cloud/` – the parked "cloud brain" code from v3 (not part of the build)
- `LOG.md` – build diary · `TEST.md` – the test sheet · `Tamagotchi Companion Brief.docx` – the brief
- `faces_preview.png`, `v4_preview.png` – what the screens look like
- Older images: v1 `Tamagotchi_full_8MB.bin`, v2, v3 (cloud, untested), v4 (robot face), v5 (code-drawn penguin), v6 (8 sketches), v7 (22 moods)
