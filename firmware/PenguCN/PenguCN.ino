/*
 * Pengu v14a "pipe test" - a little penguin from Harbin you can talk to in Chinese (Waveshare ESP32-S3-ePaper-1.54, V2)
 *
 * Press the gear: Pengu picks up the phone ("喂？") and says 你好！ Then you talk, it answers, out loud, in very simple
 * Chinese. The screen shows what Pengu said in big characters, the pinyin under them, and a little English.
 * Pengu starts with the most basic words (你好, 我是Pengu, 谢谢) and builds up slowly.
 *
 *   gear, short press   start a call / hang up
 *   gear, hold 3 s      setup page (Wi-Fi, OpenAI key, voice): join the Wi-Fi "Pengu-Setup" with your phone
 *   power button        full restart
 *   say 再见 (zài jiàn)  Pengu says goodbye; the call ends after 20 s of quiet anyway
 *
 * How it works: live.cpp keeps a secure WebSocket to OpenAI's live voice (Realtime API). The mic streams up while it
 * is your turn and is muted while Pengu talks. Pengu's voice plays from a buffer (audio_out.cpp). After each reply a
 * second, text-only request turns the characters into pinyin + English for the screen (rt_proto.h). The Chinese font
 * lives in its own flash partition ("font", made by tools/make_font.py).
 *
 * v14a is the pipe test: no "Hey Pengu", no lessons yet. Those come in v14 once this works.
 */

#include <Arduino.h>
#include <WiFi.h>
#include "esp_partition.h"
#include "user_config.h"
#include "audio_bsp.h"
#include "audio_out.h"
#include "settings.h"
#include "live.h"
#include "screen_cn.h"
#include "src/display/epaper_driver_bsp.h"
#include "src/power/board_power_bsp.h"

#define VOICE_VOLUME     85        // 0..100; the codec is quiet below 70
#define SETUP_HOLD_MS    3000      // hold the gear this long at rest for the setup page
#define REDRAW_MIN_MS    350       // e-paper: at most one quick update every 350 ms
#define FULL_REFRESH_EVERY 30      // a clean (flashing) refresh at rest after this many quick ones
#define HOLD_HANZI_MS    4500      // the strip under Pengu: characters this long...
#define HOLD_PINYIN_MS   3500      // ...then the pinyin...
#define HOLD_ENGLISH_MS  3000      // ...then the English, then round again (soft fades in between)

// ---------------------------------------------------------------- who Pengu is (sent to the cloud at the start of each call)
static const char PENGU_INSTRUCTIONS[] =
  "You are Pengu, a small, warm, cheerful penguin from Harbin who lives with the learner. You talk on a tiny device "
  "with a speaker and a small screen that shows what you say. The learner is an absolute beginner in Mandarin "
  "Chinese: they know only a handful of words. Your job: a tiny, friendly chat that starts super basic and builds up "
  "very slowly.\n"
  "Rules:\n"
  "- Speak only Mandarin Chinese, slowly and clearly, like to a small child. Sound small, cute and happy.\n"
  "- Every reply is ONE short sentence of 2 to 6 Chinese characters. Never more than 8 characters.\n"
  "- Use only the most basic words: 你好, 我, 你, 是, 好, 很, 不, 吗, 呢, 对, 谢谢, 不客气, 再见, 叫, 什么, 名字, "
  "喜欢, 吃, 喝, 水, 鱼, 要, 不要. Add at most ONE new word at a time, and only once the learner has used the earlier ones.\n"
  "- Build up in this order, one step per turn, moving on only when the learner manages the step: "
  "你好！ -> 我是Pengu。 -> 你好吗？ (teach 我很好) -> 你叫什么名字？ -> 谢谢！/不客气 -> 你喜欢鱼吗？ -> 再见！\n"
  "- Repeat the same words often so they stick.\n"
  "- If the learner is silent, says 什么, or seems confused: say the same sentence again, slower, or even simpler.\n"
  "- If the learner speaks English: answer with the shortest Chinese way to say what they meant "
  "(they say 'thank you' -> you say 谢谢！), then wait for them to copy it.\n"
  "- Praise small wins with 很好！ or 对！\n"
  "- Never speak English, never spell pinyin aloud, never explain grammar, never lecture or correct harshly.\n"
  "- When the learner says 再见 or bye, say 再见！ and nothing more.\n"
  "- Write Pengu as 'Pengu' in your text.";

// the first reply's own instructions replace the session's, so the greeting carries the whole persona too
static const char PENGU_GREETING_NOW[] = "\n\nThis is the very start of the call: say only 你好！";

