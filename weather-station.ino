#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <Wire.h>
#include <NetworkClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_AHT10.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_BMP280 bmp;
Adafruit_AHT10 aht;
WiFiUDP udp;

WebServer server(80);

const char* ssid = "some-wifi-network";
const char* password = "some-wifi-password";
const char* hostname = "weather-station";

int loop_count = 0;

float* get_weather() {
  static float weather[4];
  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp);
  weather[0] = temp.temperature;
  weather[1] = humidity.relative_humidity;
  weather[2] = bmp.readPressure()/100;
  weather[3] = compute_heat_index(temp.temperature, humidity.relative_humidity);
  return weather;
}

void handleWeather() {
  float* weather = get_weather();
  String fake_json = "{\"temperature\": " + String(weather[0], 2) + ", \"relative_humidity\": " + String(weather[1], 2) + ", \"pressure\": " + String(weather[2], 2) + ", \"heat_index\": " + String(weather[3], 2) + "}";
  server.send(200, "application/json", fake_json);
}

void handleRoot() {
  server.send(200, "text/plain", "hewwo uwu");
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

float compute_heat_index(float t, float r) {
  float c_1 = -8.78469475556;
  float c_2 = 1.61139411;
  float c_3 = 2.33854883889;
  float c_4 = -0.14611605;
  float c_5 = -0.012308094;
  float c_6 = -0.0164248277778;
  float c_7 = 2.211732*0.001;
  float c_8 = 7.2546*0.0001;
  float c_9 = -3.582*0.000001;
  float heat_index = (c_1) + (c_2*t) + (c_3*r) + (c_4*t*r) + (c_5*t*t) + (c_6*r*r) + (c_7*t*t*r) + (c_8*t*r*r) + (c_9*t*t*r*r);
  return heat_index;
}

void update_display() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  float* weather = get_weather();
  display.println("T:  "+String(weather[0], 1)+char(247)+"C Hu: "+String(weather[1], 1)+"%");
  display.println("HI: "+String(weather[3], 1)+char(247)+"C P: "+String(weather[2], 0)+"hPa");
  display.display(); 
}

void setup() {
  // Set I2C pins to GPIO19 (SDA) and GPIO22 (SCK)
  Wire.begin(19, 22);

  // Initialize serial interface
  Serial.begin(115200);
  Serial.println("Booting");

  // Set display to initializing
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  display.println("Initializing weather station...");
  display.display(); 

  // Connect to WiFi
  WiFi.setHostname(hostname);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  // Initialize ArduinoOTA
  // Port defaults to 3232
  ArduinoOTA.setHostname(hostname);
  ArduinoOTA.setPassword("some-ota-password");
  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");
  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else
        type = "filesystem";
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
  ArduinoOTA.begin();

  // Print IP address
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Initialize AHT10
  if (! aht.begin()) {
    Serial.println("Could not find the AHT10 sensor");
    while (1) delay(10);
  }
  Serial.println("AHT10 sensor found");
  
  // Initialize BMP280
  if (!bmp.begin()) {
    Serial.println(F("Could not find the BMP280 sensor"));
    while (1);
  }
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL, Adafruit_BMP280::SAMPLING_X2, Adafruit_BMP280::SAMPLING_X16, Adafruit_BMP280::FILTER_X16, Adafruit_BMP280::STANDBY_MS_500);
  Serial.println("BMP280 sensor found");

  // Initialize webserver
  if (MDNS.begin("esp32")) {
    Serial.println("MDNS responder started");
  }
  server.on("/", handleRoot);
  server.on("/weather", handleWeather);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  ArduinoOTA.handle();
  server.handleClient();

  // Update SSD1306 128x32 every 1 second
  if (loop_count > 500) {
    update_display();
    loop_count = 0;
  }
  loop_count++;
  delay(2);
}
