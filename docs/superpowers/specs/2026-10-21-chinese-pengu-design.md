# Chinese Pengu: design (v14a + v14)

Date: 21 Oct 2026. Status: agreed in chat, waiting for M's review. Replaces the v13 feelings Pengu (kept in git and in `releases/`).

## The one sentence

Pengu is a small penguin from Harbin who lives in your house and can only be looked after in Chinese. Each time you
help it, you learn one new word you can use straight away.

## Decisions made in the brainstorm

| Question | Decision |
|---|---|
| Where does the brain live? | On the board + the cloud. Wi-Fi is on, so the old "offline only" rule ends. |
| Which cloud? | OpenAI only, with one key. Live voice (realtime), so it feels like talking, not waiting. |
| Character | Pengu from Harbin: small, warm, never judges, has a little life of its own (snacks, naps, cold). |
| English | Chinese first, with a rescue (slower → English → move on). You can always answer in English and get shown the Chinese. |
| What you study | Pengu leads: a built-in path, home first, then outside. |
| The game | Pengu has needs (hungry, thirsty, tired, bored). You meet them by asking in Chinese. |
| Neglect | No punishment. It never dies, never gets sick, never loses progress. |
| Hints | Fade per word: full card → pinyin only → characters only. |
| Length | As long as you like: every round ends with 还要吗? (want more?). |
| v14 scope | The whole house: kitchen, bedroom, living room. |
| Voice | A cloud voice (Alpha does not speak Chinese), told to sound small, cheerful and slow. |
| Your Qwen3-4B | Not in v14. Parked as a later bonus. |
| How the board connects | Directly to OpenAI over a secure websocket. No relay server in v14. |

## The learning rules (what keeps it easy)

