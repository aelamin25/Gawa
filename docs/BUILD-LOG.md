# Pengu build log

A diary of how Pengu got made. One entry per step. Each one says **what we wanted**, **what we did**,
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

**So what:** when we argue about a feature, the brief settles it. "Never nag" is in there, so Pengu never nags.

---

## Step 4 – v2: 100 phrases and a name (20 Oct 2026)

**What we wanted:** idea 1 from the brief. More phrases, more faces, a wake word.

**What we did:** 100 phrases in `phrases.h` (happy, sad, thinking, hello). Saying "Hey Pengu" opens a 10 second listening
window so the TV cannot change its mood. 3 faces and 8 reply lines per mood, never the same twice in a row.

**Did it work:** yes. "Pengu" is a made up word but the chip guessed the sound fine.

**So what:** Pengu has a name, and it only listens when you talk to it. That is the start of it feeling like someone.

---

## Step 5 – v3: a cloud brain (built, parked)

**What we wanted:** idea 2. Send what you say to OpenAI and get back a reply in Pengu's own words.

**What we did:** built it: record until you stop talking, send the sound to the cloud, show the reply. Setup through a
web page Pengu serves on its own Wi-Fi, so no coding. Then we decided to stay offline for now. Code kept in `extras_cloud/`.

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

**So what:** one line in a list teaches Pengu a whole set of sentences. And it stops ignoring you when it does not understand,
which is the most "robot" thing a robot can do.

---

## Step 7 – v5: Pengu becomes a penguin (20 Oct 2026)

**What we wanted:** make Pengu cute.

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

**So what:** the screen can show real art. Anyone can draw a new face for Pengu without writing code.

---

## Step 9 – v7: 22 moods and talking to Pengu (20 Oct 2026)

**What we wanted:** many more feelings, and let people talk *to* Pengu, not only *at* it.

**What we did:** a second sheet of 23 drawings went through the same tool. 24 faces, 22 moods (scared, grumpy, grateful,
proud, curious, confident, peaceful, mischievous, thoughtful, relief, shocked, disgusted, nervous, tired, shy and the rest).
51 patterns, 391 sentences, just under the chip's limit of 400. "How are you", "what is your name", "do you like me",
"tell me a joke" all get answers.

**Did it work:** compiles, every pattern previewed with its face. Not yet seen on the board.

**So what:** Pengu went from three moods to the whole range a person has. Now it can match you instead of just sorting you.

---

## Step 10 – v8: a proper conversation, like a smart speaker (20 Oct 2026)

**What we wanted:** say "Hey Pengu" once at the start, then just talk.

**What we did:** copied how Google Home does it. "Hey Google" starts a conversation; after each answer the speaker keeps
listening about 10 seconds; the conversation ends after 10 seconds of silence or when you say "thank you" or "stop".
Pengu now does the same: "Hey Pengu" (or any sentence with its name in it) starts a chat; after every reply it listens for
10 more seconds and shows a small "(((" mark top left while it does; "thank you", "stop", "that is all" or "good night"
close it. Then it is "Hey Pengu" again. 52 patterns, 397 sentences.

**Did it work:** compiles. Not yet seen on the board.

**So what:** talking to Pengu feels like talking, not like pressing buttons with your voice.

---

## Step 11 – v9: no flashes, better questions, and a tidy folder (20 Oct 2026)

**What we wanted:** the first real test video showed two problems. The screen flashed black in the middle of a chat
(it looked like a crash), and Pengu asked "how was your day?" and then could not understand the answer.

**What we did:**
- The flashing is the e-paper's "clean" refresh. It now only happens when Pengu goes back to resting, never mid chat.
- Pengu only asks questions it can hear the answers to: "GOOD DAY OR BAD DAY?" And it learned the 27 ways people answer
  that: "good", "pretty good", "not great", "rubbish", "fine", "long", "busy"...
- When it still misses, it stays playful: magnifying glass face, "MY EARS! ONE MORE TIME?", and after two misses
  "I DID NOT GET IT, BUT I AM HERE" instead of a telling off.
- Tidied the whole folder for GitHub: `docs/` (diary, brief, tests), `firmware/Pengu/` (the program),
  `releases/` (files to load), `artwork/` (drawings), `tools/`, `extras/`. The README now explains Pengu like you are nine.

**Then we grilled it before loading.** A second reviewer read every line without knowing what we expected, and found ten
gaps. The big ones: a reply text that could turn to rubbish after ten seconds (a pointer to memory that had been reused);
the gear button demo pointing at the wrong moods; single word phrases like "good", "stop" and "pengu" that the chip would
mishear from normal talk; a fan or TV that could keep Pengu "listening" forever; and eleven reply lines that quietly broke
our own rule by giving advice ("deep breath", "one step at a time", "rest now"). All fixed. 387 sentences from 55 patterns.

**Did it work:** compiles and previews. Waiting for the next video.

