#define VERSION "dev"

#include <ArduinoJson.h>
#include <EEPROM.h>
#include <OneButton.h>          // https://github.com/mathertel/OneButton
#include <WS2812FX.h>           // https://github.com/kitesurfer1404/WS2812FX
#include <ESPAsyncWebServer.h>  // https://github.com/me-no-dev/ESPAsyncWebServer
#include <WiFi.h>
#include <ESPmDNS.h>
#include <Update.h>

/* Put your HOST SSID & Password */
const char *hostname = "CubeFX";
const char *ssid = "ZimaCube";       // Enter SSID here
const char *password = "homecloud";  // Enter Password here

/* Put IP Address details */
IPAddress local_ip(172, 16, 1, 1);
IPAddress gateway(172, 16, 1, 1);
IPAddress subnet(255, 255, 255, 0);

/* Put HTTP details */
const uint8_t http_port = 80;

AsyncWebServer httpServer(http_port);

static const char htmlIndex[] PROGMEM = R"(<!DOCTYPE html>
<html>
<head>
<title>CubeFX</title>
<meta charset='utf-8'>
<meta name='viewport' content='width=device-width,initial-scale=1'/>
</head>
<body>
<h1>CubeFX )" VERSION R"(</h1>
<p>A better ZimaCube light strip system.</p>
<ul>
<li><a href='/post'>Light Panel</a></li>
<li><a href='/get'>Light Status</a></li>
<li><a href='/light/switch'>Light Switch</a></li>
<li><a href='/light/mode'>Light NextDemo</a></li>
<li><a href='/wifi/off'>WiFiAP TurnOff</a></li>
<li><a href='/update'>Firmware Update</a></li>
</ul>
<i>Made with ❤️ by Cp0204</i>
</body>
</html>)";

static const char htmlUpdate[] PROGMEM = R"(<!DOCTYPE html>
<html>
<head>
<title>Update</title>
<meta charset='utf-8'>
<meta name='viewport' content='width=device-width,initial-scale=1'/>
</head>
<body>
<form method='POST' action='' enctype='multipart/form-data'>
Firmware: <input type='file' accept='.bin,.bin.gz' name='firmware'> <input type='submit' value='Update'>
</form>
</body>
</html>)";

static const char htmlPanel[] PROGMEM = R"(
<!DOCTYPE html><html><head><title>CubeFX Panel</title><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1' /><style>.colorPicker{ height: 25px; width: 25px; margin: 2px; cursor: pointer;} </style>
</head><body><h1>CubeFX Panel</h1><form id="ledForm"><p>on:&nbsp;<input type="checkbox" id="onCheck"></p><p>id:&nbsp;<select id="idSel"></select>&nbsp;<button type="button" id="btnNext">QuickNext</button></p>
<p>speed:&nbsp;<input type="range" id="speedRng" min="0" max="255" value="150">&nbsp;<span id="speedVal">150</span></p><p>lightness:&nbsp;<input type="range" id="lightRng" min="0" max="255" value="255">&nbsp;<span id="lightVal">255</span></p>
<p>data:&nbsp;<span id="colorCtr"></span></p><button type="submit">Submit</button></form><pre id="log"></pre><i>Made with ❤️ by Cp0204</i><script>const form=document.getElementById('ledForm'), onCheck=document.getElementById('onCheck'),
idSel=document.getElementById('idSel'), btnNext=document.getElementById('btnNext'), speedRng=document.getElementById('speedRng'), speedVal=document.getElementById('speedVal'), lightRng=document.getElementById('lightRng'), lightVal=document.getElementById('lightVal'),
colorCtr=document.getElementById('colorCtr'), log=document.getElementById('log'), apiServer='http://172.16.1.1'; for (let i=0; i < 13; i++){ const picker=document.createElement('input'); picker.type='color'; picker.classList.add('colorPicker'); picker.value='#FFFFFF';
colorCtr.appendChild(picker);} for (let i=5; i >=-71; i--){ const opt=document.createElement('option'); opt.text=i; opt.selected=i===5; idSel.appendChild(opt);} speedRng.addEventListener('input', ()=>speedVal.textContent=speedRng.value);
lightRng.addEventListener('input', ()=>lightVal.textContent=lightRng.value); btnNext.addEventListener('click', ()=>{ let currentId=parseInt(idSel.value); idSel.value=currentId==5 ? -71 : currentId + 1; form.dispatchEvent(new Event('submit'));});
form.addEventListener('submit', (event)=>{ event.preventDefault(); const data={ on: onCheck.checked ? 1 : 0, id: parseInt(idSel.value), speed: parseInt(speedRng.value), lightness: parseInt(lightRng.value),
data: Array.from(document.querySelectorAll('.colorPicker')).map(picker=>picker.value.replace('#', ''))}; fetch(apiServer + '/post',{ method: 'POST', headers:{ 'Content-Type': 'application/json'},
body: JSON.stringify(data)}) .then(response=>response.json()) .then(response=>log.textContent +="\n/post Success " + JSON.stringify(response)) .catch(err=>log.textContent +='\nError: ' + err);});
window.addEventListener('load', ()=>{ fetch(apiServer + '/get') .then(response=>response.json()) .then(data=>{ onCheck.checked=data.on===1; idSel.value=data.id; speedRng.value=data.speed; speedVal.textContent=data.speed; lightRng.value=data.lightness;
lightVal.textContent=data.lightness; const colorPickers=document.querySelectorAll('.colorPicker'); for (let i=0; i < data.data.length; i++){ colorPickers[i].value='#' + data.data[i];} log.textContent +='/get Success';}) .catch(err=>{ log.textContent +='Error: ' + err; form.querySelectorAll('button').forEach(button=>{ button.disabled=true;});});}); </script></body></html>
)";