// ---------------------------------------------------------------- display
static epaper_driver_display *epd = nullptr;
static board_power_bsp_t board_power(EPD_PWR_PIN, Audio_PWR_PIN, VBAT_PWR_PIN);
static int partial_refreshes = 0;
static uint32_t last_draw = 0;
static CnFonts fonts;
static bool fonts_ok = false;

static void put_black_pixel(int x, int y) { epd->EPD_DrawColorPixel(x, y, DRIVER_COLOR_BLACK); }

static void flush(bool allow_full)
{
  if (allow_full && partial_refreshes >= FULL_REFRESH_EVERY) {
    epd->EPD_Init(); epd->EPD_Display(); epd->EPD_DisplayPartBaseImage(); epd->EPD_Init_Partial();
    partial_refreshes = 0;
  } else {
    epd->EPD_DisplayPart();
    partial_refreshes++;
  }
}

static void show_en(mood_t mood, const char *line, const char *corner = "", bool allow_full = false)
{
  epd->EPD_Clear();
  draw_en(mood, 0, line, corner, put_black_pixel);
  flush(allow_full);
}

static void show_card(mood_t mood, CnPart part, const char *text, int ink, int parts, bool listening, bool full)
{
  epd->EPD_Clear();
  if (fonts_ok) draw_card(&fonts, mood, 0, part, text, ink, parts, listening, "", put_black_pixel);
  else draw_en(mood, 0, "NO FONT", "", put_black_pixel);
  flush(full);
}

static void show_status(const char *line) { show_en(MOOD_IDLE, line); }

static void display_init()
{
  custom_lcd_spi_t cfg = {};
  cfg.cs = EPD_CS_PIN;   cfg.dc = EPD_DC_PIN;   cfg.rst = EPD_RST_PIN;  cfg.busy = EPD_BUSY_PIN;
  cfg.mosi = EPD_MOSI_PIN; cfg.scl = EPD_SCK_PIN; cfg.spi_host = EPD_SPI_NUM;
  cfg.buffer_len = EPD_WIDTH * EPD_HEIGHT / 8;
  epd = new epaper_driver_display(EPD_WIDTH, EPD_HEIGHT, cfg);
  epd->EPD_Init(); epd->EPD_Clear(); epd->EPD_DisplayPartBaseImage(); epd->EPD_Init_Partial();
}

static bool font_init()
{
  const esp_partition_t *p = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "font");
  if (!p) { Serial.println("font: no 'font' partition"); return false; }
  const void *map = nullptr; esp_partition_mmap_handle_t h;
  if (esp_partition_mmap(p, 0, p->size, ESP_PARTITION_MMAP_DATA, &map, &h) != ESP_OK) { Serial.println("font: mmap failed"); return false; }
  bool ok = cn_font_open((const uint8_t *)map, p->size, &fonts);
  Serial.printf("font: %s (%u characters)\n", ok ? "ok" : "not flashed?", ok ? (unsigned)fonts.big.count : 0);
  return ok;
}

// ---------------------------------------------------------------- Wi-Fi
static void wifi_start()
{
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);                 // a live call needs the radio awake
  WiFi.setAutoReconnect(true);
  WiFi.begin(settings.wifi_ssid.c_str(), settings.wifi_pass.c_str());
}

static bool wifi_wait(uint32_t ms)
{
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < ms) delay(100);
  return WiFi.status() == WL_CONNECTED;
}

// ---------------------------------------------------------------- the strip: characters -> pinyin -> English
// One thing at a time under a big Pengu. A new line from Pengu always starts with the characters (they match the
// voice). The pinyin + English arrive a moment later; then the strip takes turns, with a soft fade in between.
struct Strip {
  char text[3][256] = { "", "", "" };   // CN_HANZI, CN_PINYIN, CN_ENGLISH
  char from[256] = "";                  // what the fade starts from (kept, because a new line clears text[])
  int parts = 0;                        // 1 = only the characters so far, 3 = all of them
  CnPart part = CN_HANZI;               // what is on screen
  int fade = -1;                        // -1 = still; 0..2 = steps of a fade to `next`
  CnPart next = CN_HANZI;
  uint32_t until = 0;                   // when to move on to the next part
  mood_t mood = MOOD_CURIOUS;
  bool listening = false;
  bool dirty = true;                    // redraw (no fade) on the next tick
  bool full = false;                    // that redraw is a clean (flashing) one: clears e-paper ghosting
};
static Strip strip;

static uint32_t hold_for(CnPart p) { return p == CN_HANZI ? HOLD_HANZI_MS : p == CN_PINYIN ? HOLD_PINYIN_MS : HOLD_ENGLISH_MS; }

