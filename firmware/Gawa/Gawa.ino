/*
 * Gawa v9 - a pencil-sketch penguin you can have a conversation with
 *
 * Conversation works like a smart speaker: say "Hey Gawa" once to start. After each reply Gawa keeps
 * listening for about 10 seconds, so you can just keep talking. The conversation ends after 10 seconds
 * of silence, or when you say "thank you", "stop", "that is all" or "good night". Then it is "Hey Gawa" again.
 * A sentence with its name in it ("hey gawa how are you", "thank you gawa") starts a conversation by itself., for the Waveshare ESP32-S3-ePaper-1.54 (V2)
 *
 *   "Hey Gawa"                      -> ears up, Gawa listens for about 10 seconds
 *   "my dog died" / "my gran is ill" -> sad face, "SO SORRY ABOUT YOUR DOG"
 *   "I passed my driving test"       -> happy face, "YOU PASSED YOUR DRIVING TEST!"
 *   "I am thinking about work"       -> thoughtful face, "HMM, WORK. GO ON"
 *   mumble something it does not know -> puzzled face, "SORRY, SAY THAT AGAIN?" and it listens again
 *   say nothing                      -> "I AM HERE WHEN YOU NEED ME" and it rests
 *
 * Everything runs on the chip. Phrase patterns and replies live in phrases.h, faces in faces.h.
 * Gear (BOOT) button: short press steps through the moods by hand.
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
#define LISTEN_WINDOW_MS    10000    // how long Gawa waits for you to say something after "Hey Gawa"
#define FOLLOWUP_MS         10000    // after a reply: how long you can keep talking without "Hey Gawa" (like Google's 10 s)
#define UNHEARD_WAIT_MS     2500     // you spoke, nothing matched: how long to wait before "say that again?"
#define MAX_RETRIES         2        // how many "say that again?" before it gives up politely
#define MOOD_HOLD_MS        120000   // a reaction stays on screen this long, then Gawa rests (2 minutes)
#define FULL_REFRESH_EVERY  10       // every Nth screen update is a full (flashing) refresh to clear ghosting
#define FRAME_MS            320      // time between animation frames
#define BLINK_EVERY_MS      25000    // how often Gawa blinks while resting
#define SLEEP_AFTER_MS      600000   // falls asleep after 10 quiet minutes
#define LISTEN_TILT_MS      1500     // head tilts side to side while listening

// ---------------------------------------------------------------- display
static epaper_driver_display *epd = nullptr;
static board_power_bsp_t board_power(EPD_PWR_PIN, Audio_PWR_PIN, VBAT_PWR_PIN);
static int partial_refreshes = 0;
static bool allow_full_refresh = false;   // the flashing clean refresh only happens when Gawa goes back to resting

static void put_black_pixel(int x, int y) { epd->EPD_DrawColorPixel(x, y, DRIVER_COLOR_BLACK); }

static mood_t last_mood = MOOD_IDLE; static int last_var = 0; static const char *last_text = "";
static void show(mood_t mood, int variant, int frame, const char *line)
{
  last_mood = mood; last_var = variant; last_text = line;
  epd->EPD_Clear();
  draw_face(mood, variant, frame, line, put_black_pixel);
  if (allow_full_refresh && partial_refreshes >= FULL_REFRESH_EVERY) {
    epd->EPD_Init(); epd->EPD_Display(); epd->EPD_DisplayPartBaseImage(); epd->EPD_Init_Partial();
    partial_refreshes = 0;
  } else {
    epd->EPD_DisplayPart();
    partial_refreshes++;
  }
}

// play the mood's little animation (4 frames), ending on frame 0
static void play(mood_t mood, int variant, const char *line)
{
  for (int f = 1; f < ANIM_FRAMES; f++) { show(mood, variant, f, line); delay(FRAME_MS); }
  show(mood, variant, 0, line);
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
static bool     *phrase_has_name = nullptr;   // sentence contains "gawa": it can start a conversation by itself
static size_t    phrase_count = 0;

static void build_phrases()
{
  size_t total = 0;
  for (size_t f = 0; f < FAMILY_COUNT; f++) total += FAMILIES[f].ntopics * FAMILIES[f].nendings;
  all_phrases = (sr_cmd_t *)heap_caps_calloc(total, sizeof(sr_cmd_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  phrase_has_name = (bool *)heap_caps_calloc(total, sizeof(bool), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  phrase_count = 0;
  for (size_t f = 0; f < FAMILY_COUNT; f++) {
    const Family &fam = FAMILIES[f];
    for (int t = 0; t < fam.ntopics; t++)
      for (int e = 0; e < fam.nendings; e++) {
        sr_cmd_t &c = all_phrases[phrase_count++];
        c.command_id = CMD_ID(f, t);
        snprintf(c.str, sizeof(c.str), "%s%s%s", fam.prefix, fam.topics[t], fam.endings[e]);
        phrase_has_name[phrase_count - 1] = strstr(c.str, "gawa") != nullptr;
      }
  }
  Serial.printf("Gawa knows %u sentences from %u patterns\n", (unsigned)phrase_count, (unsigned)FAMILY_COUNT);
}

// ---------------------------------------------------------------- pet state
enum State { RESTING, LISTENING, REACTED };
static State    state = RESTING;
static uint32_t state_until = 0;
static int      retries = 0;
static uint32_t rest_since = 0, last_blink = 0, last_tilt = 0, last_sleep_frame = 0;
static int      sleep_frame = 0, tilt_frame = 0;
static bool     asleep = false;
static bool     in_chat = false;
static bool     follow_up = false;      // listening for a follow-up after a reply (the reply face stays on screen)        // a conversation is open: no "Hey Gawa" needed for the next thing you say
static const char *rest_line = "", *listen_line = "";
static int      last_variant = -1, last_line = -1;
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

static void go_rest(const char *line)
{
  g_chat_dot = false;
  allow_full_refresh = true;            // a good moment to clean the screen
  state = RESTING; vad_on = false; retries = 0; asleep = false; in_chat = false;
  rest_since = last_blink = millis(); rest_line = line;
  show(MOOD_IDLE, 0, 0, line);
  allow_full_refresh = false;
}

static void go_listen(const char *line)
{
  g_chat_dot = false;
  state = LISTENING; asleep = false; in_chat = true; follow_up = false;
  state_until = millis() + LISTEN_WINDOW_MS;
  vad_heard_voice = false; vad_on = true; listen_line = line;
  show(MOOD_LISTENING, 0, 1, line); delay(FRAME_MS);
  show(MOOD_LISTENING, 0, 0, line);
  last_tilt = millis(); tilt_frame = 0;
}

static void react(int command_id)
{
  int f = CMD_FAMILY(command_id), t = CMD_TOPIC(command_id);
  if (f < 0 || f >= (int)FAMILY_COUNT) return;
  const Family &fam = FAMILIES[f];
  const char *tmpl = fam.replies[pick_different(fam.nreplies, &last_line)];
  char line[64];
  snprintf(line, sizeof(line), tmpl, fam.topics[t]);      // %s -> the word it heard (if the reply has one)
  Serial.printf("Gawa: %s\n", line);
  g_chat_dot = !fam.ends_chat;        // show the little listening mark while the chat stays open
  play(fam.mood, pick_different(FACE_VARIANTS, &last_variant), line);
  retries = 0;
  if (fam.ends_chat) {                // "thank you", "stop", "good night": the conversation is over
    in_chat = false; follow_up = false;
    state = REACTED; state_until = millis() + MOOD_HOLD_MS;
    vad_on = false;
  } else {                            // keep the chat open: listen for a follow-up, reply face stays on screen
    in_chat = true; follow_up = true;
    state = LISTENING;
    state_until = millis() + FOLLOWUP_MS;
    vad_heard_voice = false; vad_on = true;
  }
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
  if (!vad_on) noise_floor = noise_floor * 0.98f + level * 0.02f;     // learn how loud the room is
  else {
    float threshold = noise_floor * 3.0f; if (threshold < 350) threshold = 350;
    if (level > threshold) { vad_last_voice_ms = millis(); vad_heard_voice = true; }
  }
  return ESP_OK;
}

static void on_voice_event(void *arg, sr_event_t event, int command_id, int phrase_id)
{
  if (event == SR_EVENT_COMMAND) { voice_cmd = command_id; voice_phrase = phrase_id; }
  sr_set_mode(SR_MODE_COMMAND);       // always keep listening
}

// ---------------------------------------------------------------- Arduino
void setup()
{
  Serial.begin(115200);
  delay(300);
  Serial.println("\nGawa v4 starting...");
  randomSeed(esp_random());
  pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);

  board_power.POWEER_EPD_ON();
  board_power.POWEER_Audio_ON();
  delay(50);
  display_init();
  show(MOOD_IDLE, 0, 0, "WAKING UP...");

  build_phrases();
  audio_bsp_init();
  esp_err_t err = sr_start(mic_fill, nullptr, SR_CHANNELS_MONO, SR_MODE_COMMAND, "M",
                           all_phrases, phrase_count, on_voice_event, nullptr);
  if (err != ESP_OK) {
    Serial.printf("Speech recognition failed to start (%d). Is the speech model flashed?\n", err);
    show(MOOD_SAD, 0, 0, "NO VOICE MODEL");
    while (true) delay(1000);
  }
  go_rest(LINE_IDLE);
  Serial.println("Say: Hey Gawa");
}

void loop()
{
  uint32_t now = millis();

  // ---- gear button: step through moods by hand
  static bool was_pressed = false;
  bool pressed = digitalRead(BOOT_BUTTON_PIN) == LOW;
  if (pressed && !was_pressed) {
    static int step = 0;
    const int demo[10] = { CMD_ID(1, 0), CMD_ID(15, 2), CMD_ID(11, 0), CMD_ID(27, 0), CMD_ID(29, 0), CMD_ID(31, 0), CMD_ID(46, 0), CMD_ID(50, 0), CMD_ID(48, 0), CMD_ID(43, 0) };
    react(demo[step]); step = (step + 1) % 10;   // how are you / passed test / cat died / angry / scared / grumpy / nervous / shy / tired / shocked
  }
  was_pressed = pressed;

  // ---- a sentence was recognised
  if (voice_cmd >= 0) {
    int cmd = voice_cmd, ph = voice_phrase; voice_cmd = -1;
    Serial.printf("Heard: \"%s\"\n", (ph >= 0 && ph < (int)phrase_count) ? all_phrases[ph].str : "?");
    int fam = CMD_FAMILY(cmd);
    bool has_name = (ph >= 0 && ph < (int)phrase_count) ? phrase_has_name[ph] : false;
    if (fam == FAMILY_WAKE) {
      go_listen(R_LISTEN[pick_different(N(R_LISTEN), &last_line)]);
    } else if (in_chat || has_name) {
      react(cmd);
    } else {
      Serial.println("  (ignored - say Hey Gawa first)");
    }
  }

  // ---- while listening: did they talk but nothing matched? did they say nothing at all?
  if (state == LISTENING) {
    if (vad_heard_voice && now - vad_last_voice_ms > UNHEARD_WAIT_MS) {
      if (retries < MAX_RETRIES) {
        retries++;
        Serial.println("Talked, but no match -> asking again");
        state_until = now + LISTEN_WINDOW_MS;
        vad_heard_voice = false;
        listen_line = R_DIDNT_HEAR[pick_different(N(R_DIDNT_HEAR), &last_line)];
        play(MOOD_CURIOUS, 0, listen_line);     // magnifying glass: "let me listen harder"
        last_tilt = millis();
      } else {
        Serial.println("Still no match -> giving up politely");
        go_rest(R_GIVE_UP[pick_different(N(R_GIVE_UP), &last_line)]);   // go_rest closes the conversation (calm face, not a telling off)
      }
    } else if (!vad_heard_voice && (int32_t)(now - state_until) >= 0) {
      if (follow_up) {                      // 10 s of quiet after a reply: the conversation ends, face stays up
        Serial.println("Quiet -> conversation over");
        in_chat = false; follow_up = false; vad_on = false;
        state = REACTED; state_until = now + MOOD_HOLD_MS;
        g_chat_dot = false; show(last_mood, last_var, 0, last_text);   // same face, listening mark gone
      } else {
        Serial.println("Nothing said -> resting");
        go_rest(R_NOTHING[pick_different(N(R_NOTHING), &last_line)]);
      }
    }
  }

  // ---- while listening: tilt the head now and then
  if (state == LISTENING && !follow_up && now - last_tilt > LISTEN_TILT_MS) {
    last_tilt = now; tilt_frame ^= 1;
    show(MOOD_LISTENING, 0, tilt_frame, listen_line);
  }

  // ---- a reaction has been up long enough: rest
  if (state == REACTED && (int32_t)(now - state_until) >= 0) go_rest(LINE_IDLE);

  // ---- resting: blink sometimes, fall asleep after a long quiet spell
  if (state == RESTING) {
    if (!asleep && now - rest_since > SLEEP_AFTER_MS) {
      asleep = true; sleep_frame = 0; last_sleep_frame = now;
      show(MOOD_SLEEP, 0, 0, "ZZZ");
    } else if (asleep && now - last_sleep_frame > 30000) {
      last_sleep_frame = now; sleep_frame = (sleep_frame + 1) % ANIM_FRAMES;
      show(MOOD_SLEEP, 0, sleep_frame, "ZZZ");
    } else if (!asleep && now - last_blink > BLINK_EVERY_MS) {
      last_blink = now;
      show(MOOD_IDLE, 0, 2, rest_line); delay(250);      // a tiny breath
      show(MOOD_IDLE, 0, 0, rest_line);
    }
  }

  delay(20);
}
