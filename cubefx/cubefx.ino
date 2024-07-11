#define VERSION "dev"

#include <ArduinoJson.h>
#include <EEPROM.h>
#include <OneButton.h>
#include <WS2812FX.h>  // https://github.com/kitesurfer1404/WS2812FX
#include <WebServer.h>
#include <WiFi.h>
// For OTA
// #include <ESPmDNS.h>
#include <Update.h>

/* Put your HOST SSID & Password */
const char* host = "CubeFX";
const char* ssid = "ZimaCube";       // Enter SSID here
const char* password = "homecloud";  // Enter Password here

/* Put IP Address details */
IPAddress local_ip(172, 16, 1, 1);
IPAddress gateway(172, 16, 1, 1);
IPAddress subnet(255, 255, 255, 0);

WebServer httpServer(80);
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
<li><a href='/get'>Light Status</a></li>
<li><a href='/light/switch'>Light Switch</a></li>
<li><a href='/light/mode'>Light NextDemo</a></li>
<li><a href='/wifi/off'>WiFiAP TurnOff</a></li>
<li><a href='/update'>Firmware Update</a></li>
</ul>
<p>Developed by: <a href='https://github.com/Cp0204/CubeFX'>Cp0204</a></p>
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
  WiFi.softAPConfig(local_ip, gateway, subnet);
  WiFi.softAP(ssid, password);
}

void openHttpServer() {
  // OTA
  setupOTA("/update");
  // index
  httpServer.on("/", HTTP_GET, []() {
    httpServer.send(200, "text/html", htmlIndex);
  });
  // light post
  httpServer.on("/post", HTTP_POST, handleHttpPost);
  // light get
  httpServer.on("/get", HTTP_GET, handleHttpGet);
  // version
  httpServer.on("/version", HTTP_GET, [] {
    httpServer.send(200, "application/json", "{\"version\":\"" + String(VERSION) + "\"}");
  });
  // wifi
  httpServer.on("/wifi/off", [] {
    httpServer.send(200, "text/plain", "Turning off WiFi... Press the redlight side Button or Re-insert to Reopen");
    delay(1000);
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
  });
  // light switch
  httpServer.on("/light/switch", [] {
    if (ws2812fx.isRunning()) {
      httpServer.send(200, "text/plain", "Switch Light... Now OFF");
      isLightOn = false;
      ws2812fx.stop();
    } else {
      httpServer.send(200, "text/plain", "Switch Light... Now ON");
      isLightOn = true;
      ws2812fx.start();
    }
    EEPROM.put(EEPROM_ADDR_IS_LIGHT_ON, isLightOn);
    EEPROM.commit();
  });
  // light switch
  httpServer.on("/light/mode", [] {
    effectId = effectId == 5 ? -73 : (effectId + 1);
    showEffect();
    httpServer.send(200, "text/plain", "Light Next Mode... " + String(effectId) + ":" + ws2812fx.getModeName(ws2812fx.getMode()));
  });
  // 404
  httpServer.onNotFound([] {
    httpServer.send(404, "text/plain", "Not found");
  });
  httpServer.begin();
  // MDNS.addService("http", "tcp", 80);
  Serial.printf("HTTPServer Started: http://%s/update\n", WiFi.softAPIP().toString());
}

// 颜色兼容rgb和hsv输入
// https://colorpicker.me/
uint32_t compatibleColor(JsonObject color) {
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
  } else {
    return 0;
  }
}

void showEffect() {
  ws2812fx.resume();
  ws2812fx.setSpeed(map(255 - effectSpeed, 0, 255, 0, 5000));  // 在 ws2812fx 速度代表帧 ms 延迟，越大越慢
  ws2812fx.setBrightness(lightness);
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

void handleHttpPost() {
  DeserializationError error = deserializeJson(doc, httpServer.arg("plain"));
  if (error) {
    Serial.print("Failed to parse JSON: ");
    Serial.println(error.c_str());
    httpServer.send(400, "text/plain", "Bad Request");
  } else {
    effectId = doc["id"];
    effectSpeed = constrain(doc["speed"], 0, 255);
    lightness = constrain(doc["lightness"], 0, 254) + 1;
    JsonArray data = doc["data"];
    for (int i = 0; i < NUM_PIXELS; i++) {
      colors[i] = compatibleColor(data[i]);
    }
    showEffect();
    saveToEEPROM();
    httpServer.send(200, "application/json", "{\"response\":\"Received\",\"id\":" + String(effectId) + "}");
  }
}

void handleHttpGet() {
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
  httpServer.send(200, "application/json", jsonResponse);
}

void handleBtnClick() {
  digitalWrite(BOARD_LED, LOW);
  delay(50);
  digitalWrite(BOARD_LED, HIGH);
  openAP();
}

void setup() {
  // Init IO
  pinMode(BOARD_LED, OUTPUT);
  digitalWrite(BOARD_LED, HIGH);
  //
  Serial.begin(15200);
  Serial.println("Initial the ZimaCube light effect system!");
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
  //
  ws2812fx.init();
  // 自定义效果，从 0:72 开始注册
  ws2812fx.setCustomMode(0, F("waterDropEffect"), waterDropEffect);  // 72
  ws2812fx.setCustomMode(1, F("starEffect"), starEffect);            // 73
  ws2812fx.setCustomMode(2, F("customShow"), customShow);            // 74
  if (isLightOn) {
    ws2812fx.start();
    showEffect();
  }
  //
  button.attachClick(handleBtnClick);
  //
  openAP();
  openHttpServer();
}

void loop() {
  httpServer.handleClient();
  ws2812fx.service();
  button.tick();
}

// ===== 自定义效果 =====

// 水滴效果，从中间扩散
uint16_t waterDropEffect(void) {
  WS2812FX::Segment* seg = ws2812fx.getSegment();
  WS2812FX::Segment_runtime* segrt = ws2812fx.getSegmentRuntime();
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

// 星空效果，随机闪缩，然后渐暗
uint16_t starEffect(void) {
  WS2812FX::Segment* seg = ws2812fx.getSegment();
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
  WS2812FX::Segment* seg = ws2812fx.getSegment();
  for (int i = 0; i < NUM_PIXELS; i++) {
    ws2812fx.setPixelColor(i, colors[i]);
  }
  ws2812fx.setCycle();
  return seg->speed;
}

// ==============================================================

// OTA
void setupOTA(const String& path) {
  // handler for the /update form page
  httpServer.on(path.c_str(), HTTP_GET, []() {
    httpServer.send(200, F("text/html"), htmlUpdate);
  });
  // handler for the /update form POST (once file upload finishes)
  httpServer.on(
    path.c_str(), HTTP_POST,
    []() {
      if (Update.hasError()) {
        httpServer.send(200, "text/plain", "FAIL");
      } else {
        httpServer.send(200, "text/html", "<META http-equiv=\"refresh\" content=\"15;URL=/\">Update SUCCESS! Rebooting...");
        delay(1000);
        ESP.restart();
      }
    },
    []() {
      HTTPUpload& upload = httpServer.upload();
      if (upload.status == UPLOAD_FILE_START) {
        Serial.setDebugOutput(true);
        Serial.printf("Update: %s\n", upload.filename.c_str());
        uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
        if (!Update.begin(maxSketchSpace, U_FLASH)) {
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_WRITE) {
        Serial.printf(".");
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) {
          Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
        } else {
          Update.printError(Serial);
        }
        Serial.setDebugOutput(false);
      } else if (upload.status == UPLOAD_FILE_ABORTED) {
        Update.end();
        Serial.println("Update was aborted");
      }
    });
}
