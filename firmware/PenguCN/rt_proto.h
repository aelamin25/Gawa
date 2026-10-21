// rt_proto.h - the messages Pengu sends to and reads from OpenAI's Realtime API (GA, WebSocket).
// Pure C++ + ArduinoJson, no Arduino calls, so tools/test_cn/test_rt_proto.cpp checks it on a PC.
//
// Sent:  session.update (who Pengu is, the voice, 24 kHz PCM both ways, server VAD), response.create (the greeting),
//        input_audio_buffer.append (the mic), and an out-of-band text request that turns the characters Pengu just
//        said into pinyin + English for the screen (the "card").
// Read:  the event type, audio deltas (decoded in place, no JSON tree: they are big), response.done (the transcript of
//        an audio reply, or the text of a card reply), errors, and the user's own words.
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <ArduinoJson.h>

// ---------------------------------------------------------------- base64
static const char RT_B64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// out needs 4 * ((n + 2) / 3) + 1 bytes. Returns the length (out is 0-terminated).
static inline size_t rt_b64_encode(const uint8_t *in, size_t n, char *out)
{
  size_t o = 0, i = 0;
  for (; i + 2 < n; i += 3) {
    uint32_t v = (in[i] << 16) | (in[i + 1] << 8) | in[i + 2];
    out[o++] = RT_B64[(v >> 18) & 63]; out[o++] = RT_B64[(v >> 12) & 63];
    out[o++] = RT_B64[(v >> 6) & 63];  out[o++] = RT_B64[v & 63];
  }
  if (i < n) {
    uint32_t v = in[i] << 16; if (i + 1 < n) v |= in[i + 1] << 8;
    out[o++] = RT_B64[(v >> 18) & 63]; out[o++] = RT_B64[(v >> 12) & 63];
    out[o++] = (i + 1 < n) ? RT_B64[(v >> 6) & 63] : '=';
    out[o++] = '=';
  }
  out[o] = 0;
  return o;
}

static inline int rt_b64_val(char c)
{
  if (c >= 'A' && c <= 'Z') return c - 'A';
  if (c >= 'a' && c <= 'z') return c - 'a' + 26;
  if (c >= '0' && c <= '9') return c - '0' + 52;
  if (c == '+') return 62;
  if (c == '/') return 63;
  return -1;
}

// Decodes n base64 characters of s into bytes at the start of s (the output is always shorter than the input).
// Returns the number of bytes, or -1 if a character is not base64.
static inline int rt_b64_decode_inplace(char *s, size_t n)
{
  uint8_t *out = (uint8_t *)s;
  size_t o = 0; uint32_t acc = 0; int bits = 0;
  for (size_t i = 0; i < n; i++) {
    char c = s[i];
    if (c == '=') break;
    if (c == '\\' && i + 1 < n && s[i + 1] == '/') { c = '/'; i++; }   // JSON may escape "/" as "\/"
    int v = rt_b64_val(c);
    if (v < 0) return -1;
    acc = (acc << 6) | (uint32_t)v; bits += 6;
    if (bits >= 8) { bits -= 8; out[o++] = (uint8_t)(acc >> bits); }
  }
  return (int)o;
}

// ---------------------------------------------------------------- building messages
struct RtSessionCfg {
  const char *instructions = "";
  const char *voice = "marin";
  int silence_ms = 700;              // how long a pause means "your turn, Pengu"
};

static inline size_t rt_write(JsonDocument &doc, char *out, size_t cap)
{
  size_t need = measureJson(doc);
  if (need + 1 > cap) { if (cap) out[0] = 0; return 0; }
  return serializeJson(doc, out, cap);
}

static inline size_t rt_session_update(char *out, size_t cap, const RtSessionCfg &c)
{
  JsonDocument doc;
  doc["type"] = "session.update";
  JsonObject s = doc["session"].to<JsonObject>();
  s["type"] = "realtime";
  s["output_modalities"].to<JsonArray>().add("audio");
  s["instructions"] = c.instructions;
  JsonObject in = s["audio"]["input"].to<JsonObject>();
  in["format"]["type"] = "audio/pcm";
  in["format"]["rate"] = 24000;
  in["transcription"]["model"] = "gpt-4o-mini-transcribe";
  JsonObject td = in["turn_detection"].to<JsonObject>();
  td["type"] = "server_vad";
  td["threshold"] = 0.5;
  td["prefix_padding_ms"] = 300;
  td["silence_duration_ms"] = c.silence_ms;
  td["create_response"] = true;
  td["interrupt_response"] = false;     // the mic is muted while Pengu talks, so nothing should cut it off
  JsonObject outa = s["audio"]["output"].to<JsonObject>();
  outa["format"]["type"] = "audio/pcm";
  outa["format"]["rate"] = 24000;
  outa["voice"] = c.voice;
  return rt_write(doc, out, cap);
}