DynamicJsonDocument doc(1024);

// WS2812 设置
#define LED_PIN 2
#define NUM_PIXELS 13
WS2812FX ws2812fx = WS2812FX(NUM_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);  // Just used to calculate compatible colors

// 板载LED
#define BOARD_LED 8

// 按键设置
#define BTN_PIN 9
OneButton button(BTN_PIN, true);

// 默认参数
bool isLightOn = true;
int8_t effectId = 5;
uint8_t effectSpeed = 128;
uint8_t lightness = 255;
uint32_t colors[NUM_PIXELS] = {
  0xFF0000,  // 红色
  0xFF8000,  // 橙色
  0xFFFF00,  // 黄色
  0x80FF00,  // 黄绿色
  0x00FF00,  // 绿色
  0x00FF80,  // 青绿色
  0x00FFFF,  // 青色
  0x0080FF,  // 蓝绿色
  0x0000FF,  // 蓝色
  0x8000FF,  // 紫色
  0xFF00FF,  // 粉红色
  0xFF0080,  // 玫瑰红色
  0xFF0000   // 红色
};

// EEPROM 操作
#define EEPROM_ADDR_IS_LIGHT_ON 0
#define EEPROM_ADDR_EFFECT_ID 1
#define EEPROM_ADDR_EFFECT_SPEED 2
#define EEPROM_ADDR_LIGHTNESS 3
#define EEPROM_ADDR_COLORS 4

void loadFromEEPROM() {
  EEPROM.begin(EEPROM_ADDR_COLORS + NUM_PIXELS * sizeof(uint32_t));
  if (EEPROM.read(EEPROM_ADDR_EFFECT_ID) == 0 || EEPROM.read(EEPROM_ADDR_EFFECT_ID) == 255) {
    // 可能是首次写入本固件
    saveToEEPROM();
  } else {
    EEPROM.get(EEPROM_ADDR_IS_LIGHT_ON, isLightOn);
    EEPROM.get(EEPROM_ADDR_EFFECT_ID, effectId);
    EEPROM.get(EEPROM_ADDR_EFFECT_SPEED, effectSpeed);
    EEPROM.get(EEPROM_ADDR_LIGHTNESS, lightness);
    EEPROM.get(EEPROM_ADDR_COLORS, colors);
  }
}

void saveToEEPROM() {
  EEPROM.put(EEPROM_ADDR_IS_LIGHT_ON, isLightOn);
  EEPROM.put(EEPROM_ADDR_EFFECT_ID, effectId);
  EEPROM.put(EEPROM_ADDR_EFFECT_SPEED, effectSpeed);
  EEPROM.put(EEPROM_ADDR_LIGHTNESS, lightness);
  EEPROM.put(EEPROM_ADDR_COLORS, colors);
  EEPROM.commit();
}

