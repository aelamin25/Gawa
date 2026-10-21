// live.cpp - see live.h. One task owns the secure WebSocket (reads, writes, timers); a mic task feeds it audio.
#include <Arduino.h>
#include <WiFi.h>
#include "esp_transport.h"
#include "esp_transport_ssl.h"
#include "esp_transport_ws.h"
#include "esp_crt_bundle.h"
#include "live.h"
#include "rt_proto.h"
#include "audio_bsp.h"
#include "audio_out.h"

#define HOST            "api.openai.com"
#define MIC_BLOCK       960            // 40 ms at 24 kHz
#define QUIET_END_MS    20000          // nobody talks for 20 s -> the call ends (you never pay for silence)
#define NO_ANSWER_MS    15000          // connected, but the cloud never says hello
#define MAX_CALL_MS     (15UL * 60UL * 1000UL)
#define RX_START        (64 * 1024)    // grows if a message is bigger (in PSRAM)

static SemaphoreHandle_t view_mux = nullptr;
static LiveView view;
static QueueHandle_t mic_q = nullptr;
static volatile bool mic_open = false;         // the live task opens the mic only when it is your turn
static volatile bool stop_wanted = false;
static volatile bool running = false;
static char *cfg_key = nullptr, *cfg_model = nullptr, *cfg_voice = nullptr, *cfg_instr = nullptr, *cfg_greet = nullptr;

// ---------------------------------------------------------------- the shared view
template <typename F> static void edit_view(F f)
{
  xSemaphoreTake(view_mux, portMAX_DELAY);
  f(view);
  view.version++;
  xSemaphoreGive(view_mux);
}
void live_view(LiveView *out)
{
  if (!view_mux) { *out = LiveView(); return; }
  xSemaphoreTake(view_mux, portMAX_DELAY); *out = view; xSemaphoreGive(view_mux);
}
static void set_state(LiveState s)
{
  if (view.state == s) return;     // read without the lock is fine: only this task writes the state during a call
  edit_view([&](LiveView &v) { v.state = s; });
}
static void set_error(const char *e)
{
  Serial.printf("live: error %s\n", e);
  edit_view([&](LiveView &v) { v.state = LV_ERROR; snprintf(v.error, sizeof(v.error), "%s", e); });
}

// ---------------------------------------------------------------- mic task: 40 ms blocks, queued only on your turn
static void mic_task(void *)
{
  static int16_t stereo[MIC_BLOCK * 2], mono[MIC_BLOCK];
  int pick = -1;                                           // which side the mic is on: locked after the first loud block
  for (;;) {
    if (audio_mic_read(stereo, sizeof(stereo)) != 0) { vTaskDelay(pdMS_TO_TICKS(10)); continue; }
    if (!mic_open || audio_out_busy()) continue;           // Pengu is talking (or just stopped): do not listen to it
    int side = pick;
    if (side < 0) {
      int64_t el = 0, er = 0;
      for (int i = 0; i < MIC_BLOCK; i++) { el += abs(stereo[2 * i]); er += abs(stereo[2 * i + 1]); }
      side = er > el ? 1 : 0;
      if ((el + er) / MIC_BLOCK > 600) pick = side;        // clearly someone talking: remember the side for good
    }
    for (int i = 0; i < MIC_BLOCK; i++) mono[i] = stereo[2 * i + side];
    xQueueSend(mic_q, mono, 0);                            // full queue: drop the block (the link is slow)
  }
}

// ---------------------------------------------------------------- the WebSocket
static esp_transport_handle_t ssl = nullptr, ws = nullptr;
static char *rx = nullptr; static size_t rx_cap = 0;
static char *tx = nullptr; static const size_t TX_CAP = 16 * 1024;

static bool ws_send(const char *msg, size_t n)
{
  if (!n) return false;
  int r = esp_transport_ws_send_raw(ws, (ws_transport_opcodes_t)(WS_TRANSPORT_OPCODES_TEXT | WS_TRANSPORT_OPCODES_FIN),
                                    msg, (int)n, 5000);
  return r == (int)n;
}

