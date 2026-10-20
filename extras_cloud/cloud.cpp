#include "cloud.h"
#include "settings.h"
#include <WiFi.h>
#include <NetworkClientSecure.h>
#include <HTTPClient.h>
#include "src/ArduinoJson/ArduinoJson.h"

static const char *API_HOST = "https://api.openai.com";

// Gawa's personality. Keep it short and warm.
static const char *SYSTEM_PROMPT =
  "You are Gawa, a small friendly pet who lives on a desk and listens to one person talk about their day. "
  "Reply like a kind, simple friend: warm, playful, easy words a 9 year old knows. "
  "Be glad with them, sorry with them, or curious with them. Never give advice, never judge, never nag, "
  "never say you are an AI. No emoji. At most 60 characters. Usually one short sentence, sometimes a tiny question. "
  "Answer ONLY with JSON like {\"mood\":\"happy\",\"reply\":\"...\"} where mood is how YOU feel about what they said: "
  "happy, sad, thinking (curious or unsure) or neutral.";

// ---------------------------------------------------------------- conversation memory
#define HISTORY_MAX 8      // messages kept (4 exchanges)
static String history_role[HISTORY_MAX];
static String history_text[HISTORY_MAX];
static int history_len = 0;
static uint32_t history_last_ms = 0;

static void history_add(const char *role, const String &text)
{
  if (history_len == HISTORY_MAX) {
    for (int i = 1; i < HISTORY_MAX; i++) { history_role[i - 1] = history_role[i]; history_text[i - 1] = history_text[i]; }
    history_len--;
  }
  history_role[history_len] = role;
  history_text[history_len] = text;
  history_len++;
  history_last_ms = millis();
}

void cloud_forget() { history_len = 0; }

// ---------------------------------------------------------------- wifi
bool cloud_wifi_connect(uint32_t timeout_ms)
{
  if (!settings.has_wifi()) return false;
  if (WiFi.status() == WL_CONNECTED) return true;
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(settings.wifi_ssid.c_str(), settings.wifi_pass.c_str());
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < timeout_ms) delay(100);
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("Wi-Fi connected: %s\n", WiFi.localIP().toString().c_str());
    return true;
  }
  Serial.println("Wi-Fi not connected (yet)");
  return false;
}

bool cloud_online() { return settings.has_wifi() && settings.has_key() && WiFi.status() == WL_CONNECTED; }

// ---------------------------------------------------------------- step 1: speech -> text
static bool transcribe(const int16_t *pcm, size_t samples, String &text_out, String &err)
{
  const size_t pcm_bytes = samples * 2;
  const String boundary = "----GawaBoundary7MA4YWxkTrZu0gW";
  String head;
  head += "--" + boundary + "\r\nContent-Disposition: form-data; name=\"model\"\r\n\r\n" + settings.model_stt + "\r\n";
  head += "--" + boundary + "\r\nContent-Disposition: form-data; name=\"languages[]\"\r\n\r\nen\r\n";
  head += "--" + boundary + "\r\nContent-Disposition: form-data; name=\"prompt\"\r\n\r\nSomeone talking to their pet Gawa about their day.\r\n";
  head += "--" + boundary + "\r\nContent-Disposition: form-data; name=\"file\"; filename=\"say.wav\"\r\nContent-Type: audio/wav\r\n\r\n";
  String tail = "\r\n--" + boundary + "--\r\n";

  // WAV header (44 bytes) for 16 kHz mono 16-bit
  uint8_t wav[44];
  uint32_t data_len = pcm_bytes, file_len = 36 + data_len;
  memcpy(wav, "RIFF", 4); memcpy(wav + 4, &file_len, 4); memcpy(wav + 8, "WAVEfmt ", 8);
  uint32_t fmt_len = 16; uint16_t fmt = 1, ch = 1, bits = 16, align = 2; uint32_t rate = 16000, byte_rate = 32000;
  memcpy(wav + 16, &fmt_len, 4); memcpy(wav + 20, &fmt, 2); memcpy(wav + 22, &ch, 2); memcpy(wav + 24, &rate, 4);
  memcpy(wav + 28, &byte_rate, 4); memcpy(wav + 32, &align, 2); memcpy(wav + 34, &bits, 2);
  memcpy(wav + 36, "data", 4); memcpy(wav + 40, &data_len, 4);

  size_t total = head.length() + 44 + pcm_bytes + tail.length();
  uint8_t *body = (uint8_t *)heap_caps_malloc(total, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!body) { err = "NO MEMORY"; return false; }
  size_t o = 0;
  memcpy(body + o, head.c_str(), head.length()); o += head.length();
  memcpy(body + o, wav, 44); o += 44;
  memcpy(body + o, pcm, pcm_bytes); o += pcm_bytes;
  memcpy(body + o, tail.c_str(), tail.length()); o += tail.length();

  NetworkClientSecure client;
  client.setInsecure();                     // skip certificate check (fine for a desk pet; see README)
  HTTPClient http;
  http.setTimeout(25000);
  http.begin(client, String(API_HOST) + "/v1/audio/transcriptions");
  http.addHeader("Authorization", "Bearer " + settings.api_key);
  http.addHeader("Content-Type", "multipart/form-data; boundary=" + boundary);
  int code = http.POST(body, total);
  String resp = http.getString();
  http.end();
  free(body);

  Serial.printf("transcribe: HTTP %d, %u bytes\n", code, (unsigned)resp.length());
  if (code != 200) {
    Serial.println(resp);
    err = (code == 401) ? "BAD KEY" : (code == 404 || code == 400) ? "HEARING MODEL?" : "NO ANSWER";
    return false;
  }
  JsonDocument doc;
  if (deserializeJson(doc, resp)) { err = "ODD REPLY"; return false; }
  text_out = doc["text"] | "";
  text_out.trim();
  return true;
}