1. **You hear it before you say it.** Pengu always says a new phrase first; you copy it.
2. **Three sentence shapes at home:**
   - **…吧** (let's / go on…)
   - **我想…** (I want…)
   - **要 / 不要** (yes / no)
   Each new word is one swap into a shape you already know.
3. **One new word at a time.** Everything else Pengu says uses only words you know. The board sends your word list with
   every brief.
4. **Old words come back.** Rounds alternate new, review, new, review.
   - A new word is the next unseen word, in map order, in the newest open place.
   - A review picks the known word with the fewest clean tries. On a tie it picks the one seen longest ago, and it
     prefers the place that matches Pengu's mood.
   - With no unseen words left, every round is a review.
5. **A new place opens when the last one is easy.** Every word in it has been said without a hint at least 2 times.
   There are no tests.

## The map (levels 1 to 3 are v14; 4 to 6 come later)

Each line reads: what Pengu says → what you say → what Pengu does.

### Level 1: 厨房 chú fáng (kitchen)

| # | Word | Pengu says | You say | Pengu does |
|---|---|---|---|---|
| 1 | 饭 fàn (meal) | 我饿了！ wǒ è le (I'm hungry) | 吃饭吧 chī fàn ba | eats, 好吃！谢谢！ |
| 2 | 鱼 yú (fish) | 我想吃鱼！ | 吃鱼吧 | eats |
| 3 | 水 shuǐ (water) | 我渴了！ wǒ kě le (I'm thirsty) | 喝水吧 hē shuǐ ba | drinks |
| 4 | 冰箱 bīngxiāng (fridge) | 我想开冰箱 | 开冰箱吧 kāi bīngxiāng ba | opens the fridge |
| 5 | 面条 miàntiáo (noodles) | 我想吃面条 | 吃面条吧 | eats |
| 6 | 牛奶 niúnǎi (milk) | 我想喝牛奶 | 喝牛奶吧 | drinks |
| 7 | 饺子 jiǎozi (dumplings) | 我想吃饺子 | 吃饺子吧 | eats |
| 8 | 苹果 píngguǒ (apple) | 我想吃苹果 | 吃苹果吧 | eats |

Words used in every round: 还要吗？ hái yào ma (want more?), 要 yào, 不要 bú yào, 好吃 hǎo chī, 谢谢 xièxie, 拜拜 bāibāi.

### Level 2: 卧室 wòshì (bedroom)

| # | Word | Pengu says | You say | Pengu does |
|---|---|---|---|---|
| 1 | 睡觉 shuìjiào (sleep) | 我累了… wǒ lèi le (I'm tired) | 我们睡觉吧 | sleeps (Zs) |
| 2 | 起床 qǐchuáng (get up) | (asleep) 呼…呼… | 起床吧！ | wakes up |
| 3 | 早上好 zǎoshang hǎo (good morning) | (just woke up, waits) | 早上好！ | 早上好！ back |
| 4 | 晚安 wǎn'ān (good night) | 我想睡觉 | 晚安！ | sleeps |
| 5 | 洗澡 xǐzǎo (shower) | 我想洗澡 | 洗澡吧 | splashes |
| 6 | 刷牙 shuāyá (brush teeth) | 我想刷牙 | 刷牙吧 | brushes |

### Level 3: 客厅 kètīng (living room)

| # | Word | Pengu says | You say | Pengu does |
|---|---|---|---|---|
| 1 | 玩游戏 wán yóuxì (play games) | 我很无聊 wǒ hěn wúliáo (I'm bored) | 我们玩游戏吧 | plays, 好玩！ |
| 2 | 看电视 kàn diànshì (watch TV) | 我想看电视 | 看电视吧 | watches |
| 3 | 听音乐 tīng yīnyuè (music) | 我想听音乐 | 听音乐吧 | listens |
| 4 | 跳舞 tiàowǔ (dance) | 我想跳舞！ | 我们跳舞吧 | dances |
| 5 | 看书 kàn shū (read) | 我想看书 | 看书吧 | reads |
| 6 | 休息 xiūxi (rest) | 我有点儿累 | 休息吧 | rests |

### Later: levels 4 to 6

- **4 外面 outside:** 去, 散步, 公园, 冷, 热
- **5 商店 shop:** 买, 多少钱, 一 to 十, 块
- **6 饭馆 restaurant:** 点菜, 我要…, 好的

## What you see and do (v14)

1. **Pengu's mood follows the clock** (London time, from Wi-Fi):
   - hungry around 8:00, 12:30 and 18:30
   - tired after 22:00
   - bored in the afternoon
   - content otherwise
   It only shows; it never rings, beeps or gets worse.
2. **Start** with the gear or "Hey Pengu". The hint card for the first word appears straight away while the link
   connects, so there is no loading screen. The first word is picked by the teacher. At mealtimes it prefers a kitchen
   word, at night a bedroom word.
3. **A round:**
   - Pengu says its line (for example 我饿了！).
   - The card shows what you could say.
   - You say it.
   - Pengu does the action, says one short happy line, then asks 还要吗？
4. **要** → next round. **不要**, "bye" or 拜拜 → Pengu says 拜拜, the link closes, and the face rests.
5. **Rescue:**
   - silent 5 s or 什么? → Pengu says it again, slower
   - English → "用中文说: …" and the card shows it in full
   - after 3 tries → 没关系 and Pengu acts anyway; the word counts as "helped", not "clean"
6. **The screen**, top to bottom (200x200):
   - the place name (厨房)
   - Pengu's face (the existing pencil faces)
   - Pengu's line in characters (24 px, at most 8 per line, at most 2 lines)
   - pinyin with tone marks
   - small English
   - the hint card replaces Pengu's line while it is your turn
7. **Hint stage per word:**
   - 0 to 2 clean tries → characters + pinyin + English
   - 3 to 5 → pinyin only
   - 6 or more → characters only
   - a "helped" try moves the word back one stage

## How it works inside

| File | Job |
|---|---|
| `settings.cpp/h` | Reused from `extras/cloud-brain`. "Pengu-Setup" hotspot page: Wi-Fi, password, OpenAI key, model name, voice name. Opens on a long gear press or when no Wi-Fi is saved. |
| wake (in `Pengu.ino`) | ESP-SR stays for "Hey Pengu" (offline, instant) plus the gear. It pauses for the whole lesson. |
| `live.cpp/h` | Secure websocket to OpenAI's realtime voice. Streams mic audio up and plays Pengu's audio as it arrives. The server decides when you stopped talking. Passes function calls to the teacher. |
| `course.h` | The map above, as data (place, word, pinyin, English, Pengu's line, your line, action, face). New places are data only. |
| `lesson.cpp/h` | The teacher. Picks the word, writes the brief for the cloud, handles the function calls, applies the rescue counts. Pure C++, so it can be tested off the board. |
| `progress.cpp/h` | Saved in the board's flash (NVS): per word, clean and helped counts and the last round seen; open places; round counter. |
| `hanzi.h` + font partition | The 3,500 most common characters, 24 px, 1 bit (about 250 KB), in the partition that held the English voice (0x620000). A small Latin font with all tone-marked vowels (ā á ǎ à … ǖ ǘ ǚ ǜ ü). A missing character draws as a box. |
| clock | NTP over Wi-Fi, Europe/London, for the mood. |

**The brief sent to the cloud each round contains:**
- who Pengu is
- the place
- the target word and the line Pengu should say
- the words you know
- the rules: only known words plus the target, one short sentence, speak slowly, and the rescue steps
- "never judge, never lecture"

**The cloud's 4 functions:**
- `show(hanzi, pinyin, english)`: called before every line Pengu says, so the screen matches the voice
- `act(action)`: eat, drink, sleep, wake, play, dance and so on → the board animates
- `done(word, tries)`: you said it; the board saves progress and picks the next word
- `end_lesson()`: after 不要 or bye

**Cost guards:**
- the link closes after 20 s of quiet
- a lesson ends at 15 minutes (Pengu yawns, 拜拜)
- prepaid credit with auto-recharge off, so Pengu cannot spend more than what is loaded

**When things fail:**
- no Wi-Fi → NO WIFI
- the key is refused → BAD KEY
- other cloud errors → CLOUD?

In all three cases the hint card still shows, and Pengu goes back to rest. Nothing is lost: progress is only written
after `done`.

**Audio:** the realtime voice uses 16-bit, 24 kHz audio. During a lesson the I2S runs at 24 kHz (ESP-SR is paused), and it
goes back to 16 kHz afterwards. If the board cannot hold Wi-Fi + TLS + audio at once, v14a will show it first.

## Build order

1. **v14a pipe test.**
   - Setup page, Wi-Fi, the live link, the Chinese font.
   - Say anything in Chinese; Pengu answers out loud and shows the characters + pinyin.
   - We pick the voice here from a few samples.
   - *Pass:* 5 Chinese sentences answered, the characters readable, the reply starts within about 2 s, no reboot in 10 minutes.
2. **v14 the house:** course, teacher, progress and moods for levels 1 to 3.

## Testing

- `lesson.cpp` gets unit tests in the chat's workspace:
  - word picking (new/review alternation, shakiest first)
  - hint stages
  - unlock rule
  - rescue counts
  - progress survives a save/load
- Screen layouts are rendered as PNG previews for M before any flash.
- M records the Pass-test videos with a phone transcript.

**v14 Pass test:**
1. say 吃饭吧 → it eats within about 2 s
2. say 要 → a new word
3. that word comes back 2 rounds later
4. say "fish" in English → it teaches the Chinese
5. stay silent → it repeats slower
6. say 不要 → it ends nicely
7. switch it off and on → it remembers the next word
8. after kitchen words are clean twice → the bedroom opens

## Not in v14

Calls with Pengu (v15), memory of facts about you (v16), Pengu ringing you when hungry or lonely (v17), levels 4 to 6,
room drawings, a relay server, your Qwen model, offline Chinese.

## Getting an OpenAI API key (M)

1. Go to **platform.openai.com** and sign in. It is separate from a ChatGPT plan; the same login works.
2. **Settings → Billing:** add a card and buy a small credit (for example $5). Leave **auto-recharge off**. Then Pengu can
   never spend more than what you put in.
3. **API keys → Create new secret key.** Name it "Pengu". Copy it; it starts with `sk-` and is shown only once.
4. Keep it in your password manager or Notes. **Never** paste it into a chat, a file in the repo, or GitHub.
5. You will type it into Pengu's setup page when v14a is flashed.

## Changes during the v14a build (21 Oct 2026)

- **The screen:** Pengu stays full size. One strip underneath fades between the characters, the pinyin and the English,
  instead of showing all three at once. This was M's call.
- **The fonts:** characters 36 px and pinyin/English 26 px whenever they fit on one line (24/20 px otherwise). The font
  is GB2312 level 1, 3,755 characters, 1.05 MB.
- **Words:** start super basic (2 to 6 characters per reply, about 20 words), building up one step at a time.
- **v14a has no "Hey Pengu" yet.** The gear starts and ends a call. "Hey Pengu" comes back in v14.
