// LIBERY ESP32 DEV MODUL

#define _DISPLAY 1
#define _INET 1

#include "pic.h"

#include "DHT.h"
#define DHTPIN 25
#define DHTTYPE DHT21  // DHT 21 (AM2301)
DHT dht(DHTPIN, DHTTYPE);

#if (_INET == 1)
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
// Replace with your network credentials
const char* ssid = "WiFi";
const char* password = "Lokomotive132";
// Initialize Telegram BOT
#include <UniversalTelegramBot.h>  // Universal Telegram Bot Library written by Brian Lough: https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot
#define BOTtoken "5657548564:AAGvnIlqb-UVp6fik-6DC_sK2O7n_C05v3o"  // your Bot Token (Get from Botfather)

// Use @myidbot to find out the chat ID of an individual or a group
// Also note that you need to click "start" on a bot before it can
// message you
#define CHAT_ID "421330653"
WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);
#endif

#if (_DISPLAY == 1)
#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#define TFT_SCK    18//
#define TFT_MOSI   23//
#define TFT_CS     22//
#define TFT_DC     21//
#define TFT_RESET  17//
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCK, TFT_RESET);
#include <Fonts/FreeSansBold9pt7b.h> 
#include <Fonts/FreeSansBold12pt7b.h>  // 9,12,18,24
#include <Fonts/FreeSansBold18pt7b.h>
#endif

unsigned long previousMillis = 0;
constexpr long interval = 1000;  //milisec
void setup() {
  Serial.begin(9600);
  Serial.println("Serial ready");
  init_pir();
#if (_INET == 1)
  init_inet();
#endif
#if (_DISPLAY == 1)
  init_disp();
#endif
  init_buzzer();
  init_dht();
  beep();
}

void UPDATE_DISPLAY() {
  pir_sensor_to_display();
  temperature_sensor_to_display();
  air_sensor_to_display();
  water_sensor_to_display();
}
void UPDATE_DEBUG() {
  //debug_pir_sensor();
  //debug_temperature_sensor();
  //debug_air_sensor();
  debug_water_sensor();
}
void loop(void) {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {  //1 sec
    previousMillis = currentMillis;
#if (_DISPLAY == 1)
    UPDATE_DISPLAY();
#endif
    UPDATE_DEBUG();
#if (_INET == 1)
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    handleNewMessages(numNewMessages);
#endif
  }
}  //loop
