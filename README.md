# ESP32 weather station
This sketch was made for the WEMOS Lite V1.0.0 ESP32 with the BMP280 and AHT10 sensors plus the SSD1306 128x32 OLED display.
## Features
- Includes weather data for temperature, humidity, pressure, and heat index
- JSON weather data available at `http://<weather station IP>/weather`
- Weather data available on an OLED display which has a 1-second refresh rate
- Password-protected Over-The-Air (OTA) updates for applying firmware over Wi-Fi
