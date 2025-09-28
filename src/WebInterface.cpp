// WebInterface.cpp
#include "WebInterface.h"
#include "Config.h"
#include "Globals.h"
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <pgmspace.h>

namespace WebInterface {
namespace {
    AsyncWebServer server(80);
    AsyncWebSocket ws("/ws");

    // Constants for JSON keys
    const char* JSON_MODE = "mode";
    const char* JSON_LED_INDEX = "ledIndex";
    const char* JSON_BRIGHTNESS = "brightness";
    const char* JSON_FREQ = "freq";
    const char* JSON_DUTY = "duty";
    const char* JSON_HUE = "hue";
    const char* JSON_SAT = "sat";
    const char* JSON_LED_ENABLED = "ledEnabled";

    // HTML page with minified CSS/JS, single block, HSV units (0-360/0-100/0-100)
const char indexHtml[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Lume Load by UG56</title>
<style>
body { font-family: Arial; margin: 8px; color: #eee; background: #111; }
.header { display: flex; align-items: center; margin-bottom: 1em; }
.header h1 { margin: 0 0 0 0.5em; font-size: 1.5em; color: #4CAF50; user-select: none; }
.container { max-width: 40em; margin: auto; padding: 1em; }
.block { background: #222; padding: 1em; border-radius: 8px; margin-bottom: 1em; }
label { display: block; margin-bottom: 4px; }
input[type="range"], select { width: 100%; padding: 4px 0; margin-bottom: 8px; border-radius: 6px; background: #333; color: #eee; }
input[type="range"].hue { background: linear-gradient(to right, red, yellow, lime, cyan, blue, magenta, red); }
button { width: 100%; padding: 16px; border: 0; border-radius: 6px; background: #555; color: #022; font-size: 1.1em; box-sizing: border-box; }
button.active { background: #7b2; color: #111; }
</style>
</head>
<body>

<div class="header-container" style="max-width:40em; margin:auto; padding:0.5em 0; background:transparent;">
  <div style="display:flex; align-items:center;">
    <svg id="logo" viewBox="0 0 24 24" aria-label="Lume Load Logo" role="img"
         style="width:32px; height:32px; cursor:pointer; color:#4CAF50; margin-right:0.5em;">
      <defs>
        <filter id="glow" x="-50%" y="-50%" width="200%" height="200%">
          <feGaussianBlur stdDeviation="1" result="blur"/>
          <feMerge>
            <feMergeNode in="blur"/>
            <feMergeNode in="SourceGraphic"/>
          </feMerge>
        </filter>
      </defs>
      <g filter="url(#glow)">
        <line x1="12" y1="4" x2="12" y2="10.5" class="line"/>
        <path d="M16.24 7a7 7 0 1 1-8.48 0" class="circle"/>
      </g>
      <style>
        .line, .circle { stroke: currentColor; stroke-width: 2; fill: none; stroke-dasharray:100; stroke-dashoffset:100; opacity:0; animation-timing-function:ease-out; animation-fill-mode:forwards; }
        .animate .circle { animation: drawFade 4.8s 0s backwards; }
        .animate .line { animation: drawFade 4s 0.4s backwards; }
        @keyframes drawFade { 0% {stroke-dashoffset:100; opacity:0;} 20% {opacity:1;} 70% {stroke-dashoffset:0; opacity:1;} 100% {opacity:0;} }
      </style>
    </svg>
    <span style="font-size:1.1em; color:#4CAF50; font-weight:bold;">Lume Load by UG56</span>
  </div>
</div>
<div class="container">
  <div class="block">
    <label for="modeSelect">Mode</label>
    <select id="modeSelect">
      <option value="1">Static</option>
      <option value="2">Sine</option>
      <option value="3">Rect</option>
      <option value="4">Saw</option>
      <option value="5">Triangle</option>
    </select>

    <label>LEDs up to <span id="ledIndexVal"></span></label>
    <input id="ledIndexRange" type="range" min="0" max="60" value="0">

    <label>Frequency (Hz) <span id="freqVal"></span></label>
    <input id="freqRange" type="range" min="0.1" max="2" step="0.1" value="0.5">

    <label>Duty (%) <span id="dutyVal"></span></label>
    <input id="dutyRange" type="range" min="0" max="100" value="50">

    <label>Hue (&deg;) <span id="hueVal"></span></label>
    <input id="hueRange" type="range" min="0" max="360" value="0" class="hue">

    <label>Saturation (%) <span id="satVal"></span></label>
    <input id="satRange" type="range" min="0" max="100" value="100">

    <label>Brightness (%) <span id="brightnessVal"></span></label>
    <input id="brightnessRange" type="range" min="0" max="100" value="50">

    <button id="btnPower">
      <svg viewBox="0 0 24 24" width="24" height="24">
        <line x1="12" y1="4" x2="12" y2="10.5" stroke="#111" stroke-width="2"/>
        <path d="M16.24 7a7 7 0 1 1-8.48 0" stroke="#111" stroke-width="2" fill="none"/>
      </svg>
    </button>
  </div>
</div>

<script>
let ws, state = {}, retryDelay = 1000, lastSend = 0;

function hsvToRgb(h, s, v) {
  h = h / 60; s = s / 100; v = v / 100;
  let i = Math.floor(h), f = h - i, p = v * (1 - s), q = v * (1 - s * f), t = v * (1 - s * (1 - f));
  switch (i % 6) {
    case 0: return [v, t, p];
    case 1: return [q, v, p];
    case 2: return [p, v, t];
    case 3: return [p, q, v];
    case 4: return [t, p, v];
    case 5: return [v, p, q];
  }
  return [0,0,0];
}

function connectWS() {
  ws = new WebSocket(`ws://${location.host}/ws`);
  ws.onopen = () => { retryDelay = 1000; };
  ws.onclose = () => setTimeout(connectWS, retryDelay = Math.min(retryDelay * 2, 10000));
  ws.onmessage = e => {
    try { state = Object.assign(state, JSON.parse(e.data)); updateUI(); } catch(err){}
  };
}
connectWS();

function sendState(key,value) {
  const now = Date.now();
  if(now-lastSend<300||ws?.readyState!==WebSocket.OPEN) return;
  lastSend = now;
  ws.send(JSON.stringify({[key]:value}));
}

function bind(id,key,parseFn=x=>x){
  const el=document.getElementById(id);
  el.oninput = ()=>{ state[key]=parseFn(el.value); updateUI(); };
  el.onchange = ()=>{ state[key]=parseFn(el.value); sendState(key,state[key]); };
}

function updateUI() {
  const fields=['ledIndex','freq','duty','hue','sat','brightness'];
  fields.forEach(k=>{
    const el=document.getElementById(k+'Val');
    if(el) el.textContent=state[k]??'';
  });
  document.getElementById('modeSelect').value = state.mode ?? 1;
  document.getElementById('ledIndexRange').value = state.ledIndex ?? 0;
  document.getElementById('brightnessRange').value = state.brightness ?? 50;
  document.getElementById('freqRange').value = state.freq ?? 0.5;
  document.getElementById('dutyRange').value = state.duty ?? 50;
  document.getElementById('hueRange').value = state.hue ?? 0;
  document.getElementById('satRange').value = state.sat ?? 100;
  document.getElementById('btnPower').classList.toggle('active', state.ledEnabled ?? false);
  if(state.ledEnabled && state.hue!=null && state.sat!=null && state.brightness!=null){
    const [r,g,b]=hsvToRgb(state.hue,state.sat,state.brightness);
    document.getElementById('btnPower').style.background=`rgb(${Math.round(r*255)},${Math.round(g*255)},${Math.round(b*255)})`;
  } else { document.getElementById('btnPower').style.background='#555'; }
}

bind('modeSelect','mode',v=>parseInt(v));
bind('ledIndexRange','ledIndex',v=>parseInt(v));
bind('brightnessRange','brightness',v=>parseInt(v));
bind('freqRange','freq',v=>parseFloat(v));
bind('dutyRange','duty',v=>parseInt(v));
bind('hueRange','hue',v=>parseInt(v));
bind('satRange','sat',v=>parseInt(v));

document.getElementById('btnPower').onclick=()=>{
  state.ledEnabled=!state.ledEnabled;
  sendState('ledEnabled',state.ledEnabled);
  updateUI();
};

(function(){
  const logo=document.getElementById('logo');
  if(!logo) return;
  logo.classList.add('animate');
  logo.addEventListener('mouseenter',()=>{
    logo.classList.remove('animate');
    void logo.getBoundingClientRect();
    setTimeout(()=>logo.classList.add('animate'),20);
  });
})();
</script>
</body>
</html>
)rawliteral";

    StaticJsonDocument<256> doc; // Static to reduce heap fragmentation

    void broadcastState() {
        doc.clear();
        doc[JSON_MODE] = static_cast<uint8_t>(uiMode);
        doc[JSON_LED_INDEX] = ledUpTo;
        doc[JSON_BRIGHTNESS] = round(brightness * 100.0 / 255.0); // 0-255 to 0-100
        doc[JSON_FREQ] = freq;
        doc[JSON_DUTY] = duty;
        doc[JSON_HUE] = round(hue * 360.0 / 255.0); // 0-255 to 0-360
        doc[JSON_SAT] = round(sat * 100.0 / 255.0); // 0-255 to 0-100
        doc[JSON_LED_ENABLED] = ledEnabled;

        String out;
        serializeJson(doc, out);
        ws.textAll(out);
    }

    void handleWsEvent(AsyncWebSocket* /*server*/, AsyncWebSocketClient* /*client*/, AwsEventType type,
                       void* /*arg*/, uint8_t* data, size_t len) {
        if (type == WS_EVT_CONNECT) {
            broadcastState();
            return;
        }

        if (type == WS_EVT_DATA) {
            String msg;
            msg.reserve(len);
            for (size_t i = 0; i < len; ++i) {
                msg += static_cast<char>(data[i]);
            }

            DeserializationError err = deserializeJson(doc, msg);
            if (err) return;

            // Update globals with bounds checking and unit conversion
            if (doc.containsKey(JSON_MODE)) {
                uint8_t mode = doc[JSON_MODE].as<uint8_t>();
                if (mode >= static_cast<uint8_t>(::UIMode::STATIC) && mode <= static_cast<uint8_t>(::UIMode::TRIANGLE)) {
                    uiMode = static_cast<::UIMode>(mode);
                }
            }
            if (doc.containsKey(JSON_LED_INDEX)) ledUpTo = constrain(doc[JSON_LED_INDEX].as<int>(), 0, NUMPIXELS);
            if (doc.containsKey(JSON_BRIGHTNESS)) brightness = constrain(round(doc[JSON_BRIGHTNESS].as<double>() * 255.0 / 100.0), 0, 255);
            if (doc.containsKey(JSON_FREQ)) freq = constrain(doc[JSON_FREQ].as<float>(), 0.1f, 2.0f);
            if (doc.containsKey(JSON_DUTY)) duty = constrain(doc[JSON_DUTY].as<int>(), 0, 100);
            if (doc.containsKey(JSON_HUE)) hue = constrain(round(doc[JSON_HUE].as<double>() * 255.0 / 360.0), 0, 255);
            if (doc.containsKey(JSON_SAT)) sat = constrain(round(doc[JSON_SAT].as<double>() * 255.0 / 100.0), 0, 255);
            if (doc.containsKey(JSON_LED_ENABLED)) ledEnabled = doc[JSON_LED_ENABLED].as<bool>();

            broadcastState();
        }
    }
} // anonymous namespace

void begin() {
    ws.onEvent(handleWsEvent);
    server.addHandler(&ws);
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send_P(200, "text/html", indexHtml);
    });
    server.begin();
}

void update() {
    ws.cleanupClients();
}
} // namespace WebInterface