// ---------------------------------------------------------------- step 2: text -> Gawa's reply
static bool think(const String &heard, mood_t &mood_out, String &reply_out, String &err)
{
  JsonDocument req;
  req["model"] = settings.model_chat;
  req["max_completion_tokens"] = 300;
  req["reasoning_effort"] = "low";
  req["response_format"]["type"] = "json_object";
  JsonArray msgs = req["messages"].to<JsonArray>();
  JsonObject sys = msgs.add<JsonObject>(); sys["role"] = "system"; sys["content"] = SYSTEM_PROMPT;
  for (int i = 0; i < history_len; i++) { JsonObject m = msgs.add<JsonObject>(); m["role"] = history_role[i]; m["content"] = history_text[i]; }
  JsonObject u = msgs.add<JsonObject>(); u["role"] = "user"; u["content"] = heard;
  String body; serializeJson(req, body);

  NetworkClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setTimeout(25000);
  http.begin(client, String(API_HOST) + "/v1/chat/completions");
  http.addHeader("Authorization", "Bearer " + settings.api_key);
  http.addHeader("Content-Type", "application/json");
  int code = http.POST(body);
  String resp = http.getString();
  http.end();

  Serial.printf("think: HTTP %d, %u bytes\n", code, (unsigned)resp.length());
  if (code != 200) {
    Serial.println(resp);
    err = (code == 401) ? "BAD KEY" : (code == 404 || code == 400) ? "BRAIN MODEL?" : "NO ANSWER";
    return false;
  }
  JsonDocument doc;
  if (deserializeJson(doc, resp)) { err = "ODD REPLY"; return false; }
  const char *content = doc["choices"][0]["message"]["content"] | "";
  JsonDocument inner;
  if (deserializeJson(inner, content)) { reply_out = content; mood_out = MOOD_IDLE; }
  else {
    reply_out = inner["reply"] | "";
    String m = inner["mood"] | "neutral";
    mood_out = (m == "happy") ? MOOD_HAPPY : (m == "sad") ? MOOD_SAD : (m == "thinking") ? MOOD_THINKING : MOOD_IDLE;
  }
  reply_out.trim();
  if (reply_out.length() == 0) { err = "ODD REPLY"; return false; }
  history_add("user", heard);
  history_add("assistant", content);
  return true;
}

CloudReply cloud_talk(const int16_t *pcm, size_t samples)
{
  CloudReply r = { false, MOOD_IDLE, "", "", "" };
  if (!cloud_online()) { r.error = "NO WIFI"; return r; }
  if (millis() - history_last_ms > 10UL * 60UL * 1000UL) cloud_forget();   // fresh chat after 10 quiet minutes

  if (!transcribe(pcm, samples, r.heard, r.error)) return r;
  Serial.printf("Heard: \"%s\"\n", r.heard.c_str());
  if (r.heard.length() < 2) { r.ok = true; r.mood = MOOD_THINKING; r.reply = "I did not catch that"; return r; }

  if (!think(r.heard, r.mood, r.reply, r.error)) return r;
  Serial.printf("Gawa: \"%s\"\n", r.reply.c_str());
  r.ok = true;
  return r;
}
