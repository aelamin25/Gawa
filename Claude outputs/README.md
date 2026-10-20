# Gawa v3 – a warm little companion (Waveshare ESP32-S3-ePaper-1.54, V2 board)

Say **"Hey Gawa"** → its ears go up and it listens.
Say anything about your day → Gawa thinks for a few seconds, then shows a face and answers in its own words.
Keep talking for 12 seconds after a reply, no need to say "Hey Gawa" again.
No Wi-Fi? Gawa still reacts to 100 built-in phrases (see `Tamagotchi/phrases.h`).

Hold the gear button for 5 seconds → the setup page. Short press → flips moods by hand.

## Part 1 – Get an OpenAI key (one time, 5 minutes)

1. Go to https://platform.openai.com and sign in (or make an account).
2. Add a little credit under Billing (£5 lasts a long time: each chat with Gawa costs well under a penny).
3. Go to API keys → **Create new secret key** → name it "Gawa" → copy the key (starts with `sk-`).
   Paste it somewhere safe like your phone's Notes. You will type it into Gawa's page in Part 3.
   If you ever pasted a key into a chat or email, delete that key here and make a new one.

## Part 2 – Load Gawa v3 onto the board (2 minutes)

1. Unplug the pet. Hold the **gear** button, plug the cable in, let go.
2. In Chrome open https://espressif.github.io/esptool-js/ → **Connect** → pick the "USB JTAG" port.
3. Click **Erase Flash**, wait for "completed".
4. Flash Address: **0x0**. Choose File → Desktop → Tamagotchi → **Gawa_v3_full_8MB.bin**.
5. Click **Program**. Wait for 100% and "Hash of data verified" (about a minute).
6. Unplug, plug back in.

## Part 3 – Tell Gawa your Wi-Fi and key (3 minutes, on your phone)

1. Gawa's screen says **JOIN WIFI GAWA-SETUP**.
2. On your phone: Settings → Wi-Fi → join **Gawa-Setup** (no password).
3. A page called "Hi, I am Gawa" pops up. (If it does not, open the browser and go to `192.168.4.1`.)
4. Type your home Wi-Fi name, its password, and paste the OpenAI key. Leave the two model boxes as they are.
5. Tap **Save and restart Gawa**. Gawa restarts and joins your Wi-Fi.
6. Put your phone back on your normal Wi-Fi.

Gawa now says "SAY HEY GAWA". Try: "Hey Gawa" … "I had a really good day today".

## What to expect

- ears up (listening) → "HMM..." (asking the cloud, 2–5 seconds) → face + reply, and what it heard in small letters.
- Replies are short and warm. Gawa never gives advice or lectures.
- It remembers the last few things you said, so follow-ups make sense. After 10 quiet minutes it starts fresh.
- **BAD KEY**: the key was typed wrong. Hold the gear 5 s and re-enter it.
- **NO WIFI** / "I cannot reach the cloud": it could not join your Wi-Fi (name or password wrong, or a 5 GHz-only network; Gawa needs 2.4 GHz).
- **BRAIN MODEL?** or **HEARING MODEL?**: OpenAI renamed a model. Hold the gear 5 s and type the new name in that box.

## For editing in Arduino IDE (optional)

Board **ESP32S3 Dev Module**, USB CDC On Boot Enabled, Flash 8MB, Flash Mode QIO, PSRAM OPI,
Partition Scheme **Custom**, Erase All Flash **Disabled**. Open `Tamagotchi/Tamagotchi.ino`.
Gawa's personality is the `SYSTEM_PROMPT` text at the top of `cloud.cpp`. Faces are in `faces.h`, offline phrases in `phrases.h`.

Note: the board talks to OpenAI over HTTPS without checking the certificate (simplest for a desk toy).
If you want to lock that down later, the place is `client.setInsecure()` in `cloud.cpp`.

## Files

- `Gawa_v3_full_8MB.bin` – the complete image for Part 2 (program + speech model)
- `Tamagotchi/` – the Arduino sketch and drivers
- `TEST.md` – the 30-line test from the brief
- `faces_preview.png`, `chat_preview.png` – what the screens look like
- older images `Tamagotchi_full_8MB.bin` (v1) and `Gawa_v2_full_8MB.bin` (v2, offline only) if you ever want to go back