void openAP() {
  WiFi.setHostname(hostname);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  WiFi.softAP(ssid, password);
}

void setupMDNS() {
  MDNS.begin(hostname);
  MDNS.addService("http", "tcp", http_port);
  MDNS.addServiceTxt("_http", "_tcp", "board", "ESP32");
}

void openHttpServer() {
  // OTA
  setupOTA("/update");
  // index
  httpServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", htmlIndex);
  });
  // light post
  httpServer.on(
    "/post", HTTP_POST, [](AsyncWebServerRequest *request) {
      //nothing and dont remove it
    },
    NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      handleHttpPost(request, data);
    });
  // handle preflight
  httpServer.on("/post", HTTP_OPTIONS, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(204);
    response->addHeader("Access-Control-Allow-Origin", "*");
    response->addHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
    response->addHeader("Access-Control-Allow-Headers", "*");
    request->send(response);
  });
  // light show panel
  httpServer.on("/post", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", htmlPanel);
  });
  // light get
  httpServer.on("/get", HTTP_GET, [](AsyncWebServerRequest *request) {
    handleHttpGet(request);
  });
  // version
  httpServer.on("/version", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "application/json", "{\"version\":\"" + String(VERSION) + "\"}");
  });
  // wifi off
  httpServer.on("/wifi/off", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Turning off WiFi... Press the redlight side Button or Re-insert to Reopen");
    delay(1000);
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
  });
  // light switch
  httpServer.on("/light/switch", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (ws2812fx.isRunning()) {
      request->send(200, "text/plain", "Switch Light... Now OFF");
      isLightOn = false;
      ws2812fx.stop();
    } else {
      request->send(200, "text/plain", "Switch Light... Now ON");
      isLightOn = true;
      ws2812fx.start();
    }
    EEPROM.put(EEPROM_ADDR_IS_LIGHT_ON, isLightOn);
    EEPROM.commit();
  });
  // light mode
  httpServer.on("/light/mode", HTTP_GET, [](AsyncWebServerRequest *request) {
    effectId = effectId == 5 ? -71 : (effectId + 1);
    showEffect();
    request->send(200, "text/plain", "Light Next Mode... " + String(effectId) + ":" + ws2812fx.getModeName(ws2812fx.getMode()));
  });
  // 404
  httpServer.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
  });
  httpServer.begin();
  Serial.printf("HTTPServer Started: http://%s/update\n", WiFi.softAPIP().toString());
}

// 颜色兼容 rgb hsv hex 输入
// https://colorpicker.me/
uint32_t compatibleColor(JsonVariant item) {
  if (item.is<JsonObject>()) {
    JsonObject color = item.as<JsonObject>();
    if (color.containsKey("r")) {
      uint8_t r = constrain(color["r"], 0, 255);
      uint8_t g = constrain(color["g"], 0, 255);
      uint8_t b = constrain(color["b"], 0, 255);
      return (uint32_t(r) << 16) | (uint32_t(g) << 8) | uint32_t(b);
    } else if (color.containsKey("h")) {
      uint16_t h = map(color["h"], 0, 360, 0, 65537);
      uint8_t s = map(color["s"], 0, 100, 0, 255);
      uint8_t v = map(color["v"], 0, 100, 0, 255);
      return strip.gamma32(strip.ColorHSV(h, s, v));
    }
  } else if (item.is<String>()) {
    String hexColor = item.as<String>();
    if (hexColor.length() == 6) {
      return strtol(hexColor.c_str(), nullptr, 16);
    }
  }
  return 0;
}