static bool rx_room(size_t need)
{
  if (need <= rx_cap) return true;
  size_t ncap = need + 16 * 1024;
  char *n = (char *)heap_caps_realloc(rx, ncap, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!n) { Serial.printf("live: no memory for a %u byte message\n", (unsigned)need); return false; }
  rx = n; rx_cap = ncap;
  return true;
}

// Reads one whole text message into rx. Returns its length, 0 if nothing arrived, -1 if the link broke.
// It polls first and only then reads with a long timeout. A short read timeout would hit an IDF 5.5 quirk: after IDF
// answers a PING by itself, the next read that times out on the header re-reads the old PING length and the stream
// loses sync (found in review).
static int ws_read_message()
{
  size_t len = 0;
  for (;;) {
    int p = esp_transport_poll_read(ws, len ? 3000 : 10);
    if (p < 0) return -1;
    if (p == 0) return len ? -1 : 0;                     // idle, or a split message that stalled
    if (!rx_room(len + 4096)) return -1;
    int r = esp_transport_read(ws, rx + len, (int)(rx_cap - len - 1), 5000);
    if (r < 0) return -1;
    ws_transport_opcodes_t op = esp_transport_ws_get_read_opcode(ws);
    if (op == WS_TRANSPORT_OPCODES_NONE) { if (len == 0) return 0; continue; }   // a PING IDF answered itself
    if (op == WS_TRANSPORT_OPCODES_CLOSE) return -1;
    if (op == WS_TRANSPORT_OPCODES_PING || op == WS_TRANSPORT_OPCODES_PONG) { if (len == 0) return 0; continue; }
    int plen = esp_transport_ws_get_read_payload_len(ws);
    if (plen < 0 || !rx_room(len + plen + 1)) return -1;
    int got = r;
    while (got < plen) {                                 // the rest of this frame
      int k = esp_transport_read(ws, rx + len + got, plen - got, 5000);
      if (k <= 0) return -1;
      got += k;
    }
    len += plen;
    if (esp_transport_ws_get_fin_flag(ws)) { rx[len] = 0; return (int)len; }   // the last frame of the message
  }
}

static void ws_teardown()
{
  if (ws) { esp_transport_close(ws); esp_transport_destroy(ws); ws = nullptr; }   // ws first: it shares ssl's socket
  if (ssl) { esp_transport_destroy(ssl); ssl = nullptr; }
}

// a cloud error code -> a short word for the screen
static void error_word(const char *em, char *out, size_t cap)
{
  if (strstr(em, "invalid_api_key")) snprintf(out, cap, "BAD KEY");
  else if (strstr(em, "insufficient_quota") || strstr(em, "billing")) snprintf(out, cap, "NO CREDIT?");
  else if (strstr(em, "model")) snprintf(out, cap, "MODEL?");
  else { const char *c = strchr(em, ':'); int n = c ? (int)(c - em) : (int)strlen(em); if (n > 14) n = 14; snprintf(out, cap, "%.*s", n, em); }
}

// ---------------------------------------------------------------- the call
struct Call {
  uint32_t t_start = 0, last_activity = 0;
  bool session_ready = false;     // session.update sent
  bool greeted = false;           // Pengu's first line is done: only now does the mic open
  bool user_speaking = false;
  uint32_t card_seq = 0;          // which line the card we asked for belongs to
  char pending_err[24] = "";      // the cloud's reason, shown if the link then closes
};