// A reply from Pengu right now (the greeting), with extra instructions for this one reply.
static inline size_t rt_response_create(char *out, size_t cap, const char *instructions)
{
  JsonDocument doc;
  doc["type"] = "response.create";
  if (instructions && *instructions) doc["response"]["instructions"] = instructions;
  return rt_write(doc, out, cap);
}

// Out of band (not part of the chat): "give me pinyin and English for these characters", answered as text.
static const char RT_CARD_INSTRUCTIONS[] =
  "You make flash cards for a beginner learning Mandarin. The user message is Chinese that was just spoken aloud. "
  "Reply with ONLY one line of JSON, no code fence: {\"p\":\"<Hanyu Pinyin with tone marks, one space between words, "
  "tone changes as actually spoken (bú yào, yí gè), keep punctuation like ! and ?>\",\"e\":\"<short natural English, "
  "at most 40 characters>\"}. Keep any English words as they are.";

// seq: which of Pengu's lines this card is for (comes back in metadata.n, so a late card is never shown under a new line)
static inline size_t rt_card_request(char *out, size_t cap, const char *hanzi, uint32_t seq)
{
  JsonDocument doc;
  doc["type"] = "response.create";
  JsonObject r = doc["response"].to<JsonObject>();
  r["conversation"] = "none";
  r["output_modalities"].to<JsonArray>().add("text");
  r["metadata"]["k"] = "card";
  char sq[12]; snprintf(sq, sizeof(sq), "%lu", (unsigned long)seq);
  r["metadata"]["n"] = sq;                 // metadata values must be strings
  r["instructions"] = RT_CARD_INSTRUCTIONS;
  JsonObject item = r["input"].to<JsonArray>().add<JsonObject>();
  item["type"] = "message";
  item["role"] = "user";
  JsonObject part = item["content"].to<JsonArray>().add<JsonObject>();
  part["type"] = "input_text";
  part["text"] = hanzi;
  return rt_write(doc, out, cap);
}

// {"type":"input_audio_buffer.append","audio":"<base64>"}. n = number of 16-bit samples. Returns 0 if cap is too small.
static inline size_t rt_audio_append(const int16_t *pcm, size_t n, char *out, size_t cap)
{
  static const char head[] = "{\"type\":\"input_audio_buffer.append\",\"audio\":\"";
  size_t hl = sizeof(head) - 1, bytes = n * 2, b64 = 4 * ((bytes + 2) / 3);
  if (hl + b64 + 3 > cap) { if (cap) out[0] = 0; return 0; }
  memcpy(out, head, hl);
  rt_b64_encode((const uint8_t *)pcm, bytes, out + hl);    // samples are little endian on the ESP32 and on PCs
  size_t o = hl + b64;
  out[o++] = '"'; out[o++] = '}'; out[o] = 0;
  return o;
}

// ---------------------------------------------------------------- reading messages
enum RtEvent {
  RT_OTHER = 0, RT_SESSION_CREATED, RT_SESSION_UPDATED, RT_AUDIO_DELTA, RT_SPEECH_STARTED, RT_SPEECH_STOPPED,
  RT_RESPONSE_CREATED, RT_RESPONSE_DONE, RT_USER_TRANSCRIPT, RT_ERROR
};

// finds the first "key":"value" in the message and points at the value (returns its length, or -1)
static inline int rt_find_str(const char *msg, size_t n, const char *key, const char **val)
{
  char pat[48]; int pl = snprintf(pat, sizeof(pat), "\"%s\"", key);
  if (pl <= 0 || pl >= (int)sizeof(pat)) return -1;
  const char *end = msg + n;
  for (const char *p = msg; p + pl < end; p++) {
    if (memcmp(p, pat, pl) != 0) continue;
    const char *q = p + pl;
    while (q < end && (*q == ' ' || *q == ':')) q++;
    if (q >= end || *q != '"') continue;
    q++;
    const char *v = q;
    while (q < end && *q != '"') { if (*q == '\\') q++; q++; }
    if (q >= end) return -1;
    *val = v;
    return (int)(q - v);
  }
  return -1;
}

