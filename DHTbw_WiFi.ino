/* *****************************************************************

   Download latest Blinker library here:
   https://github.com/blinker-iot/blinker-library/archive/master.zip


   Blinker is a cross-hardware, cross-platform solution for the IoT.
   It provides APP, device and server support,
   and uses public cloud services for data transmission and storage.
   It can be used in smart home, data monitoring and other fields
   to help users build Internet of Things projects better and faster.

   Make sure installed 2.7.4 or later ESP8266/Arduino package,
   if use ESP8266 with Blinker.
   https://github.com/esp8266/Arduino/releases

   Make sure installed 1.0.5 or later ESP32/Arduino package,
   if use ESP32 with Blinker.
   https://github.com/espressif/arduino-esp32/releases

   Docs: https://diandeng.tech/doc


 * *****************************************************************

   Blinker 库下载地址:
   https://github.com/blinker-iot/blinker-library/archive/master.zip

   Blinker 是一套跨硬件、跨平台的物联网解决方案，提供APP端、设备端、
   服务器端支持，使用公有云服务进行数据传输存储。可用于智能家居、
   数据监测等领域，可以帮助用户更好更快地搭建物联网项目。

   如果使用 ESP8266 接入 Blinker,
   请确保安装了 2.7.4 或更新的 ESP8266/Arduino 支持包。
   https://github.com/esp8266/Arduino/releases

   如果使用 ESP32 接入 Blinker,
   请确保安装了 1.0.5 或更新的 ESP32/Arduino 支持包。
   https://github.com/espressif/arduino-esp32/releases

   文档: https://diandeng.tech/doc


 * *****************************************************************/

#define BLINKER_WIFI

#include <Blinker.h>
#include <DHT.h>
#include <Arduino_JSON.h>
#include <string.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ir_Coolix.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <Ticker.h>

const String host = "ESP32";
const String auth = "";
const String ssid = "";
const String pswd = "";

BlinkerNumber HUMI("humi");
BlinkerNumber TEMP("temp");
BlinkerNumber HIDX("hidx");
BlinkerNumber GAS("gas");
BlinkerText ERR("errlog");
BlinkerButton CURTAIN("curtain");
BlinkerButton AC("ac");
BlinkerButton ACMODE("acmode");
BlinkerNumber ACTEMP("actemp");
BlinkerButton ACTEMPUP("actempup");
BlinkerButton ACTEMPDOWN("actempdown");


void setupOTA();

#define DHTPIN 32
#define MQ2PIN 33
#define ACPIN 13

#define DHTTYPE DHT11   // DHT 11
//#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)

DHT dht(DHTPIN, DHTTYPE);
IRCoolixAC ac(ACPIN);
WebServer server(80);
Ticker timer;

void dataRead(const String & data)
{
  BLINKER_LOG("Blinker readString: ", data);

  Blinker.vibrate();

  uint32_t BlinkerTime = millis();

  Blinker.print("millis", BlinkerTime);
}

void heartbeat()
{
  digitalWrite(LED_BUILTIN, HIGH);
  if (ac.getPower()) {
    AC.text("关闭空调");
    AC.color("LightGreen");
    AC.print();
  }
  ACTEMP.color(ac.getMode() == kCoolixHeat ? "Red" : "Blue");
  ACTEMP.print(ac.getTemp());
  if (ac.getMode() == kCoolixHeat) {
    ACMODE.text("制热");
    ACMODE.icon("fad fa-heat");
    ACMODE.color("OrangeRed");
    ACMODE.print();
  } else {
    ACMODE.text("制冷");
    ACMODE.icon("fad fa-snowflakes");
    ACMODE.color("DeepSkyBlue");
    ACMODE.print();
  }

  GAS.print(analogRead(MQ2PIN) / 40.96);
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (isnan(h) || isnan(t)) {
    BLINKER_LOG("Failed to read from DHT sensor!");
    ERR.print("读取失败");
    digitalWrite(LED_BUILTIN, LOW);
    return;
  }
  float hic = dht.computeHeatIndex(t, h, false);

  BLINKER_LOG("Humidity: ", h, " %");
  BLINKER_LOG("Temperature: ", t, " *C");
  BLINKER_LOG("Heat index: ", hic, " *C");

  HUMI.print(h);
  TEMP.print(t);
  HIDX.print(hic);


  ERR.print("");
  digitalWrite(LED_BUILTIN, LOW);
}

void curtain(const String & state) {
}

void acpower(const String & state) {
  if (!ac.getPower()) {
    ac.on();
    ac.setMode(kCoolixCool);
    ac.setFan(kCoolixFanAuto);
    ac.setTemp(ac.getTemp());
    ac.send();
    AC.text("关闭空调");
    AC.color("LightGreen");
    AC.print();
    ACTEMP.color("Blue");
    ACTEMP.print(ac.getTemp());
    ACMODE.text("制冷");
    ACMODE.icon("fad fa-snowflakes");
    ACMODE.color("DeepSkyBlue");
    ACMODE.print();
  } else {
    ac.off();
    ac.send();
    AC.text("打开空调");
    AC.color("Gray");
    AC.print();
  }
}

