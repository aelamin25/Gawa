/*
 * Pengu v12 - a pencil-sketch penguin you can have a conversation with, fully offline (Waveshare ESP32-S3-ePaper-1.54, V2)
 *
 * How it understands: it listens for short chunks that carry the feeling ("was bad", "so tired", "cat died")
 * inside whatever you say. One-word answers ("bad", "fine") only count right after Pengu asks "good or bad?".
 * When it hears you but catches nothing, it stays present ("MM HMM. GO ON") instead of complaining.
 * 66 feeling families, 399 chunks, 99 hand-drawn faces: each family picks one of its own faces per conversation.
 *
 * It talks back (v12): most feelings end with a little question Pengu can actually hear the answer to:
 *   "I am sad"      -> "OH NO. WANT A HUG?"        yes -> "BIG HUG. SQUEEZE" (hearts)   no -> "OKAY. I AM HERE ANYWAY"
 *   "so tired"      -> "LONG ONE. REST WITH ME?"   yes -> "OKAY. RESTING TOGETHER" (Zs)
 *   "got the job"   -> "LOOK AT YOU GO! PARTY?"    yes -> "WOO! DANCE TIME" (bounce, notes)
 *   "so angry"      -> "GRR. NOT FAIR. TELL ME MORE?"  yes -> "GO ON. I AM LISTENING" (a fresh listening window)
 *   "how are you"   -> "GOOD OR BAD?"  bad -> "SORRY IT WAS BAD. HUG?"  yes -> "BIG HUG. SQUEEZE"
 * "yes" and "no" only count right after a question, so the TV cannot answer for you.
 * The power button restarts Pengu completely.
 *
 * The loop (v11): the face only changes when a feeling is understood. Listening and misses keep the face and change
 * only the text and the "(((" mark. Every conversation ends with Pengu showing it heard you: the feeling face if it
 * caught one, otherwise the same face with "I HEARD YOU" and a little heart. Never a shrug, never silence.
 *
 * Conversation works like a smart speaker: say "Hey Pengu" once to start. After each reply Pengu keeps
 * listening for about 10 seconds, so you can just keep talking. The conversation ends after 10 seconds
 * of silence, or when you say "thank you", "stop", "that is all" or "good night". Then it is "Hey Pengu" again.
 * A sentence with its name in it ("hey pengu how are you", "thank you pengu") starts a conversation by itself.
 *
 *   "my dog died" / "my gran is sick"   -> sad face, "SO SORRY ABOUT YOUR DOG"
 *   "I passed my driving test"          -> proud face, "YOU PASSED YOUR DRIVING TEST!"
 *   "how are you"                       -> "GREAT! HOW WAS YOUR DAY? GOOD OR BAD?"  then "pretty good" / "not great" work
 *   mumble something it does not know   -> same face, "MM HMM. GO ON", then "I HEARD YOU" with a heart
 *   say nothing after "Hey Pengu"        -> "I AM HERE WHEN YOU NEED ME" and it rests
 *
 * Everything runs on the chip. Phrase patterns and replies live in phrases.h, faces in faces.h, drawings in sketches.h.
 * Gear (BOOT) button: short press starts a new conversation (same as "Hey Pengu"); hold it for a second to step through demo moods.
 * Power button: full restart.
 * The flashing full-screen refresh only happens at boot and when Pengu falls asleep.
 */

#include <Arduino.h>
#include "ESP_SR.h"
#include "esp32-hal-sr.h"
#include "user_config.h"
#include "audio_bsp.h"
#include "faces.h"
#include "phrases.h"
#include "src/display/epaper_driver_bsp.h"
#include "src/power/board_power_bsp.h"

// ---------------------------------------------------------------- settings you may want to change
#define LISTEN_WINDOW_MS    10000    // how long Pengu waits for you to say something after "Hey Pengu"
#define FOLLOWUP_MS         10000    // after a reply: how long you can keep talking without "Hey Pengu" (like Google's 10 s)
#define UNHEARD_WAIT_MS     2500     // you spoke, nothing matched: how long to wait before "mm hmm, go on"
#define MAX_RETRIES         1        // how many "mm hmm, go on" before the "I heard you" moment
#define MOOD_HOLD_MS        120000   // a reaction stays on screen this long, then Pengu rests (2 minutes)
#define FULL_REFRESH_EVERY  10       // once 10 quick updates have happened, the next sleep-time redraw is a full (flashing) clean refresh
#define FRAME_MS            320      // time between animation frames
#define BLINK_EVERY_MS      25000    // how often Pengu blinks while resting
#define SLEEP_AFTER_MS      600000   // falls asleep after 10 quiet minutes
#define PULSE_MS            500      // the "(((" ear mark pulses while Pengu listens