static inline RtEvent rt_event_type(const char *msg, size_t n)
{
  const char *v; int l = rt_find_str(msg, n, "type", &v);
  if (l <= 0) return RT_OTHER;
  struct { const char *name; RtEvent e; } map[] = {
    { "response.output_audio.delta", RT_AUDIO_DELTA },
    { "session.created", RT_SESSION_CREATED }, { "session.updated", RT_SESSION_UPDATED },
    { "input_audio_buffer.speech_started", RT_SPEECH_STARTED }, { "input_audio_buffer.speech_stopped", RT_SPEECH_STOPPED },
    { "response.created", RT_RESPONSE_CREATED }, { "response.done", RT_RESPONSE_DONE },
    { "conversation.item.input_audio_transcription.completed", RT_USER_TRANSCRIPT }, { "error", RT_ERROR },
  };
  for (auto &m : map)
    if ((int)strlen(m.name) == l && memcmp(v, m.name, l) == 0) return m.e;
  return RT_OTHER;
}

// Decodes the "delta" of an audio delta in place. *pcm points at the 16-bit samples (inside msg). Returns the sample
// count, or -1. msg must be writable.
static inline int rt_audio_delta_decode(char *msg, size_t n, const int16_t **pcm)
{
  const char *v; int l = rt_find_str(msg, n, "delta", &v);
  if (l < 0) return -1;
  char *start = (char *)v;
  if (((uintptr_t)start & 1) != 0) { memmove(start - 1, start, l); start--; }   // keep the samples 2-byte aligned
  int bytes = rt_b64_decode_inplace(start, l);
  if (bytes < 0) return -1;
  *pcm = (const int16_t *)start;
  return bytes / 2;
}

struct RtDone {
  bool is_card = false;          // our out-of-band card request (metadata.k == "card")
  char status[16] = "";          // completed, cancelled, failed, incomplete
  char text[512] = "";           // what Pengu said (audio transcript) or the card's text
  char seq[12] = "";             // the card's metadata.n
};

static inline bool rt_response_done(const char *msg, size_t n, RtDone *d)
{
  JsonDocument filter;
  filter["response"]["status"] = true;
  filter["response"]["metadata"]["k"] = true;
  filter["response"]["metadata"]["n"] = true;
  filter["response"]["output"][0]["type"] = true;
  filter["response"]["output"][0]["content"][0]["transcript"] = true;
  filter["response"]["output"][0]["content"][0]["text"] = true;
  JsonDocument doc;
  if (deserializeJson(doc, msg, n, DeserializationOption::Filter(filter))) return false;
  JsonObject r = doc["response"];
  if (r.isNull()) return false;
  d->is_card = strcmp(r["metadata"]["k"] | "", "card") == 0;
  snprintf(d->seq, sizeof(d->seq), "%s", r["metadata"]["n"] | "");
  strncpy(d->status, r["status"] | "", sizeof(d->status) - 1); d->status[sizeof(d->status) - 1] = 0;
  d->text[0] = 0;
  size_t used = 0;
  for (JsonObject item : r["output"].as<JsonArray>()) {
    if (strcmp(item["type"] | "", "message") != 0) continue;
    for (JsonObject part : item["content"].as<JsonArray>()) {
      const char *t = part["transcript"] | (const char *)nullptr;
      if (!t) t = part["text"] | (const char *)nullptr;
      if (!t) continue;
      size_t tl = strlen(t);
      if (used + tl + 1 > sizeof(d->text)) tl = sizeof(d->text) - 1 - used;
      memcpy(d->text + used, t, tl); used += tl; d->text[used] = 0;
    }
  }
  return true;
}

// {"p":"…","e":"…"} -> pinyin, english. Tolerates a code fence or words around the JSON.
static inline bool rt_parse_card(const char *text, char *py, size_t pcap, char *en, size_t ecap)
{
  const char *a = strchr(text, '{'), *b = strrchr(text, '}');
  if (!a || !b || b < a) return false;
  JsonDocument doc;
  if (deserializeJson(doc, a, (size_t)(b - a + 1))) return false;
  const char *p = doc["p"] | (const char *)nullptr, *e = doc["e"] | (const char *)nullptr;
  if (!p) return false;
  snprintf(py, pcap, "%s", p);
  snprintf(en, ecap, "%s", e ? e : "");
  return true;
}

static inline bool rt_error_message(const char *msg, size_t n, char *out, size_t cap)
{
  JsonDocument doc;
  if (deserializeJson(doc, msg, n)) return false;
  JsonObject e = doc["error"];
  if (e.isNull()) return false;
  const char *code = e["code"] | (e["type"] | "error");
  snprintf(out, cap, "%s: %s", code, e["message"] | "");
  return true;
}

// any top-level string field (for the serial log: what the cloud heard you say)
static inline bool rt_json_string(const char *msg, size_t n, const char *key, char *out, size_t cap)
{
  JsonDocument filter; filter[key] = true;
  JsonDocument doc;
  if (deserializeJson(doc, msg, n, DeserializationOption::Filter(filter))) return false;
  const char *v = doc[key] | (const char *)nullptr;
  if (!v) return false;
  snprintf(out, cap, "%s", v);
  return true;
}
