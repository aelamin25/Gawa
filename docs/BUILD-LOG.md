# Gawa build log

A diary of how Gawa got made. One entry per step. Each one says **what we wanted**, **what we did**,
**did it work**, and **so what** (why it matters). Written so a nine year old could follow it.
Newest at the bottom.

---

## Step 1 – Unboxing (20 Oct 2026)

**What we wanted:** to know what the little board can do.

**What we did:** plugged in a Waveshare ESP32-S3-ePaper-1.54. It is a tiny computer with a 1.54 inch black and white
screen (the kind that does not glow, like a Kindle), a microphone behind the grille on the side, a small speaker, and two
buttons. It showed a clock and the room temperature. That was the factory program.

**Did it work:** yes. We learned it is the V2 version (8 MB of storage, 8 MB of memory).

**So what:** everything we build has to fit on this. No big screen, no colour, no internet unless we add it.

---

## Step 2 – v1: "happy" and "sad" (20 Oct 2026)

**What we wanted:** say a word, see a face.

**What we did:** wrote a small program (an Arduino "sketch") that uses Espressif's speech recogniser, which runs on the
chip itself with no internet. It knew two words, "happy" and "sad", and drew two faces pixel by pixel.
Loading it needs no software: hold the gear button while plugging in, open Chrome at espressif.github.io/esptool-js,
Connect, Erase, pick the file, Program.

**Did it work:** first time. Say "happy", it smiles.

**So what:** the hard part (hearing words offline) is possible on this board. Everything after this is making it nicer.

---

## Step 3 – The brief (20 Oct 2026)

**What we wanted:** to agree what we are building before building more.

**What we did:** wrote a one page brief. Problem in one line: *there is no low effort way to be heard for ten seconds
without being managed, judged or ignored.* Who it is for, 4 things that get in their way, 5 rules it must follow, 3 ways
to build it, and 1 test (30 real sentences, pass at 25). Saved as `Tamagotchi Companion Brief.docx`.

**Did it work:** yes, and we keep coming back to it.

**So what:** when we argue about a feature, the brief settles it. "Never nag" is in there, so Gawa never nags.

---

## Step 4 – v2: 100 phrases and a name (20 Oct 2026)

**What we wanted:** idea 1 from the brief. More phrases, more faces, a wake word.

**What we did:** 100 phrases in `phrases.h` (happy, sad, thinking, hello). Saying "Hey Gawa" opens a 10 second listening
window so the TV cannot change its mood. 3 faces and 8 reply lines per mood, never the same twice in a row.

**Did it work:** yes. "Gawa" is a made up word but the chip guessed the sound fine.

**So what:** Gawa has a name, and it only listens when you talk to it. That is the start of it feeling like someone.

---

## Step 5 – v3: a cloud brain (built, parked)

**What we wanted:** idea 2. Send what you say to OpenAI and get back a reply in Gawa's own words.

**What we did:** built it: record until you stop talking, send the sound to the cloud, show the reply. Setup through a
web page Gawa serves on its own Wi-Fi, so no coding. Then we decided to stay offline for now. Code kept in `extras_cloud/`.

**Did it work:** it compiles. Never tested on the board.

**So what:** it is there when we want it. Staying offline means no accounts, no passwords, no cost, and it never breaks
when the Wi-Fi does.

---

## Step 6 – v4: patterns, not sentences (20 Oct 2026)

**What we wanted:** "my cat died" should also work for dog, hamster, gran. And if it hears you but does not understand,
it should say so.

**What we did:** phrases became *families*: a start + a word + an ending. "my" + (cat, dog, gran...) + (died, is sick...).
The reply uses the word it heard: "SO SORRY ABOUT YOUR HAMSTER". A loudness detector tells talking from silence: talked
but no match, puzzled face, "SORRY, SAY THAT AGAIN?"; said nothing, "I AM HERE WHEN YOU NEED ME".

**Did it work:** compiles, previews right.

**So what:** one line in a list teaches Gawa a whole set of sentences. And it stops ignoring you when it does not understand,
which is the most "robot" thing a robot can do.

---

## Step 7 – v5: Gawa becomes a penguin (20 Oct 2026)

**What we wanted:** make Gawa cute.

