// test_rt_proto.cpp - checks the OpenAI Realtime messages Pengu builds and reads.
// g++ -std=c++17 -I../../firmware/PenguCN -I$HOME/Arduino/libraries/ArduinoJson/src test_rt_proto.cpp -o test_rt_proto
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "rt_proto.h"

static int fails = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)
static bool has(const char *hay, const char *needle) { return strstr(hay, needle) != nullptr; }

int main()
{
  // ---- base64 round trip (and the odd lengths)
  for (int n = 0; n < 10; n++) {
    uint8_t in[16]; for (int i = 0; i < n; i++) in[i] = (uint8_t)(i * 37 + 5);
    char enc[64]; size_t el = rt_b64_encode(in, n, enc);
    CHECK(el == (size_t)((n + 2) / 3 * 4));
    CHECK(enc[el] == 0);
    int dl = rt_b64_decode_inplace(enc, el);
    CHECK(dl == n);
    CHECK(memcmp(enc, in, n) == 0);
  }
  char known[] = "aGVsbG8=";
  CHECK(rt_b64_decode_inplace(known, 8) == 5 && memcmp(known, "hello", 5) == 0);
  char bad[] = "a$b=";
  CHECK(rt_b64_decode_inplace(bad, 4) < 0);

  // ---- session.update
  char buf[8192];
  RtSessionCfg cfg; cfg.instructions = "You are Pengu. 说中文。"; cfg.voice = "marin"; cfg.silence_ms = 700;
  size_t n = rt_session_update(buf, sizeof(buf), cfg);
  CHECK(n > 0 && n < sizeof(buf));
  CHECK(has(buf, "\"type\":\"session.update\""));
  CHECK(has(buf, "\"type\":\"realtime\""));
  CHECK(has(buf, "\"audio/pcm\""));
  CHECK(has(buf, "\"rate\":24000"));
  CHECK(has(buf, "\"voice\":\"marin\""));
  CHECK(has(buf, "\"server_vad\""));
  CHECK(has(buf, "\"silence_duration_ms\":700"));
  CHECK(has(buf, "\"create_response\":true"));
  CHECK(has(buf, "\"interrupt_response\":false"));
  CHECK(has(buf, "说中文"));
  CHECK(has(buf, "\"transcription\""));

  // ---- the greeting and the card request
  n = rt_response_create(buf, sizeof(buf), "Say hello.");
  CHECK(n > 0 && has(buf, "\"type\":\"response.create\"") && has(buf, "Say hello."));
  n = rt_card_request(buf, sizeof(buf), "你好\"！", 7);
  CHECK(n > 0);
  CHECK(has(buf, "\"conversation\":\"none\""));
  CHECK(has(buf, "\"output_modalities\":[\"text\"]"));
  CHECK(has(buf, "\"k\":\"card\""));
  CHECK(has(buf, "\"n\":\"7\""));
  CHECK(has(buf, "你好\\\"！"));                     // the quote inside is escaped

  // ---- audio append
  int16_t pcm[4] = { 0, 1, -1, 32767 };
  n = rt_audio_append(pcm, 4, buf, sizeof(buf));
  CHECK(n > 0 && n == strlen(buf));
  CHECK(has(buf, "{\"type\":\"input_audio_buffer.append\",\"audio\":\""));
  CHECK(buf[n - 1] == '}' && buf[n - 2] == '"');
  CHECK(rt_audio_append(pcm, 4, buf, 20) == 0);     // too small: refuses, does not overflow

  // ---- event types
  const char *e1 = "{\"type\":\"response.output_audio.delta\",\"event_id\":\"e1\",\"delta\":\"AAAB\"}";
  const char *e2 = "{\"event_id\":\"e2\",\"type\":\"session.created\",\"session\":{\"type\":\"realtime\"}}";
  const char *e3 = "{\"type\": \"input_audio_buffer.speech_started\"}";
  CHECK(rt_event_type(e1, strlen(e1)) == RT_AUDIO_DELTA);
  CHECK(rt_event_type(e2, strlen(e2)) == RT_SESSION_CREATED);
  CHECK(rt_event_type(e3, strlen(e3)) == RT_SPEECH_STARTED);
  CHECK(rt_event_type("{\"type\":\"rate_limits.updated\"}", 29) == RT_OTHER);
  CHECK(rt_event_type("{}", 2) == RT_OTHER);

  // ---- audio delta decoded in place (two samples: 1 and -2)
  uint8_t raw[4] = { 1, 0, 0xFE, 0xFF }; char b64[16]; rt_b64_encode(raw, 4, b64);
  std::string msg = std::string("{\"type\":\"response.output_audio.delta\",\"response_id\":\"r\",\"delta\":\"") + b64 + "\"}";
  std::vector<char> m(msg.begin(), msg.end());
  const int16_t *samples = nullptr;
  int ns = rt_audio_delta_decode(m.data(), m.size(), &samples);
  CHECK(ns == 2);
  CHECK(samples && samples[0] == 1 && samples[1] == -2);

  // ---- response.done: an audio reply
  const char *done_audio =
    "{\"type\":\"response.done\",\"event_id\":\"x\",\"response\":{\"id\":\"r1\",\"status\":\"completed\",\"metadata\":null,"
    "\"output\":[{\"id\":\"i1\",\"type\":\"message\",\"role\":\"assistant\",\"content\":[{\"type\":\"output_audio\","
    "\"transcript\":\"你好！我是Pengu！\"}]}]}}";
  RtDone d;
  CHECK(rt_response_done(done_audio, strlen(done_audio), &d));
  CHECK(!d.is_card);
  CHECK(strcmp(d.text, "你好！我是Pengu！") == 0);
  CHECK(strcmp(d.status, "completed") == 0);

  // ---- response.done: the card (out of band, text)
  const char *done_card =
    "{\"type\":\"response.done\",\"response\":{\"status\":\"completed\",\"metadata\":{\"k\":\"card\",\"n\":\"7\"},"
    "\"output\":[{\"type\":\"message\",\"content\":[{\"type\":\"output_text\",\"text\":\"{\\\"p\\\":\\\"nǐ hǎo\\\",\\\"e\\\":\\\"Hello\\\"}\"}]}]}}";
  CHECK(rt_response_done(done_card, strlen(done_card), &d));
  CHECK(d.is_card);
  CHECK(strcmp(d.seq, "7") == 0);
  char py[128], en[128];
  CHECK(rt_parse_card(d.text, py, sizeof(py), en, sizeof(en)));
  CHECK(strcmp(py, "nǐ hǎo") == 0 && strcmp(en, "Hello") == 0);

  // ---- a card wrapped in a code fence still parses
  CHECK(rt_parse_card("```json\n{\"p\": \"chī fàn ba\", \"e\": \"Let's eat\"}\n```", py, sizeof(py), en, sizeof(en)));
  CHECK(strcmp(py, "chī fàn ba") == 0 && strcmp(en, "Let's eat") == 0);
  CHECK(!rt_parse_card("sorry, no", py, sizeof(py), en, sizeof(en)));

  // ---- a cancelled response has no text
  const char *done_cancel = "{\"type\":\"response.done\",\"response\":{\"status\":\"cancelled\",\"output\":[]}}";
  CHECK(rt_response_done(done_cancel, strlen(done_cancel), &d));
  CHECK(d.text[0] == 0 && strcmp(d.status, "cancelled") == 0);

  // ---- errors
  const char *err = "{\"type\":\"error\",\"error\":{\"type\":\"invalid_request_error\",\"code\":\"invalid_value\",\"message\":\"Bad voice\"}}";
  CHECK(rt_event_type(err, strlen(err)) == RT_ERROR);
  char em[64];
  CHECK(rt_error_message(err, strlen(err), em, sizeof(em)));
  CHECK(strcmp(em, "invalid_value: Bad voice") == 0);

  // ---- the user's words (for the serial log)
  const char *heard = "{\"type\":\"conversation.item.input_audio_transcription.completed\",\"transcript\":\"吃饭吧\"}";
  CHECK(rt_event_type(heard, strlen(heard)) == RT_USER_TRANSCRIPT);
  char tr[64];
  CHECK(rt_json_string(heard, strlen(heard), "transcript", tr, sizeof(tr)) && strcmp(tr, "吃饭吧") == 0);

  printf(fails ? "%d FAILED\n" : "all rt_proto tests passed\n", fails);
  return fails ? 1 : 0;
}