**So what:** a companion that asks a question it cannot understand the answer to is a trap, so now every question Pengu
asks has answers it knows. And reviewing before loading caught a crash that a test video would have shown us the hard way.

---

## Step 12 – v10: listen for the feeling, 99 faces, a complete offline companion (20 Oct 2026)

**What we wanted:** the second test video (with a transcript this time) showed the real problem. You said "my day was bad"
and Pengu only knew "today was bad". People do not talk from a list. We decided to stay offline and make it as finished
as it can be before any cloud, the way a good product is made: every path works, no rough edges. Then a big new sheet of
112 penguin expressions arrived, so the faces grew too.

**What we did:**
- Pengu now listens for short *chunks* that carry the feeling ("was bad", "so tired", "cat died", "passed my") and spots
  them inside whatever you say. "My day was bad", "honestly it was a bad day", "today was pretty bad" all land on the
  sad face.
- One-word answers ("bad", "fine", "rubbish") only count right after Pengu asks "good or bad?", so they cannot misfire
  from the TV. If you say "today was terrible" mid chat with no question pending, it still counts as sad.
- When Pengu hears you but catches no chunk, it stays present instead of complaining: thoughtful face, "MM HMM. GO ON";
  a second time, calm face, "I HEAR YOU. I AM HERE", and the chat closes gently.
- 75 new pencil faces cut from the big sheet (99 in all). Each feeling family now has its own little list of faces and
  picks one at random, so "sad" is not always the same drawing. The animation (tears, hearts, Zs, puffs) stays per family.
- 22 new feeling families so an evening of real talk is covered: hopeful, jealous, guilty, hopeless, bored, hungry,
  nostalgic, feeling loved, overwhelmed, focused, energetic, caring about someone else, sick, insecure, grateful for
  something, admiring, dread, wary, "leave me alone" (Pengu goes quiet, no questions), "I miss you", "sorry Pengu", and a
  quiet-presence family for the darkest things a person can say (Pengu stays and says it cares, nothing more).
  63 families, 399 chunks: the chip holds 400, so every one is spent.
- Reviewed line by line (twice: me, then a fresh reviewer) and fixed what we found: "no way" was swallowing "no way out",
  "I hate this" caught "I hate myself", "too much" caught "not too much sugar", "I won" caught "I wonder", a few replies
  that leaned towards advice ("one step at a time") were rewritten so Pengu only ever sits with you, and a few faces that
  argued with their words (a resigned penguin saying "I am not going anywhere") were swapped.
- Hold the gear button for a second and Pengu shows the one-line reminder of how to use it, for 20 seconds.

**Did it work:** compiles (2.9 MB of the 4 MB app space), every family rendered on the PC and checked by eye
(`artwork/previews/v10_every_family.png`), no duplicate chunks, every reply fits three rows. Waiting for the next video.

**So what:** a companion does not need to understand every word to be good company. It needs to catch the feeling, have
a face for it, and never make you feel unheard. The chip's limit of 400 phrases turned out to be a good editor: every
chunk had to earn its place.

---

## Step 13 – test video 3: it works, but it feels empty (20 Oct 2026)

**What we wanted:** to see the complete v10 in a real one minute chat, with the phone transcript next to it.

**What we did:** flashed v10 and talked to it: "Hey Pengu, how are you? It was good", "I spent time having fun",
"Hey Pengu", "I am sad", "can you please help me?".

**Did it work:** yes and no. It never crashed, never complained, and the faces are lovely. But "it was good" was said
before Pengu had asked, so it was lost; "having fun" and "I am sad" are not chunks it knows; "help me" got nothing; and
six different faces went by in a minute while only two of them meant anything. The one moment that matters, "I talked
and it heard me", never clearly happened.

**So what:** more faces and more chunks did not make it a better companion. The shape of the conversation did not
change, and that is the thing to fix. Rule for v11: the face only changes when the feeling changes, every conversation
ends with Pengu showing it heard you, the gear button starts a chat, and the black refresh only happens at sleep time.
The plan is in `docs/HANDOVER.md`.

---

## Step 14 – v11: the face only changes when the feeling changes (20 Oct 2026)

**What we wanted:** fix the three things test video 3 showed, and nothing else: moody face swaps, an empty feeling at the
end of a chat, and the black flash. Plus: a gear press should start a conversation.

**What we did:**
- The face now only changes when Pengu understands a feeling. "Hey Pengu", listening, and "MM HMM. GO ON" all keep the
  face that is already on screen and change only the words and the "(((" ear mark. Each family picks one drawing per
  conversation instead of a new one every reply.
- The ear mark pulses (one, two, three arcs) while you are talking, so you can see it is hearing you.
- Every conversation where you spoke ends with the "I heard you" moment: same face, a little heart, "I HEAR YOU. I AM
  HERE". Never a shrug, never silence. The chat then closes and the face stays for two minutes.
- Gear button: a short press starts a new conversation (same as "Hey Pengu", works mid chat too). Holding it for a
  second steps through demo moods. Debounced.