**What we did:** first try was a huge face filling the screen with heavy dots. Not cute. Lesson: cute is *small* with empty
space around it, thin lines, light shading, tiny eyes set low, a barely there beak, blush under the eyes. Second try was a
small chibi penguin drawn in code with short animations: bounce, tears, thought bubble, blink, Zs.

**Did it work:** better, but still "made of circles".

**So what:** we learned what cute means on this screen. That lesson made step 8 possible.

---

## Step 8 – v6: real pencil sketches on the screen (20 Oct 2026)

**What we wanted:** faces as good as a real drawing.

**What we did:** took a sheet of eight hand drawn penguin emotions, cut each one out, shrank it to 200 x 150 pixels and
turned it black and white with *dithering* (the shading becomes a fine dot texture). Stored the dots inside the program.
Animations are drawn on top: bounce, falling tears, steam puffs for angry, a bobbing question mark, growing Zs.
New tool: drop a PNG in `sketches/`, run `python3 tools/make_sketches.py`, upload.

**Did it work:** yes, on the real screen. The photos show the sketch looking like the drawing.

**So what:** the screen can show real art. Anyone can draw a new face for Gawa without writing code.

---

## Step 9 – v7: 22 moods and talking to Gawa (20 Oct 2026)

**What we wanted:** many more feelings, and let people talk *to* Gawa, not only *at* it.

**What we did:** a second sheet of 23 drawings went through the same tool. 24 faces, 22 moods (scared, grumpy, grateful,
proud, curious, confident, peaceful, mischievous, thoughtful, relief, shocked, disgusted, nervous, tired, shy and the rest).
51 patterns, 391 sentences, just under the chip's limit of 400. "How are you", "what is your name", "do you like me",
"tell me a joke" all get answers.

**Did it work:** compiles, every pattern previewed with its face. Not yet seen on the board.

**So what:** Gawa went from three moods to the whole range a person has. Now it can match you instead of just sorting you.

---

## Step 10 – v8: a proper conversation, like a smart speaker (20 Oct 2026)

**What we wanted:** say "Hey Gawa" once at the start, then just talk.

**What we did:** copied how Google Home does it. "Hey Google" starts a conversation; after each answer the speaker keeps
listening about 10 seconds; the conversation ends after 10 seconds of silence or when you say "thank you" or "stop".
Gawa now does the same: "Hey Gawa" (or any sentence with its name in it) starts a chat; after every reply it listens for
10 more seconds and shows a small "(((" mark top left while it does; "thank you", "stop", "that is all" or "good night"
close it. Then it is "Hey Gawa" again. 52 patterns, 397 sentences.

**Did it work:** compiles. Not yet seen on the board.

**So what:** talking to Gawa feels like talking, not like pressing buttons with your voice.

---

## Step 11 – v9: no flashes, better questions, and a tidy folder (20 Oct 2026)

**What we wanted:** the first real test video showed two problems. The screen flashed black in the middle of a chat
(it looked like a crash), and Gawa asked "how was your day?" and then could not understand the answer.

**What we did:**
- The flashing is the e-paper's "clean" refresh. It now only happens when Gawa goes back to resting, never mid chat.
- Gawa only asks questions it can hear the answers to: "GOOD DAY OR BAD DAY?" And it learned the 27 ways people answer
  that: "good", "pretty good", "not great", "rubbish", "fine", "long", "busy"...
- When it still misses, it stays playful: magnifying glass face, "MY EARS! ONE MORE TIME?", and after two misses
  "I DID NOT GET IT, BUT I AM HERE" instead of a telling off.
- Tidied the whole folder for GitHub: `docs/` (diary, brief, tests), `firmware/Gawa/` (the program),
  `releases/` (files to load), `artwork/` (drawings), `tools/`, `extras/`. The README now explains Gawa like you are nine.

**Did it work:** compiles, 398 sentences from 55 patterns. Waiting for the next video.

**So what:** a companion that asks a question it cannot understand the answer to is a trap. Now every question Gawa asks
has answers it knows. And anyone landing on GitHub can find their way around.

---

## How to add the next entry

Copy the shape above: **What we wanted**, **What we did**, **Did it work**, **So what**. Say what broke too; that is the useful part.