void acmode(const String & state) {
  if (!ac.getPower()) {
    ERR.print("未开空调");
    return;
  }
  if (ac.getMode() == kCoolixCool) {
    ac.setMode(kCoolixHeat);
    ac.send();
    ACTEMP.color("Red");
    ACTEMP.print(ac.getTemp());
    ACMODE.text("制热");
    ACMODE.icon("fad fa-heat");
    ACMODE.color("OrangeRed");
    ACMODE.print();
  } else {
    ac.setMode(kCoolixCool);
    ac.send();
    ACTEMP.color("Blue");
    ACTEMP.print(ac.getTemp());
    ACMODE.text("制冷");
    ACMODE.icon("fad fa-snowflakes");
    ACMODE.color("DeepSkyBlue");
    ACMODE.print();
  }
}

void autoac() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (isnan(h) || isnan(t)) {
    return;
  }
  float hic = dht.computeHeatIndex(t, h, false);
  if (!ac.getPower() && hic >= 32 && Blinker.hour() < 23 && Blinker.hour() > 12) {
    ac.on();
    ac.setMode(kCoolixCool);
    ac.setFan(kCoolixFanAuto);
    ac.setTemp(ac.getTemp());
    ac.send();
    Blinker.print("auto ac on!");
  }
}

void actempup(const String & state) {
  if (ac.getTemp() == kCoolixTempMax) {
    ERR.print("最高温度");
    return;
  }
  if (!ac.getPower()) {
    ERR.print("未开空调");
    return;
  }
  ac.setTemp(ac.getTemp() + 1);
  ac.send();
  ACTEMP.color(ac.getMode() == kCoolixCool ? "Blue" : "Red");
  ACTEMP.print(ac.getTemp());
  Blinker.print("set temp", ac.getTemp());
}

void actempdown(const String & state) {
  if (ac.getTemp() == kCoolixTempMin) {
    ERR.print("最低温度");
    return;
  }
  if (!ac.getPower()) {
    ERR.print("未开空调");
    return;
  }
  ac.setTemp(ac.getTemp() - 1);
  ac.send();
  ACTEMP.color(ac.getMode() == kCoolixCool ? "Blue" : "Red");
  ACTEMP.print(ac.getTemp());
  Blinker.print("set temp", ac.getTemp());
}

void setup()
{
  Serial.begin(115200);
  BLINKER_DEBUG.stream(Serial);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  Blinker.begin(auth.c_str(), ssid.c_str(), pswd.c_str());
  Blinker.attachData(dataRead);
  Blinker.attachHeartbeat(heartbeat);
  Blinker.setTimezone(8.0);
  CURTAIN.attach(curtain);
  AC.attach(acpower);
  ACMODE.attach(acmode);
  ACTEMPUP.attach(actempup);
  ACTEMPDOWN.attach(actempdown);
  timer.attach_ms(10 * 1000, autoac);

  dht.begin();
  ac.begin();
  setupOTA();

  digitalWrite(LED_BUILTIN, HIGH);
  Blinker.delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  Blinker.delay(100);
  digitalWrite(LED_BUILTIN, HIGH);
  Blinker.delay(100);
  digitalWrite(LED_BUILTIN, LOW);
}

void loop()
{
  Blinker.run();
  server.handleClient();
}

const char* serverIndex =
  "<!DOCTYPE html>"
  "<html>"
  "<head>"
  "<meta charset='UTF-8'>"
  "<script src='http://cdn.staticfile.org/jquery/3.6.0/jquery.min.js'></script>"
  "</head>"
  "<body>"
  "<div style=text-align:center>"
  "<p>"
  "<img src='https://pic2.zhimg.com/v2-5a203790c6d463fc747cf2007d948a8a_1440w.jpg'"
  "style=width:200px><br><br>"
  "</p>"
  "<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
  "<p>"
  "<input type='file' name='update' accept='.bin'><br>"
  "</p>"
  "<p>"
  "<input type='submit' value='提交'>"
  "</p>"
  "</form>"
  "<p>"
  "<div id='prg'>进度: 0%</div>"
  "</p>"
  "</div>"
  "<script>"
  " $('form').submit(function (e) {"
  "e.preventDefault();"
  "var form = $('#upload_form')[0];"
  "var data = new FormData(form);"
  "$.ajax({"
  "url: '/update',"
  "type: 'POST',"
  "data: data,"
  "contentType: false,"
  "processData: false,"
  "xhr: function () {"
  "var xhr = new window.XMLHttpRequest();"
  "xhr.upload.addEventListener('progress', function (evt) {"
  "if (evt.lengthComputable) {"
  "var per = evt.loaded / evt.total;"
  "$('#prg').html('进度: ' + Math.round(per * 100) + '%');"
  "}"
  "}, false);"
  "return xhr;"
  "},"
  "success: function (d, s) {"
  "console.log('完成!');"
  "},"
  "error: function (a, b, c) {"
  "}"
  "});"
  "});"
  "</script>"
  "</body>"
  "</html>";

void setupOTA() {
  if (!MDNS.begin(host.c_str())) { //http://esp32.local
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");
  /*return index page which is stored in serverIndex */
  server.on("/", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", serverIndex);
  });
  /*handling uploading firmware file */
  server.on("/update", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("Update: %s\n", upload.filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      /* flashing firmware to ESP*/
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
    }
  });
  server.begin();
}