void showEffect() {
  if (!isLightOn) {
    ws2812fx.stop();
  } else if (!ws2812fx.isRunning()) {
    ws2812fx.start();
  }
  ws2812fx.setSpeed(map(255 - effectSpeed, 0, 255, 0, 5000));  // 在 ws2812fx 速度代表帧 ms 延迟，越大越慢
  ws2812fx.setBrightness(constrain(lightness + 1, 0, 255));    // ws2812fx的不明bug，当亮度=0时似乎=255
  ws2812fx.setColor(colors[0]);
  switch (effectId) {
    case 0:
      ws2812fx.setMode((ws2812fx.getMode() + 1) % ws2812fx.getModeCount());
      break;
    case 1:
      ws2812fx.setMode(2);
      break;
    case 2:
      ws2812fx.setMode(0);
      break;
    case 3:
      ws2812fx.setMode(72);
      // ws2812fx.setMode(FX_MODE_CUSTOM);
      // ws2812fx.setCustomMode(waterDropEffect);
      break;
    case 4:
      ws2812fx.setMode(73);
      // ws2812fx.setMode(FX_MODE_CUSTOM);
      // ws2812fx.setCustomMode(starEffect);
      break;
    case 5:
      ws2812fx.setMode(74);
      // ws2812fx.setMode(FX_MODE_CUSTOM);
      // ws2812fx.setCustomMode(customShow);
      break;
    default:
      if (effectId < 0) {
        ws2812fx.setMode(-effectId);
      }
      break;
  }
}

void handleHttpPost(AsyncWebServerRequest *request, uint8_t *data) {
  String requestBody = (const char *)data;
  DeserializationError error = deserializeJson(doc, requestBody);
  if (error) {
    Serial.print("Failed to parse JSON: ");
    Serial.println(error.c_str());
    request->send(400, "text/plain", "Bad Request");
  } else {
    isLightOn = doc.containsKey("on") ? (doc["on"] == 0 ? 0 : 1) : 1;
    effectId = doc.containsKey("id") ? doc["id"] : effectId;
    effectSpeed = doc.containsKey("speed") ? constrain(doc["speed"], 0, 255) : effectSpeed;
    lightness = doc.containsKey("lightness") ? constrain(doc["lightness"], 0, 255) : lightness;
    if (doc.containsKey("data")) {
      JsonArray data = doc["data"];
      for (int i = 0; i < NUM_PIXELS; i++) {
        colors[i] = compatibleColor(data[i]);
      }
    }
    showEffect();
    saveToEEPROM();
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", "{\"response\":\"Received\",\"id\":" + String(effectId) + "}");
    response->addHeader("Access-Control-Allow-Origin", "*");
    response->addHeader("Access-Control-Allow-Headers", "*");
    response->addHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
    request->send(response);
  }
}

void handleHttpGet(AsyncWebServerRequest *request) {
  DynamicJsonDocument doc(1024);
  doc["on"] = isLightOn ? 1 : 0;
  doc["id"] = effectId;
  doc["speed"] = effectSpeed;
  doc["lightness"] = lightness;
  JsonArray data = doc.createNestedArray("data");
  for (int i = 0; i < NUM_PIXELS; i++) {
    // JsonObject color = data.createNestedObject();
    // color["r"] = (colors[i] >> 16) & 0xFF;
    // color["g"] = (colors[i] >> 8) & 0xFF;
    // color["b"] = colors[i] & 0xFF;
    char colorString[7];
    sprintf(colorString, "%06X", colors[i]);
    data.add(colorString);
  }
  String jsonResponse;
  serializeJson(doc, jsonResponse);
  AsyncWebServerResponse *response = request->beginResponse(200, "application/json", jsonResponse);
  response->addHeader("Access-Control-Allow-Origin", "*");
  response->addHeader("Access-Control-Allow-Headers", "*");
  response->addHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
  request->send(response);
}

void handleBtnClick() {
  digitalWrite(BOARD_LED, LOW);
  delay(50);
  digitalWrite(BOARD_LED, HIGH);
  openAP();
}

// ==============================================================

void setup() {
  // Init IO
  pinMode(BOARD_LED, OUTPUT);
  digitalWrite(BOARD_LED, HIGH);
  // Output startup parameter
  Serial.begin(15200);
  Serial.println("Initial CubeFX");
  Serial.println("==============================");
  loadFromEEPROM();
  Serial.printf("isLightOn: %d\n", isLightOn);
  Serial.printf("effectId: %d\n", effectId);
  Serial.printf("effectSpeed: %d\n", effectSpeed);
  Serial.printf("lightness: %d\n", lightness);
  Serial.print("colors: ");
  for (int i = 0; i < NUM_PIXELS; i++) {
    Serial.printf("%06X%s", colors[i], (i < NUM_PIXELS - 1) ? ", " : "\n");
  }
  Serial.println("==============================");
  // WS2812FX
  ws2812fx.init();
  // 自定义效果，从 0:72 开始注册
  ws2812fx.setCustomMode(0, F("waterDropEffect"), waterDropEffect);  // 72
  ws2812fx.setCustomMode(1, F("starEffect"), starEffect);            // 73
  ws2812fx.setCustomMode(2, F("customShow"), customShow);            // 74
  showEffect();
  // OneButton
  button.attachClick(handleBtnClick);
  // WebServer
  openAP();
  openHttpServer();
  setupMDNS();
}