static void strip_new_line(const char *hanzi)
{
  bool had = strip.text[CN_HANZI][0] != 0;
  snprintf(strip.from, sizeof(strip.from), "%s", strip.text[strip.part]);
  snprintf(strip.text[CN_HANZI], sizeof(strip.text[0]), "%s", hanzi);
  strip.text[CN_PINYIN][0] = strip.text[CN_ENGLISH][0] = 0;
  strip.parts = 1;
  if (partial_refreshes >= FULL_REFRESH_EVERY) {               // many quick updates so far: this one is a clean refresh
    strip.part = CN_HANZI; strip.fade = -1; strip.dirty = true; strip.full = true;
  } else if (had && strip.part != CN_HANZI) { strip.next = CN_HANZI; strip.fade = 0; }   // fade from the old part
  else { strip.part = CN_HANZI; strip.fade = -1; strip.dirty = true; }
  strip.until = millis() + HOLD_HANZI_MS;
}

static void strip_tick()
{
  uint32_t now = millis();
  if (now - last_draw < REDRAW_MIN_MS) return;
  if (strip.fade >= 0) {                                       // one step of a fade
    epd->EPD_Clear();
    draw_fade_step(&fonts, strip.mood, strip.fade, strip.part, strip.from, strip.next, strip.text[strip.next], strip.parts,
                   strip.listening, put_black_pixel);
    flush(false);
    last_draw = now;
    if (++strip.fade > 2) { strip.part = strip.next; strip.fade = -1; strip.dirty = false; strip.until = now + hold_for(strip.part); }
    return;
  }
  if (strip.dirty) {
    show_card(strip.mood, strip.part, strip.text[strip.part], CN_INK_FULL, strip.parts, strip.listening, strip.full);
    strip.dirty = false; strip.full = false; last_draw = now;
    return;
  }
  if (strip.parts == 3 && (int32_t)(now - strip.until) >= 0) {   // time to take turns (an empty part is skipped)
    CnPart nx = strip.part;
    for (int i = 0; i < 3; i++) { nx = (CnPart)((nx + 1) % 3); if (strip.text[nx][0]) break; }
    if (nx == strip.part) { strip.until = now + hold_for(strip.part); return; }
    strip.next = nx;
    snprintf(strip.from, sizeof(strip.from), "%s", strip.text[strip.part]);
    strip.fade = 0;
  }
}

// ---------------------------------------------------------------- the pet
enum Mode { REST, CALL };
static Mode mode = REST;
static uint32_t shown_version = 0;
static char rest_line[40] = "PRESS THE GEAR";

static void go_rest(const char *line, mood_t mood = MOOD_IDLE)
{
  mode = REST;
  snprintf(rest_line, sizeof(rest_line), "%s", line);
  show_en(mood, rest_line, "", true);
}

static void start_call()
{
  if (!fonts_ok) { go_rest("NO FONT - REFLASH", MOOD_SAD); return; }
  if (!settings.has_key()) { go_rest("NO KEY - HOLD GEAR", MOOD_SAD); return; }
  for (int i = 0; i < 60 && live_running(); i++) delay(50);   // the last call may still be closing its link
  if (WiFi.status() != WL_CONNECTED) {
    show_en(MOOD_THINKING, "FINDING WIFI...");
    if (!wifi_wait(8000)) { go_rest("NO WIFI", MOOD_SAD); return; }
  }
  LiveConfig c;
  c.api_key = settings.api_key.c_str(); c.model = settings.model.c_str(); c.voice = settings.voice.c_str();
  static String greeting; greeting = String(PENGU_INSTRUCTIONS) + PENGU_GREETING_NOW;
  c.instructions = PENGU_INSTRUCTIONS; c.greeting = greeting.c_str();
  if (!live_start(c)) { go_rest("BUSY - TRY AGAIN", MOOD_THINKING); return; }
  mode = CALL; shown_version = 0;
  strip = Strip();                                            // the phone is picked up: "喂？" (wéi? = hello?)
  strip.mood = MOOD_CURIOUS;
  snprintf(strip.text[CN_HANZI], sizeof(strip.text[0]), "喂？");
  snprintf(strip.text[CN_PINYIN], sizeof(strip.text[0]), "wéi?");
  snprintf(strip.text[CN_ENGLISH], sizeof(strip.text[0]), "Hello? (on the phone)");
  strip.parts = 3; strip.until = millis() + HOLD_HANZI_MS;
  last_draw = 0;
  strip_tick();
}

