#include "settings.h"
#include <Preferences.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>

GawaSettings settings;

static const char *DEFAULT_MODEL_CHAT = "gpt-5.4-nano";
static const char *DEFAULT_MODEL_STT  = "gpt-transcribe";

void settings_load()
{
  Preferences p;
  p.begin("gawa", true);
  settings.wifi_ssid  = p.getString("ssid", "");
  settings.wifi_pass  = p.getString("pass", "");
  settings.api_key    = p.getString("key", "");
  settings.model_chat = p.getString("mchat", DEFAULT_MODEL_CHAT);
  settings.model_stt  = p.getString("mstt", DEFAULT_MODEL_STT);
  p.end();
  if (settings.model_chat.length() == 0) settings.model_chat = DEFAULT_MODEL_CHAT;
  if (settings.model_stt.length() == 0)  settings.model_stt  = DEFAULT_MODEL_STT;
}

void settings_save()
{
  Preferences p;
  p.begin("gawa", false);
  p.putString("ssid", settings.wifi_ssid);
  p.putString("pass", settings.wifi_pass);
  p.putString("key", settings.api_key);
  p.putString("mchat", settings.model_chat);
  p.putString("mstt", settings.model_stt);
  p.end();
}

// ---------------------------------------------------------------- setup web page
static const char PAGE_HEAD[] PROGMEM = R"HTML(<!doctype html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1"><title>Gawa setup</title>
<style>body{font-family:-apple-system,Helvetica,Arial,sans-serif;max-width:420px;margin:24px auto;padding:0 16px;color:#222}
h1{font-size:26px}label{display:block;margin:14px 0 4px;font-weight:600}input{width:100%;padding:10px;font-size:16px;border:1px solid #bbb;border-radius:8px;box-sizing:border-box}
button{margin-top:20px;width:100%;padding:14px;font-size:18px;border:0;border-radius:10px;background:#222;color:#fff}small{color:#666}
.ok{background:#e8f7e8;border:1px solid #9c9;padding:12px;border-radius:8px}</style></head><body>)HTML";

static String html_escape(const String &s)
{
  String o; o.reserve(s.length() + 8);
  for (char c : s) {
    if (c == '&') o += "&amp;"; else if (c == '<') o += "&lt;"; else if (c == '>') o += "&gt;";
    else if (c == '"') o += "&quot;"; else o += c;
  }
  return o;
}

static String page_form()
{
  String h = FPSTR(PAGE_HEAD);
  h += "<h1>Hi, I am Gawa</h1><p>Tell me how to get online. I keep this inside me and never show it again.</p>";
  h += "<form method='POST' action='/save'>";
  h += "<label>Home Wi-Fi name</label><input name='ssid' value='" + html_escape(settings.wifi_ssid) + "' autocapitalize='none'>";
  h += "<label>Wi-Fi password</label><input name='pass' type='password' value='" + html_escape(settings.wifi_pass) + "'>";
  h += "<label>OpenAI key <small>(starts with sk-)</small></label><input name='key' value='" + html_escape(settings.api_key) + "' autocapitalize='none' autocorrect='off'>";
  h += "<label>Brain model <small>(leave as is unless told)</small></label><input name='mchat' value='" + html_escape(settings.model_chat) + "'>";
  h += "<label>Hearing model</label><input name='mstt' value='" + html_escape(settings.model_stt) + "'>";
  h += "<button type='submit'>Save and restart Gawa</button></form></body></html>";
  return h;
}

void setup_portal_run(void (*on_status)(const char *, const char *))
{
  WiFi.mode(WIFI_AP);
  WiFi.softAP("Gawa-Setup");
  delay(200);
  IPAddress ip = WiFi.softAPIP();

  DNSServer dns;
  dns.start(53, "*", ip);          // every web address points at us -> phone pops the page up
  WebServer web(80);

  bool saved = false;
  web.on("/", HTTP_GET, [&]() { web.send(200, "text/html", page_form()); });
  web.on("/save", HTTP_POST, [&]() {
    settings.wifi_ssid  = web.arg("ssid");  settings.wifi_ssid.trim();
    settings.wifi_pass  = web.arg("pass");
    settings.api_key    = web.arg("key");   settings.api_key.trim();
    settings.model_chat = web.arg("mchat"); settings.model_chat.trim();
    settings.model_stt  = web.arg("mstt");  settings.model_stt.trim();
    settings_save();
    String h = FPSTR(PAGE_HEAD);
    h += "<div class='ok'><h1>Saved!</h1><p>I am restarting now. You can leave this Wi-Fi.</p></div></body></html>";
    web.send(200, "text/html", h);
    saved = true;
  });
  web.onNotFound([&]() {                 // captive-portal probes from phones land here
    web.sendHeader("Location", String("http://") + ip.toString() + "/", true);
    web.send(302, "text/plain", "");
  });
  web.begin();

  on_status("JOIN WIFI", "GAWA-SETUP");
  Serial.printf("Setup page: join Wi-Fi 'Gawa-Setup' then open http://%s/\n", ip.toString().c_str());

  uint32_t t0 = millis();
  while (!saved) {
    dns.processNextRequest();
    web.handleClient();
    delay(2);
    if (millis() - t0 > 10UL * 60UL * 1000UL) break;   // give up after 10 minutes
  }
  delay(1500);
  ESP.restart();
}