void loop() {
  ws2812fx.service();
  button.tick();
}

// ==============================================================
// ===== 自定义效果 =====

// 水滴效果，从中间扩散
uint16_t waterDropEffect(void) {
  WS2812FX::Segment *seg = ws2812fx.getSegment();
  WS2812FX::Segment_runtime *segrt = ws2812fx.getSegmentRuntime();
  int seglen = seg->stop - seg->start + 1;
  int middle = seg->start + seglen / 2;
  uint16_t dest = segrt->counter_mode_step;
  for (uint16_t i = seg->start; i <= seg->stop; i++) {
    if (i == middle + dest || i == middle - dest) {
      ws2812fx.setPixelColor(i, seg->colors[0]);
    } else if (i == middle + dest + 1 || i == middle - dest - 1) {
      ws2812fx.setPixelColor(i, ws2812fx.color_blend(seg->colors[0], BLACK, 250));
    } else {
      ws2812fx.setPixelColor(i, ws2812fx.color_blend(ws2812fx.getPixelColor(i), BLACK, 200));
    }
  }
  if (dest < middle + 10) {
    segrt->counter_mode_step++;
  } else {
    segrt->counter_mode_step = 0;
    ws2812fx.setCycle();
  }
  if (dest == 0) {
    return seg->speed / 8;
  } else {
    return seg->speed / 16;
  }
}

// 星空效果，随机闪烁，然后渐暗
uint16_t starEffect(void) {
  WS2812FX::Segment *seg = ws2812fx.getSegment();
  int seglen = seg->stop - seg->start + 1;
  for (uint16_t i = seg->start; i <= seg->stop; i++) {
    if (ws2812fx.random8() == 0) {
      uint32_t pixel_color = seg->colors[0];
      // pixel_color = ws2812fx.color_blend(pixel_color, ws2812fx.color_wheel(ws2812fx.random8()), 128);
      ws2812fx.setPixelColor(i, pixel_color);
    } else {
      uint32_t pixel_color = ws2812fx.getPixelColor(i);
      pixel_color = ws2812fx.color_blend(pixel_color, BLACK, 20);
      ws2812fx.setPixelColor(i, pixel_color);
    }
  }
  return seg->speed / 64;
}

uint16_t customShow(void) {
  WS2812FX::Segment *seg = ws2812fx.getSegment();
  for (uint16_t i = seg->start; i <= seg->stop; i++) {
    if (i < sizeof(colors)) {
      ws2812fx.setPixelColor(i, colors[i]);
    }
  }
  ws2812fx.setCycle();
  return seg->speed;
}

// ==============================================================

// OTA
void setupOTA(const String &path) {
  // handler for the /update form page
  httpServer.on(path.c_str(), HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, F("text/html"), htmlUpdate);
  });
  // handler for the /update form POST (once file upload finishes)
  httpServer.on(
    path.c_str(), HTTP_POST,
    [](AsyncWebServerRequest *request) {
      if (Update.hasError()) {
        request->send(200, "text/plain", "FAIL");
      } else {
        request->send(200, "text/html", "<META http-equiv=\"refresh\" content=\"5;URL=/\">Update SUCCESS! Rebooting...");
        delay(1000);
        ESP.restart();
      }
    },
    [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
      if (index == 0 && !final) {
        Serial.setDebugOutput(true);
        Serial.printf("Update: %s\n", filename.c_str());
        uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
        if (!Update.begin(maxSketchSpace, U_FLASH)) {
          Update.printError(Serial);
        }
      }
      if (len) {
        Serial.printf(".");
        if (Update.write(data, len) != len) {
          Update.printError(Serial);
        }
      }
      if (final) {
        if (Update.end(true)) {
          Serial.printf("Update Success: \nRebooting...\n");
        } else {
          Update.printError(Serial);
        }
        Serial.setDebugOutput(false);
      }
    });
}