// ---------------------------------------------------------------- display
static epaper_driver_display *epd = nullptr;
static board_power_bsp_t board_power(EPD_PWR_PIN, Audio_PWR_PIN, VBAT_PWR_PIN);
static int partial_refreshes = 0;
static bool allow_full_refresh = false;   // the flashing clean refresh only happens at boot and when Pengu falls asleep

static void put_black_pixel(int x, int y) { epd->EPD_DrawColorPixel(x, y, DRIVER_COLOR_BLACK); }

// sketch = which drawing (SK_...), or -1 for the mood's usual one
static void show(mood_t mood, int sketch, int variant, int frame, const char *line)
{
  epd->EPD_Clear();
  draw_face(mood, sketch, variant, frame, line, put_black_pixel);
  if (allow_full_refresh && partial_refreshes >= FULL_REFRESH_EVERY) {
    epd->EPD_Init(); epd->EPD_Display(); epd->EPD_DisplayPartBaseImage(); epd->EPD_Init_Partial();
    partial_refreshes = 0;
  } else {
    epd->EPD_DisplayPart();
    partial_refreshes++;
  }
}

// play the mood's little animation (4 frames), ending on frame 0
static void play(mood_t mood, int sketch, int variant, const char *line)
{
  for (int f = 1; f < ANIM_FRAMES; f++) { show(mood, sketch, variant, f, line); delay(FRAME_MS); }
  show(mood, sketch, variant, 0, line);
}

static void display_init()
{
  custom_lcd_spi_t cfg = {};
  cfg.cs = EPD_CS_PIN;   cfg.dc = EPD_DC_PIN;   cfg.rst = EPD_RST_PIN;  cfg.busy = EPD_BUSY_PIN;
  cfg.mosi = EPD_MOSI_PIN; cfg.scl = EPD_SCK_PIN; cfg.spi_host = EPD_SPI_NUM;
  cfg.buffer_len = EPD_WIDTH * EPD_HEIGHT / 8;
  epd = new epaper_driver_display(EPD_WIDTH, EPD_HEIGHT, cfg);
  epd->EPD_Init(); epd->EPD_Clear(); epd->EPD_DisplayPartBaseImage(); epd->EPD_Init_Partial();
}

// ---------------------------------------------------------------- build the sentence list from the families
static sr_cmd_t *all_phrases = nullptr;
static size_t    phrase_count = 0;

