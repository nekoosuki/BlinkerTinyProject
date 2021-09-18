/* *****************************************************************
 *
 * Download latest Blinker library here:
 * https://github.com/blinker-iot/blinker-library/archive/master.zip
 * 
 * 
 * Blinker is a cross-hardware, cross-platform solution for the IoT. 
 * It provides APP, device and server support, 
 * and uses public cloud services for data transmission and storage.
 * It can be used in smart home, data monitoring and other fields 
 * to help users build Internet of Things projects better and faster.
 * 
 * Make sure installed 2.7.4 or later ESP8266/Arduino package,
 * if use ESP8266 with Blinker.
 * https://github.com/esp8266/Arduino/releases
 * 
 * Make sure installed 1.0.5 or later ESP32/Arduino package,
 * if use ESP32 with Blinker.
 * https://github.com/espressif/arduino-esp32/releases
 * 
 * Docs: https://diandeng.tech/doc
 *       
 * 
 * *****************************************************************
 * 
 * Blinker 库下载地址:
 * https://github.com/blinker-iot/blinker-library/archive/master.zip
 * 
 * Blinker 是一套跨硬件、跨平台的物联网解决方案，提供APP端、设备端、
 * 服务器端支持，使用公有云服务进行数据传输存储。可用于智能家居、
 * 数据监测等领域，可以帮助用户更好更快地搭建物联网项目。
 * 
 * 如果使用 ESP8266 接入 Blinker,
 * 请确保安装了 2.7.4 或更新的 ESP8266/Arduino 支持包。
 * https://github.com/esp8266/Arduino/releases
 * 
 * 如果使用 ESP32 接入 Blinker,
 * 请确保安装了 1.0.5 或更新的 ESP32/Arduino 支持包。
 * https://github.com/espressif/arduino-esp32/releases
 * 
 * 文档: https://diandeng.tech/doc
 *       
 * 
 * *****************************************************************/

#define BLINKER_WIFI

#include <Blinker.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <Arduino_JSON.h>

String auth = "***";
String ssid = "***";
String pswd = "***";

BlinkerNumber HUMI("humi");
BlinkerNumber TEMP("temp");
BlinkerNumber HIDX("hidx");
BlinkerNumber GAS("gas");
BlinkerText WEATHER1("weather1");
BlinkerText WEATHER2("weather2");
BlinkerText WEATHER3("weather3");
BlinkerText TEMP1("temp1");
BlinkerText TEMP2("temp2");
BlinkerText TEMP3("temp3");
BlinkerText TIME("datatime");
BlinkerButton WEATHERBUTTON("getweather");

#define DHTPIN 32
#define MQ2PIN 33

#define DHTTYPE DHT11   // DHT 11
//#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)

DHT dht(DHTPIN, DHTTYPE);
HTTPClient http;

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
    
    dht.readTemperature();

    Blinker.delay(3000);
    
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    if (isnan(h) || isnan(t)) {
        BLINKER_LOG("Failed to read from DHT sensor!");
        return;
    }

    float hic = dht.computeHeatIndex(t, h, false);
    
    BLINKER_LOG("Humidity: ", h, " %");
    BLINKER_LOG("Temperature: ", t, " *C");
    BLINKER_LOG("Heat index: ", hic, " *C");
    
    HUMI.print(h);
    TEMP.print(t);
    HIDX.print(hic);
    GAS.print(analogRead(MQ2PIN)/40.96);

    digitalWrite(LED_BUILTIN, LOW);
}

void getweather(const String & state)
{
    int httpcode;
    JSONVar JSONObject;
    String payload = "";
    http.begin("http://iot.diandeng.tech/api/v1/user/device/diy/auth?authKey="+auth);
    httpcode = http.GET();
    Serial.printf("HTTPCODE:%d",httpcode);
    payload = http.getString();
    http.end();
    JSONObject = JSON.parse(payload);
    String devname = (const char*)JSONObject["detail"]["deviceName"];
    String token = (const char*)JSONObject["detail"]["iotToken"];
    
    http.begin("http://iot.diandeng.tech/api/v3/forecast?code=510100&device="+devname+"&key="+token);
    httpcode = http.GET();
    Serial.printf("HTTPCODE:%d",httpcode);
    payload = http.getString();
    http.end();
    Serial.println(payload);
    JSONObject = JSON.parse(payload);
    int m = JSONObject["message"];
    Serial.println(m);
    WEATHER1.print((const char*)JSONObject["detail"]["forecasts"][0]["dayweather"]);
    WEATHER2.print((const char*)JSONObject["detail"]["forecasts"][1]["dayweather"]);
    WEATHER3.print((const char*)JSONObject["detail"]["forecasts"][2]["dayweather"]);

    String s1 = "-";
    String s2 = "°C";
    String s3 = "更新时间";
    
    TEMP1.print((const char*)JSONObject["detail"]["forecasts"][0]["nighttemp"]+s1+(const char*)JSONObject["detail"]["forecasts"][0]["daytemp"]+s2);
    TEMP2.print((const char*)JSONObject["detail"]["forecasts"][1]["nighttemp"]+s1+(const char*)JSONObject["detail"]["forecasts"][1]["daytemp"]+s2);
    TEMP3.print((const char*)JSONObject["detail"]["forecasts"][2]["nighttemp"]+s1+(const char*)JSONObject["detail"]["forecasts"][2]["daytemp"]+s2);
    const char* timetmp = (const char*)JSONObject["detail"]["updateTime"];
    char buff[6];
    strncpy(buff, timetmp+5, 5);
    buff[5] = '\0';
    TIME.print(s3+buff);
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
    WEATHERBUTTON.attach(getweather);
    
    dht.begin();

    digitalWrite(LED_BUILTIN, HIGH);
    Blinker.delay(50);
    digitalWrite(LED_BUILTIN, LOW);
    Blinker.delay(50);
    digitalWrite(LED_BUILTIN, HIGH);
    Blinker.delay(50);
    digitalWrite(LED_BUILTIN, LOW);
}

void loop()
{
    Blinker.run();
}
