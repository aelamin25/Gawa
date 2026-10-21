// settings.cpp - adapted from the parked v3 cloud brain (extras/cloud-brain/settings.cpp).
#include "settings.h"
#include <Preferences.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>

PenguSettings settings;

static const char *DEFAULT_MODEL = "gpt-realtime-2.1-mini";
static const char *DEFAULT_VOICE = "marin";
static const char *VOICES[] = { "marin", "cedar", "coral", "shimmer", "sage", "ballad", "alloy", "ash", "echo", "verse" };

void settings_load()
{
  Preferences p;
  p.begin("pengu", true);
  settings.wifi_ssid = p.getString("ssid", "");
  settings.wifi_pass = p.getString("pass", "");
  settings.api_key   = p.getString("key", "");
  settings.model     = p.getString("model", DEFAULT_MODEL);
  settings.voice     = p.getString("voice", DEFAULT_VOICE);
  p.end();
  if (settings.model.length() == 0) settings.model = DEFAULT_MODEL;
  if (settings.voice.length() == 0) settings.voice = DEFAULT_VOICE;
}

void settings_save()
{
  Preferences p;
  p.begin("pengu", false);
  p.putString("ssid", settings.wifi_ssid);
  p.putString("pass", settings.wifi_pass);
  p.putString("key", settings.api_key);
  p.putString("model", settings.model);
  p.putString("voice", settings.voice);
  p.end();
}

// ---------------------------------------------------------------- setup web page
static const char PAGE_HEAD[] PROGMEM = R"HTML(<!doctype html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1"><title>Pengu setup</title>
<style>body{font-family:-apple-system,Helvetica,Arial,sans-serif;max-width:420px;margin:24px auto;padding:0 16px;color:#222}
h1{font-size:26px}label{display:block;margin:14px 0 4px;font-weight:600}input,select{width:100%;padding:10px;font-size:16px;border:1px solid #bbb;border-radius:8px;box-sizing:border-box;background:#fff}
button{margin-top:20px;width:100%;padding:14px;font-size:18px;border:0;border-radius:10px;background:#222;color:#fff}small{color:#666}
.ok{background:#e8f7e8;border:1px solid #9c9;padding:12px;border-radius:8px}</style></head><body>)HTML";

static String html_escape(const String &s)
{
  String o; o.reserve(s.length() + 8);
  for (char c : s) {
    if (c == '&') o += "&amp;"; else if (c == '<') o += "&lt;"; else if (c == '>') o += "&gt;";
    else if (c == '"') o += "&quot;"; else if (c == '\'') o += "&#39;"; else o += c;
  }
  return o;
}

static String page_form()
{
  String h = FPSTR(PAGE_HEAD);
  h += "<h1>&#20320;&#22909;! I am Pengu</h1><p>Tell me how to get online. I keep this inside me.</p>";
  h += "<form method='POST' action='/save'>";
  h += "<label>Home Wi-Fi name</label><input name='ssid' value='" + html_escape(settings.wifi_ssid) + "' autocapitalize='none'>";
  h += "<label>Wi-Fi password</label><input name='pass' type='password' value='" + html_escape(settings.wifi_pass) + "'>";
  // the saved key is never sent back to the page: leave the box empty to keep it
  h += String("<label>OpenAI key <small>(starts with sk-") + (settings.has_key() ? "; one is saved, leave empty to keep it" : "") +
       ")</small></label><input name='key' type='password' value='' autocapitalize='none' autocorrect='off'>";
  h += "<label>Pengu's voice <small>(try a few!)</small></label><select name='voice'>";
  for (const char *v : VOICES)
    h += String("<option") + (settings.voice == v ? " selected" : "") + ">" + v + "</option>";
  h += "</select>";
  h += "<label>Voice model <small>(leave as is unless told)</small></label><input name='model' value='" + html_escape(settings.model) + "'>";
  h += "<button type='submit'>Save and restart Pengu</button></form></body></html>";
  return h;
}

void setup_portal_run(void (*on_status)(const char *))
{
  WiFi.disconnect(true);
  WiFi.mode(WIFI_AP);
  WiFi.softAP("Pengu-Setup");
  delay(200);
  IPAddress ip = WiFi.softAPIP();

  DNSServer dns;
  dns.start(53, "*", ip);          // every web address points at us -> the phone pops the page up
  WebServer web(80);

  bool saved = false;
  web.on("/", HTTP_GET, [&]() { web.send(200, "text/html", page_form()); });
  web.on("/save", HTTP_POST, [&]() {
    settings.wifi_ssid = web.arg("ssid");  settings.wifi_ssid.trim();
    settings.wifi_pass = web.arg("pass");
    String key = web.arg("key"); key.trim();
    if (key.length() > 0) settings.api_key = key;     // an empty box keeps the saved key
    settings.voice = web.arg("voice");    settings.voice.trim();
    settings.model = web.arg("model");    settings.model.trim();
    settings_save();
    String h = FPSTR(PAGE_HEAD);
    h += "<div class='ok'><h1>Saved! &#35874;&#35874;!</h1><p>I am restarting now. You can leave this Wi-Fi.</p></div></body></html>";
    web.send(200, "text/html", h);
    saved = true;
  });
  web.onNotFound([&]() {                 // captive-portal probes from phones land here
    web.sendHeader("Location", String("http://") + ip.toString() + "/", true);
    web.send(302, "text/plain", "");
  });
  web.begin();

  on_status("JOIN WIFI PENGU-SETUP");
  Serial.printf("Setup page: join Wi-Fi 'Pengu-Setup' then open http://%s/\n", ip.toString().c_str());

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