- The black full refresh now only happens at boot, on "good night", when Pengu falls asleep after ten quiet minutes, and
  about once an hour while asleep. Never when a chat ends.
- Plain phrases added: "I am sad", "I am happy", "I am tired", "I am angry", "having fun", and a "help me" family
  ("I AM HERE. TELL ME"). Eight of the least likely chunks dropped to stay under the chip's 400. 64 families, 397 chunks.
- Only "good night" works without the name or an open chat now; "stop now" said to someone else no longer changes the face.
- Reviewed by a fresh reviewer; ten findings fixed (faces pre-set to the happy drawing at boot, pulse redraws after a
  reply, no debounce, quiet after "mm hmm" went to rest instead of "I heard you", wake word counted as talking).

**Did it work:** compiles, the whole conversation rendered on the PC as screens (`artwork/previews/v11_one_conversation.png`):
same happy face through listening and "mm hmm", heart at the end, sad face only when "I am sad" is understood. Waiting
for the next video.

**So what:** a calm face is a listening face. Pengu should move less, not more, when you are talking.

---

## Step 15 – v12: Pengu asks back (20 Oct 2026)

**What we wanted:** test video 4 (v11) showed the loop is calm now: the face stayed put, the gear button started a chat,
"how are you? bad, very bad" worked. But it still felt like talking to a wall: Pengu replied and the chat just fizzled
out. "Thank you" after the chat had closed was ignored, "goodbye" did nothing, and "it was good" said in the same breath
as "how are you" was lost again. M asked for a real back-and-forth: simple, cute, alive, a few words that mean something.
And a power button that restarts it.

**What we did:**
- Most feelings now end with a little question Pengu can actually hear the answer to. Sad things: "WANT A HUG?". Tired
  or sick: "REST WITH ME?". Happy things: "SHALL WE CELEBRATE?". Angry, nervous, scared, confused: "TELL ME MORE?".
  "How are you" still asks "GOOD OR BAD?", and "bad" now leads on to "SORRY IT WAS BAD. HUG?".
- "Yes" gives a real answer: BIG HUG. SQUEEZE (heart-eyes face, hearts), OKAY. RESTING TOGETHER (same face, Zs),
  WOO! DANCE TIME (bouncing face with music notes), GO ON. I AM LISTENING (a fresh listening window). "No" is always
  fine: OKAY. I AM HERE ANYWAY. Yes and no only count right after a question, so the TV cannot answer for you.
- The "I heard you" close now matches the chat: a sad chat ends "THANK YOU FOR TELLING ME. HUG", a happy one
  "LOVE HEARING THAT".
- "Thank you" and "bye bye" work while the face is still up after a chat, not only inside it.
- Power button: hold it a moment and Pengu restarts completely. It learns the button's resting level in the first second
  after boot, so it works whichever way the board wires it.
- A fresh reviewer checked it; fixed: a hug would have made a sad chat close with "LOVE HEARING THAT" (the close now
  follows the feeling, not the animation), silence after "tell me more? yes" went to the idle face instead of the heart,
  the power pin could have been read while still held, a bare "please" and a bare "goodbye" were too easy to trigger.
- Back to the 24 faces. M watched v11 and said the extra expressions were "different emotions for the sake of it":
  confusing, and some drawings were cut off at the edge of the screen. So the 75 faces from the big sheet moved to
  `artwork/sketches/extra-75/` (kept, not compiled), and every family now has one clear face from the original 24,
  two at most. A hug shows the heart-eyes face, a party the excited one. The program shrank by 280 KB.
- 66 families, 399 chunks. Still offline, still under the chip's 400.

**Did it work:** compiles; every family and every question, yes and no rendered on the PC
(`artwork/previews/v12_every_family.png`, `v12_questions_and_answers.png`). Waiting for the next video.

**So what:** a question is only kind if you can hear the answer. Four tiny questions with yes/no answers Pengu can
really understand do more for the feeling of company than seventy-five more faces. Cute means clear.

---

## Step 16 – the penguin is called Pengu (20 Oct 2026)

**What we wanted:** a new name. "Hey Pengu" is easier to say and sounds like what it is.

**What we did:** renamed everything: the wake word ("hey pengu", "hi pengu", "okay pengu"), the replies ("I AM PENGU, YOUR PENGUIN"),
the idle line (SAY HEY PENGU), the program folder (`firmware/Pengu/Pengu.ino`), the release file (`Pengu_v12_full_8MB.bin`) and
every document. Older release files keep their old names so the history still makes sense. The GitHub repo is still called
`gawa`; rename it on GitHub (Settings, Repository name) if you want, GitHub redirects the old link.

**Did it work:** compiles, same 66 families and 399 chunks. The chip has to learn a new wake word, so the first test is simply
"Hey Pengu".

**So what:** a name is part of the product. Say it out loud before you pick it.

---

## How to add the next entry

Copy the shape above: **What we wanted**, **What we did**, **Did it work**, **So what**. Say what broke too; that is the useful part.