static void build_phrases()
{
  size_t total = 0;
  for (size_t f = 0; f < FAMILY_COUNT; f++) total += FAMILIES[f].ntopics * FAMILIES[f].nendings;
  all_phrases = (sr_cmd_t *)heap_caps_calloc(total, sizeof(sr_cmd_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!all_phrases) { Serial.println("Out of memory for phrases"); while (true) delay(1000); }
  if (total > 400) Serial.printf("WARNING: %u sentences, the chip only holds 400\n", (unsigned)total);
  phrase_count = 0;
  for (size_t f = 0; f < FAMILY_COUNT; f++) {
    const Family &fam = FAMILIES[f];
    for (int t = 0; t < fam.ntopics; t++)
      for (int e = 0; e < fam.nendings; e++) {
        sr_cmd_t &c = all_phrases[phrase_count++];
        c.command_id = CMD_ID(f, t);
        snprintf(c.str, sizeof(c.str), "%s%s%s", fam.prefix, fam.topics[t], fam.endings[e]);
      }
  }
  Serial.printf("Pengu knows %u sentences from %u patterns\n", (unsigned)phrase_count, (unsigned)FAMILY_COUNT);
}

// ---------------------------------------------------------------- pet state
enum State { RESTING, LISTENING, REACTED };
static State    state = RESTING;
static uint32_t state_until = 0;
static int      retries = 0;
static uint32_t rest_since = 0, last_blink = 0, last_pulse = 0, last_sleep_frame = 0;
static int      sleep_frame = 0;
static bool     asleep = false;
static bool     in_chat = false;        // a conversation is open: no "Hey Pengu" needed for the next thing you say
static bool     follow_up = false;      // listening for a follow-up after a reply (the reply face stays on screen)
static int      awaiting = Q_NONE;      // the little question Pengu just asked (Q_DAY, Q_HUG ...): short answers count only now
static const char *rest_line = "", *listen_line = "";
static int      last_variant = -1, last_line = -1, last_face = -1;
static int      pwr_idle = HIGH; static uint32_t pwr_since = 0;   // power button: held for a moment = full restart
// the face on screen right now: it only changes when a feeling is understood (or at rest / sleep)
static mood_t   cur_mood = MOOD_IDLE; static int cur_sketch = -1, cur_var = 0; static char cur_text[64] = "";
enum Tone { TONE_NEUTRAL, TONE_SAD, TONE_GOOD };
static Tone     cur_tone = TONE_NEUTRAL;   // how the conversation feels (set by the feeling, not by a hug or a dance)
static bool     said_something = false;    // Pengu has replied to something in this conversation
static int16_t  chat_face[FAMILY_COUNT]; // which drawing each family uses in this conversation (-1 = not chosen yet)
static void new_faces() { for (int i = 0; i < (int)FAMILY_COUNT; i++) chat_face[i] = -1; }
static volatile int voice_cmd = -1, voice_phrase = -1;     // set by the speech callback

// "did they talk?" detector, fed by the microphone callback
static volatile bool     vad_on = false;
static volatile bool     vad_heard_voice = false;
static volatile uint32_t vad_last_voice_ms = 0;
static volatile float    noise_floor = 200;

static int pick_different(int count, int *last)
{
  int i;
  do { i = random(count); } while (count > 1 && i == *last);
  *last = i;
  return i;
}

static void set_face(mood_t mood, int sketch, int var, const char *text)
{
  cur_mood = mood; cur_sketch = sketch; cur_var = var; strlcpy(cur_text, text ? text : "", sizeof(cur_text));
}

static void go_rest(const char *line)
{
  g_chat_dot = false; g_heart = false;
  state = RESTING; vad_on = false; retries = 0; asleep = false; in_chat = false; follow_up = false; awaiting = Q_NONE;
  rest_since = last_blink = millis(); rest_line = line;
  set_face(MOOD_IDLE, -1, 0, line); cur_tone = TONE_NEUTRAL;
  show(MOOD_IDLE, -1, 0, 0, line);
}

// the "I heard you" moment: same face, a heart, chat closed, face stays for a while
static void heard_you()
{
  const char *line;                   // the close matches how the conversation felt
  if (cur_tone == TONE_SAD)       line = R_HEARD_SAD[pick_different(N(R_HEARD_SAD), &last_line)];
  else if (cur_tone == TONE_GOOD) line = R_HEARD_GOOD[pick_different(N(R_HEARD_GOOD), &last_line)];
  else                            line = R_HEARD_END[pick_different(N(R_HEARD_END), &last_line)];
  Serial.printf("Pengu: %s\n", line);
  in_chat = false; follow_up = false; vad_on = false; retries = 0; awaiting = Q_NONE;
  g_chat_dot = false; g_heart = true;
  state = REACTED; state_until = millis() + MOOD_HOLD_MS;
  strlcpy(cur_text, line, sizeof(cur_text));
  show(cur_mood, cur_sketch, cur_var, 0, cur_text);
}

static void go_listen(const char *line)
{
  if (!in_chat) { new_faces(); said_something = false; } // a new conversation: faces may be picked afresh
  if (asleep || state == RESTING) set_face(MOOD_IDLE, -1, 0, line);   // waking up from rest: the calm face
  g_heart = false; g_chat_dot = true; g_chat_pulse = 2;
  state = LISTENING; asleep = false; in_chat = true; follow_up = false; retries = 0; awaiting = Q_NONE;
  vad_on = false; listen_line = line;
  strlcpy(cur_text, line, sizeof(cur_text));
  show(cur_mood, cur_sketch, cur_var, 0, cur_text);     // same face, ear mark on, "I AM LISTENING"
  delay(300);                                   // let the tail of "hey pengu" pass before we count talking
  vad_heard_voice = false; vad_on = true;
  state_until = millis() + LISTEN_WINDOW_MS;
  last_pulse = millis();
}

static void react(int command_id)
{
  int f = CMD_FAMILY(command_id), t = CMD_TOPIC(command_id);
  if (f < 0 || f >= (int)FAMILY_COUNT) return;
  const Family &fam = FAMILIES[f];
  if (t < 0 || t >= fam.ntopics) return;
  const char *tmpl = fam.replies[pick_different(fam.nreplies, &last_line)];
  char line[64];
  snprintf(line, sizeof(line), tmpl, fam.topics[t]);      // %s -> the word it heard (if the reply has one)
  Serial.printf("Pengu: %s\n", line);
  g_chat_dot = !fam.ends_chat;        // show the little listening mark while the chat stays open
  g_heart = false;
  vad_on = false;                     // while the animation plays, nothing counts as talking
  if (!in_chat) new_faces();                             // reached straight from rest ("thanks pengu", "good night"): fresh picks
  if (chat_face[f] < 0 && fam.nfaces > 0) chat_face[f] = fam.faces[pick_different(fam.nfaces, &last_face)];
  int sketch = chat_face[f];                             // one drawing per family per conversation
  g_chat_pulse = 2;
  set_face(fam.mood, sketch, pick_different(FACE_VARIANTS, &last_variant), line);
  said_something = true;
  switch (fam.mood) {
    case MOOD_SAD: case MOOD_SCARED: case MOOD_NERVOUS: case MOOD_TIRED: case MOOD_GRUMPY: case MOOD_ANGRY: case MOOD_DISGUSTED: cur_tone = TONE_SAD; break;
    case MOOD_HAPPY: case MOOD_PROUD: case MOOD_CONFIDENT: case MOOD_MISCHIEVOUS: case MOOD_SHY: cur_tone = TONE_GOOD; break;
    default: cur_tone = TONE_NEUTRAL; break;
  }
  if (fam.ask == Q_HUG || fam.ask == Q_REST) cur_tone = TONE_SAD;
  play(cur_mood, cur_sketch, cur_var, cur_text);
  last_pulse = millis();
  retries = 0;
  awaiting = fam.ask;                 // only after a question do short answers count
  if (fam.ends_chat) {                // "thank you", "stop", "good night": the conversation is over
    in_chat = false; follow_up = false; vad_on = false;
    if (fam.mood == MOOD_SLEEP) {     // "good night": straight to sleep, with one clean (flashing) refresh
      state = RESTING; asleep = true; rest_since = last_sleep_frame = millis(); sleep_frame = 0; rest_line = "ZZZ";
      allow_full_refresh = true; partial_refreshes = FULL_REFRESH_EVERY;
      show(cur_mood, cur_sketch, cur_var, 0, cur_text);
      allow_full_refresh = false;
    } else {
      state = REACTED; state_until = millis() + MOOD_HOLD_MS;
    }
  } else {                            // keep the chat open: listen for a follow-up, reply face stays on screen
    in_chat = true; follow_up = true;
    state = LISTENING;
    state_until = millis() + FOLLOWUP_MS;
    vad_heard_voice = false; vad_on = true;
  }
}

// you answered Pengu's little question ("want a hug?") with yes or no
static void answer(int kind, bool yes)
{
  if (kind <= Q_DAY || kind >= N(ANSWERS)) return;
  const Answer &a = ANSWERS[kind];
  const char *line = yes ? a.yes : a.no;
  Serial.printf("Pengu: %s\n", line);
  awaiting = Q_NONE; retries = 0; g_heart = false; vad_on = false; g_chat_pulse = 2; said_something = true;
  strlcpy(cur_text, line, sizeof(cur_text));
  g_chat_dot = true;
  if (yes && kind == Q_TELL) {        // "tell me more?" yes: a fresh listening window, same face
    show(cur_mood, cur_sketch, cur_var, 0, cur_text);
    in_chat = true; follow_up = false; state = LISTENING;
    state_until = millis() + LISTEN_WINDOW_MS; vad_heard_voice = false; vad_on = true; last_pulse = millis();
    return;
  }
  if (yes) { cur_mood = a.yes_mood; if (a.yes_face >= 0) cur_sketch = a.yes_face; }   // hearts, Zs or a little dance; a hug or a party changes the face
  if (yes && kind == Q_PARTY) cur_var = 2;                                              // the dance: bounce with music notes
  play(cur_mood, cur_sketch, cur_var, cur_text);
  if (yes && kind == Q_REST) show(cur_mood, cur_sketch, cur_var, 3, cur_text);          // rest with the Zs showing
  last_pulse = millis();
  in_chat = true; follow_up = true; state = LISTENING;
  state_until = millis() + FOLLOWUP_MS; vad_heard_voice = false; vad_on = true;
}

// ---------------------------------------------------------------- microphone -> ESP-SR (+ talk detector)
static esp_err_t mic_fill(void *arg, void *out, size_t len, size_t *bytes_read, uint32_t timeout_ms)
{
  static int16_t *stereo = nullptr;
  static size_t stereo_cap = 0;
  if (stereo_cap < len * 2) {
    free(stereo);
    stereo = (int16_t *)heap_caps_malloc(len * 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    stereo_cap = len * 2;
  }
  if (audio_mic_read(stereo, len * 2) != 0) { *bytes_read = 0; return ESP_FAIL; }

  size_t frames = len / 2;
  int16_t *mono = (int16_t *)out;
  int64_t energy_l = 0, energy_r = 0;
  for (size_t i = 0; i < frames; i++) { energy_l += abs(stereo[2 * i]); energy_r += abs(stereo[2 * i + 1]); }
  int pick = (energy_r > energy_l) ? 1 : 0;
  for (size_t i = 0; i < frames; i++) mono[i] = stereo[2 * i + pick];
  *bytes_read = len;

  float level = (float)((pick ? energy_r : energy_l) / (int64_t)frames);
  if (!vad_on) {
    noise_floor = noise_floor * 0.98f + level * 0.02f;          // learn how loud the room is
  } else {
    noise_floor = noise_floor * 0.998f + level * 0.002f;        // while listening, learn very slowly (a fan or TV must not jam us)
    float threshold = noise_floor * 3.0f; if (threshold < 350) threshold = 350;
    if (level > threshold) { vad_last_voice_ms = millis(); vad_heard_voice = true; }
  }
  return ESP_OK;
}

static void on_voice_event(void *arg, sr_event_t event, int command_id, int phrase_id)
{
  if (event == SR_EVENT_COMMAND) { voice_phrase = phrase_id; voice_cmd = command_id; }   // cmd last: loop() reads it first
  sr_set_mode(SR_MODE_COMMAND);       // always keep listening
}

// ---------------------------------------------------------------- Arduino
void setup()
{
  Serial.begin(115200);
  delay(300);
  Serial.println("\nPengu v12 starting...");
  randomSeed(esp_random());
  new_faces();
  pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(PWR_BUTTON_PIN, INPUT_PULLUP);
  pwr_idle = -1;                              // learned in loop(): the level the button holds for a full second

  board_power.POWEER_EPD_ON();
  board_power.POWEER_Audio_ON();
  delay(50);
  display_init();
  show(MOOD_IDLE, -1, 0, 0, "WAKING UP...");

  build_phrases();
  audio_bsp_init();
  esp_err_t err = sr_start(mic_fill, nullptr, SR_CHANNELS_MONO, SR_MODE_COMMAND, "M",
                           all_phrases, phrase_count, on_voice_event, nullptr);
  if (err != ESP_OK) {
    Serial.printf("Speech recognition failed to start (%d). Is the speech model flashed?\n", err);
    show(MOOD_SAD, -1, 0, 0, "NO VOICE MODEL");
    while (true) delay(1000);
  }
  go_rest(LINE_IDLE);
  Serial.println("Say: Hey Pengu");
}

void loop()
{
  uint32_t now = millis();

  // ---- power button: a full restart, like unplugging it
  // (polarity is learned: whatever level the pin holds steadily for a second after boot counts as "not pressed")
  if (pwr_idle < 0) {
    static int seen = -1; static uint32_t seen_since = 0;
    int lvl = digitalRead(PWR_BUTTON_PIN);
    if (lvl != seen) { seen = lvl; seen_since = now; }
    else if (now - seen_since > 1000) pwr_idle = lvl;
  } else if (digitalRead(PWR_BUTTON_PIN) != pwr_idle) {
    if (!pwr_since) pwr_since = now;
    if (now - pwr_since > 150) {
      Serial.println("Power button -> restart");
      g_chat_dot = false; g_heart = false;
      show(MOOD_IDLE, -1, 0, 0, "RESTARTING...");
      delay(300);
      ESP.restart();
    }
  } else pwr_since = 0;

  // ---- gear button: short press = a new conversation (same as "Hey Pengu"); hold 1 s = step through demo moods
  static bool was_pressed = false; static uint32_t pressed_since = 0; static bool long_done = false;
  bool pressed = digitalRead(BOOT_BUTTON_PIN) == LOW;
  static uint32_t last_edge = 0;
  if (pressed != was_pressed && now - last_edge < 50) pressed = was_pressed;   // debounce: ignore edges within 50 ms
  if (pressed != was_pressed) last_edge = now;
  if (pressed && !was_pressed) { pressed_since = now; long_done = false; }
  if (pressed && !long_done && now - pressed_since > 1000) {
    long_done = true;
    static int step = 0;
    const char **demo[] = { T_HOWRU, T_PROUD, T_LOVED, T_ANGRY, T_SCARED, T_HOPEFUL, T_JEALOUS, T_GRUMPY, T_NERVOUS, T_SHY,
                            T_TIRED, T_SHOCKED, T_BORED, T_HUNGRY, T_CARING, T_NOSTALGIC };
    int fam = family_of(demo[step]);
    if (fam >= 0) react(CMD_ID(fam, 0));
    step = (step + 1) % N(demo);
  }
  if (!pressed && was_pressed && !long_done && now - pressed_since >= 40) {
    Serial.println("Gear pressed -> new conversation");
    in_chat = false;                                   // a fresh start, even mid chat
    go_listen(R_LISTEN[pick_different(N(R_LISTEN), &last_line)]);
  }
  was_pressed = pressed;

  // ---- a sentence was recognised
  if (voice_cmd >= 0) {
    int cmd = voice_cmd, ph = voice_phrase; voice_cmd = -1;
    Serial.printf("Heard: \"%s\"\n", (ph >= 0 && ph < (int)phrase_count) ? all_phrases[ph].str : "?");
    int fam = CMD_FAMILY(cmd);
    bool has_name = false;
    if (fam >= 0 && fam < (int)FAMILY_COUNT && CMD_TOPIC(cmd) < FAMILIES[fam].ntopics)
      has_name = strstr(FAMILIES[fam].prefix, "pengu") || strstr(FAMILIES[fam].topics[CMD_TOPIC(cmd)], "pengu");
    bool ok_fam = (fam >= 0 && fam < (int)FAMILY_COUNT);
    bool answer_only = ok_fam && FAMILIES[fam].answer_only;
    bool ends = ok_fam && FAMILIES[fam].ends_chat;
    if (fam == FAMILY_WAKE) {
      if (state != LISTENING || follow_up) go_listen(R_LISTEN[pick_different(N(R_LISTEN), &last_line)]);
      else { Serial.println("  (already listening)"); vad_heard_voice = false; }
    } else if (answer_only) {
      const char **t = FAMILIES[fam].topics;
      if (t == T_ANS_YES || t == T_ANS_NO) {
        if (in_chat && awaiting >= Q_HUG) answer(awaiting, t == T_ANS_YES);
        else Serial.println("  (yes/no, but Pengu did not ask anything - ignored)");
      } else if (in_chat && awaiting == Q_DAY) {
        react(cmd);
      } else if (in_chat) {             // "today was terrible" with no question pending: treat it as the feeling itself
        int sib = family_of(t == T_ANS_GOOD ? T_HAPPY : t == T_ANS_BAD ? T_SAD : T_PEACEFUL);
        if (sib >= 0) react(CMD_ID(sib, 0));
      }
      else Serial.println("  (short answer, but no conversation is open - ignored)");
    } else if (in_chat || has_name || (ends && (state == REACTED || FAMILIES[fam].mood == MOOD_SLEEP))) {   // "thank you" right after a chat, "good night" any time
      react(cmd);
    } else {
      Serial.println("  (ignored - say Hey Pengu first)");
    }
  }

  // ---- while listening: did they talk but nothing matched? did they say nothing at all?
  if (state == LISTENING) {
    bool window_over = (int32_t)(now - state_until) >= 0;
    if (vad_heard_voice && (now - vad_last_voice_ms > UNHEARD_WAIT_MS || window_over)) {
      if (retries < MAX_RETRIES && !follow_up) {
        retries++;
        Serial.println("Talked, but no match -> mm hmm, go on (same face)");
        listen_line = R_HEARD_YOU[pick_different(N(R_HEARD_YOU), &last_line)];
        strlcpy(cur_text, listen_line, sizeof(cur_text));
        vad_on = false; g_chat_pulse = 2;
        show(cur_mood, cur_sketch, cur_var, 0, cur_text);   // the face does not change: only the words
        vad_heard_voice = false; vad_on = true;
        state_until = millis() + LISTEN_WINDOW_MS; last_pulse = millis();
      } else {
        Serial.println("No match -> I heard you (heart), chat closed");
        heard_you();
      }
    } else if (!vad_heard_voice && window_over) {
      if (follow_up) {                      // 10 s of quiet after a reply: the conversation ends, face stays up
        Serial.println("Quiet -> conversation over");
        in_chat = false; follow_up = false; vad_on = false; awaiting = Q_NONE;
        state = REACTED; state_until = now + MOOD_HOLD_MS;
        g_chat_dot = false; show(cur_mood, cur_sketch, cur_var, 0, cur_text);   // the reply face again, listening mark gone
      } else if (retries > 0 || said_something) {          // they talked earlier in this chat, then went quiet: still heard
        Serial.println("Quiet after talking -> I heard you");
        heard_you();
      } else {
        Serial.println("Nothing said -> resting");
        go_rest(R_NOTHING[pick_different(N(R_NOTHING), &last_line)]);
      }
    }
  }

  // ---- while listening: the "(((" mark pulses when you are talking (so you can see it hears you)
  if (state == LISTENING && millis() - last_pulse > PULSE_MS) {
    last_pulse = millis();
    bool talking = vad_on && (millis() - vad_last_voice_ms < 600);
    int want = talking ? (g_chat_pulse % 3) + 1 : 2;
    if (want != g_chat_pulse) { g_chat_pulse = want; show(cur_mood, cur_sketch, cur_var, 0, cur_text); }
  }

  // ---- a reaction has been up long enough: rest
  if (state == REACTED && (int32_t)(now - state_until) >= 0) go_rest(LINE_IDLE);

  // ---- resting: blink sometimes, fall asleep after a long quiet spell
  if (state == RESTING) {
    if (!asleep && now - rest_since > SLEEP_AFTER_MS) {
      asleep = true; sleep_frame = 0; last_sleep_frame = now;
      allow_full_refresh = true;                          // falling asleep is the only time a clean (flashing) refresh is allowed
      set_face(MOOD_SLEEP, -1, 0, "ZZZ");
      show(MOOD_SLEEP, -1, 0, 0, "ZZZ");
      allow_full_refresh = false;
    } else if (asleep && now - last_sleep_frame > 60000) {
      last_sleep_frame = now; sleep_frame = (sleep_frame + 1) % ANIM_FRAMES;
      allow_full_refresh = partial_refreshes >= FULL_REFRESH_EVERY * 6;   // about once an hour while asleep: nobody is watching
      show(MOOD_SLEEP, -1, 0, sleep_frame, "ZZZ");
      allow_full_refresh = false;
    } else if (!asleep && now - last_blink > BLINK_EVERY_MS) {
      last_blink = now;
      show(MOOD_IDLE, -1, 0, 2, rest_line); delay(250);      // a tiny breath
      show(MOOD_IDLE, -1, 0, 0, rest_line);
    }
  }

  delay(20);
}