static void call_tick()
{
  LiveView v; live_view(&v);
  if (v.state == LV_ENDED && !live_running()) { go_rest("ZAI JIAN! BYE!", MOOD_HAPPY); return; }
  if (v.state == LV_ERROR && !live_running()) { go_rest(v.error, MOOD_SAD); return; }
  if (v.version != shown_version) {
    shown_version = v.version;
    if (v.hanzi[0] && strcmp(v.hanzi, strip.text[CN_HANZI]) != 0) strip_new_line(v.hanzi);
    if (v.pinyin[0] && strip.parts < 3 && strcmp(v.hanzi, strip.text[CN_HANZI]) == 0) {   // the card arrived: show the dots
      snprintf(strip.text[CN_PINYIN], sizeof(strip.text[0]), "%s", v.pinyin);
      snprintf(strip.text[CN_ENGLISH], sizeof(strip.text[0]), "%s", v.english);
      strip.parts = 3;
      if (strip.fade < 0) strip.dirty = true;
    }
    bool listening = v.state == LV_LISTENING;
    mood_t mood = listening ? MOOD_IDLE : (v.state == LV_CONNECTING ? MOOD_CURIOUS : MOOD_HAPPY);
    if (listening != strip.listening || mood != strip.mood) {
      strip.listening = listening; strip.mood = mood;
      if (strip.fade < 0) strip.dirty = true;
    }
  }
  strip_tick();
}

// ---------------------------------------------------------------- buttons (their own task: an e-paper update blocks the
// loop for ~0.3 s, three in a row during a fade, and a quick click on the gear must never be missed)
enum BtnEvent { BTN_NONE = 0, BTN_SHORT, BTN_HOLD, BTN_POWER };
static volatile BtnEvent btn_event = BTN_NONE;

static void button_task(void *)
{
  bool was = false, held = false; uint32_t down_at = 0, last_edge = 0;
  int pwr_idle = -1, seen = -1; uint32_t seen_since = 0, pwr_since = 0;
  for (;;) {
    uint32_t now = millis();
    bool pressed = digitalRead(BOOT_BUTTON_PIN) == LOW;
    if (pressed != was && now - last_edge < 40) pressed = was;      // debounce
    if (pressed != was) last_edge = now;
    if (pressed && !was) { down_at = now; held = false; }
    if (pressed && !held && now - down_at > SETUP_HOLD_MS) { held = true; btn_event = BTN_HOLD; }
    if (!pressed && was && !held) btn_event = BTN_SHORT;
    was = pressed;
    // power button: the resting level is learned in the first second (as in v13), then a press = full restart
    int lvl = digitalRead(PWR_BUTTON_PIN);
    if (pwr_idle < 0) {
      if (lvl != seen) { seen = lvl; seen_since = now; }
      else if (now - seen_since > 1000) pwr_idle = lvl;
    } else if (lvl != pwr_idle) {
      if (!pwr_since) pwr_since = now;
      if (now - pwr_since > 150) btn_event = BTN_POWER;
    } else pwr_since = 0;
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

// ---------------------------------------------------------------- Arduino

void setup()
{
  Serial.begin(115200);
  delay(300);
  Serial.println("\nPengu v14a (Chinese pipe test) starting...");
  pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(PWR_BUTTON_PIN, INPUT_PULLUP);

  board_power.POWEER_EPD_ON();
  board_power.POWEER_Audio_ON();
  delay(50);
  display_init();
  fonts_ok = font_init();
  show_en(MOOD_IDLE, "WAKING UP...");

  settings_load();
  if (!settings.has_wifi()) setup_portal_run(show_status);       // first time: straight to the setup page (restarts)

  audio_bsp_init(24000);
  audio_out_begin(VOICE_VOLUME);
  live_begin();
  xTaskCreatePinnedToCore(button_task, "buttons", 2048, nullptr, 3, nullptr, 1);
  wifi_start();
  show_en(MOOD_THINKING, "FINDING WIFI...");
  bool ok = wifi_wait(15000);
  Serial.printf("wifi: %s %s\n", ok ? "connected" : "not connected", ok ? WiFi.localIP().toString().c_str() : "");
  if (!fonts_ok) go_rest("NO FONT - REFLASH", MOOD_SAD);
  else if (!settings.has_key()) go_rest("NO KEY - HOLD GEAR", MOOD_SAD);
  else go_rest(ok ? "PRESS THE GEAR" : "NO WIFI", ok ? MOOD_IDLE : MOOD_SAD);
}

void loop()
{
  BtnEvent ev = btn_event; btn_event = BTN_NONE;
  if (ev == BTN_POWER) { live_stop(); show_en(MOOD_IDLE, "RESTARTING..."); delay(300); ESP.restart(); }
  if (ev == BTN_HOLD && mode == REST) setup_portal_run(show_status);    // restarts when saved
  if (ev == BTN_SHORT) {
    if (mode == REST) start_call();
    else { Serial.println("gear: hang up"); live_stop(); go_rest("ZAI JIAN! BYE!", MOOD_HAPPY); }   // the link closes in the background
  }
  if (mode == CALL) call_tick();
  delay(20);
}