static void handle(char *msg, size_t n, Call &c)
{
  RtEvent e = rt_event_type(msg, n);
  switch (e) {
    case RT_SESSION_CREATED: {
      RtSessionCfg s; s.instructions = cfg_instr; s.voice = cfg_voice; s.silence_ms = 700;
      ws_send(tx, rt_session_update(tx, TX_CAP, s));
      ws_send(tx, rt_response_create(tx, TX_CAP, cfg_greet));        // Pengu picks up the phone and speaks first
      c.session_ready = true; c.last_activity = millis();
      Serial.println("live: session ready, greeting asked");
      break;
    }
    case RT_AUDIO_DELTA: {
      if (stop_wanted) break;                                       // hung up: do not start talking again
      const int16_t *pcm; int ns = rt_audio_delta_decode(msg, n, &pcm);
      if (ns > 0) { audio_out_push(pcm, ns); set_state(LV_PENGU_TALKS); }
      c.last_activity = millis();
      break;
    }
    case RT_RESPONSE_DONE: {
      static RtDone d;
      if (!rt_response_done(msg, n, &d)) break;
      c.last_activity = millis();
      if (d.is_card) {
        if (strtoul(d.seq, nullptr, 10) != c.card_seq) { Serial.println("live: an old card, dropped"); break; }
        static char py[256], en[128];
        if (rt_parse_card(d.text, py, sizeof(py), en, sizeof(en))) {
          Serial.printf("live: card %s / %s\n", py, en);
          edit_view([&](LiveView &v) { snprintf(v.pinyin, sizeof(v.pinyin), "%s", py); snprintf(v.english, sizeof(v.english), "%s", en); });
        } else Serial.printf("live: card not understood: %s\n", d.text);
        break;
      }
      c.greeted = true;
      if (d.text[0]) {
        Serial.printf("live: Pengu said %s (%s)\n", d.text, d.status);
        edit_view([&](LiveView &v) { snprintf(v.hanzi, sizeof(v.hanzi), "%s", d.text); v.pinyin[0] = 0; v.english[0] = 0; });
        ws_send(tx, rt_card_request(tx, TX_CAP, d.text, ++c.card_seq));   // pinyin + English for the screen
      } else Serial.printf("live: response %s, no words\n", d.status);
      break;
    }
    case RT_SPEECH_STARTED: c.user_speaking = true;  c.last_activity = millis(); Serial.println("live: you are talking"); break;
    case RT_SPEECH_STOPPED: c.user_speaking = false; c.last_activity = millis(); break;
    case RT_USER_TRANSCRIPT: {
      static char said[128];
      if (rt_json_string(msg, n, "transcript", said, sizeof(said))) {
        Serial.printf("live: you said %s\n", said);
        edit_view([&](LiveView &v) { snprintf(v.heard, sizeof(v.heard), "%s", said); });
      }
      break;
    }
    case RT_ERROR: {
      static char em[160];
      if (rt_error_message(msg, n, em, sizeof(em))) {
        Serial.printf("live: cloud says %s\n", em);                   // most errors are recoverable
        if (!c.greeted || strstr(em, "invalid_api_key") || strstr(em, "quota")) error_word(em, c.pending_err, sizeof(c.pending_err));
      }
      break;
    }
    default: break;
  }
}

static void live_task(void *)
{
  Call c;
  c.t_start = c.last_activity = millis();
  bool ended_ok = false;
  xQueueReset(mic_q);

  if (WiFi.status() != WL_CONNECTED) { set_error("NO WIFI"); goto done; }
  {
    ssl = esp_transport_ssl_init();
    esp_transport_ssl_crt_bundle_attach(ssl, esp_crt_bundle_attach);   // checks api.openai.com's certificate
    ws = esp_transport_ws_init(ssl);
    char path[96]; snprintf(path, sizeof(path), "/v1/realtime?model=%s", cfg_model);
    esp_transport_ws_set_path(ws, path);
    static char headers[320];
    int hl = snprintf(headers, sizeof(headers), "Authorization: Bearer %s\r\n", cfg_key);
    if (hl <= 0 || hl >= (int)sizeof(headers)) { set_error("KEY TOO LONG"); ws_teardown(); goto done; }
    esp_transport_ws_set_headers(ws, headers);
    Serial.printf("live: connecting to %s%s\n", HOST, path);
    if (esp_transport_connect(ws, HOST, 443, 15000) < 0) {
      int st = esp_transport_ws_get_upgrade_request_status(ws);
      int en = esp_transport_get_errno(ws);
      char e[40];
      if (st == 401 || st == 403) snprintf(e, sizeof(e), "BAD KEY");
      else if (st == 400 || st == 404) snprintf(e, sizeof(e), "MODEL?");
      else if (st == 429) snprintf(e, sizeof(e), "NO CREDIT?");
      else if (st > 0) snprintf(e, sizeof(e), "CLOUD? %d", st);
      else snprintf(e, sizeof(e), "NO INTERNET %d", en);
      // if this shows "Header size exceeded buffer size" just above, the cloud's reply headers outgrew IDF's 1 KB buffer
      Serial.printf("live: connect failed, HTTP status %d, errno %d, free heap %u\n", st, en, (unsigned)ESP.getFreeHeap());
      set_error(e);
      ws_teardown();
      goto done;
    }
  }
  Serial.printf("live: connected (free heap %u)\n", (unsigned)ESP.getFreeHeap());

  while (!stop_wanted) {
    int n = ws_read_message();
    if (n < 0) { set_error(c.pending_err[0] ? c.pending_err : "LINK LOST"); break; }
    if (n > 0) handle(rx, n, c);

    bool talking = audio_out_busy();
    if (talking || c.user_speaking) c.last_activity = millis();
    mic_open = c.greeted && !talking;
    if (c.greeted && !talking && view.state != LV_LISTENING) set_state(LV_LISTENING);

    static int16_t block[MIC_BLOCK];
    bool broken = false;
    for (int i = 0; i < 4 && xQueueReceive(mic_q, block, 0) == pdTRUE; i++)
      if (mic_open && !ws_send(tx, rt_audio_append(block, MIC_BLOCK, tx, TX_CAP))) { broken = true; break; }
    if (broken) { set_error("LINK LOST"); break; }

    uint32_t now = millis();
    if (!c.session_ready && now - c.t_start > NO_ANSWER_MS) { set_error(c.pending_err[0] ? c.pending_err : "NO ANSWER"); break; }
    if (c.session_ready && now - c.last_activity > QUIET_END_MS) { Serial.println("live: 20 s of quiet, hanging up"); ended_ok = true; break; }
    if (now - c.t_start > MAX_CALL_MS) { Serial.println("live: 15 minutes, hanging up"); ended_ok = true; break; }
  }
  if (stop_wanted) { ended_ok = true; audio_out_clear(); }
  mic_open = false;
  ws_teardown();
done:
  mic_open = false;
  if (ended_ok) set_state(LV_ENDED);
  Serial.printf("live: call over after %lu s\n", (unsigned long)((millis() - c.t_start) / 1000));
  running = false;
  vTaskDelete(nullptr);
}

// ---------------------------------------------------------------- public
void live_begin()
{
  if (view_mux) return;
  view_mux = xSemaphoreCreateMutex();
  mic_q = xQueueCreate(12, MIC_BLOCK * sizeof(int16_t));
  rx = (char *)heap_caps_malloc(RX_START, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT); rx_cap = rx ? RX_START : 0;
  tx = (char *)heap_caps_malloc(TX_CAP, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  xTaskCreatePinnedToCore(mic_task, "mic", 4096, nullptr, 5, nullptr, 1);
}

static char *keep(char *old, const char *s) { free(old); return strdup(s ? s : ""); }

bool live_start(const LiveConfig &cfg)
{
  if (running || !view_mux) return false;
  if (!rx || !tx) {
    edit_view([](LiveView &v) { v.state = LV_ERROR; snprintf(v.error, sizeof(v.error), "NO PSRAM"); });
    return false;
  }
  cfg_key = keep(cfg_key, cfg.api_key); cfg_model = keep(cfg_model, cfg.model); cfg_voice = keep(cfg_voice, cfg.voice);
  cfg_instr = keep(cfg_instr, cfg.instructions); cfg_greet = keep(cfg_greet, cfg.greeting);
  edit_view([](LiveView &v) {                         // a clean view before the task starts (no old lines)
    v.state = LV_CONNECTING; v.hanzi[0] = v.pinyin[0] = v.english[0] = v.heard[0] = v.error[0] = 0;
  });
  stop_wanted = false; running = true;
  audio_out_clear();
  if (xTaskCreatePinnedToCore(live_task, "live", 12288, nullptr, 4, nullptr, 0) != pdPASS) { running = false; return false; }
  return true;
}

void live_stop()
{
  stop_wanted = true;
  audio_out_clear();
}

bool live_running() { return running; }
