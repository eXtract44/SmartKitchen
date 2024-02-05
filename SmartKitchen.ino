// LIBERY ESP32 DEV MODUL 

#define _DEBUG 0
#define _DISPLAY 1
#define _INET 1

#if (_INET == 1)
//internet
#include "pic.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>   // Universal Telegram Bot Library written by Brian Lough: https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot
#include <ArduinoJson.h>
// Replace with your network credentials
const char* ssid = "WiFi";
const char* password = "Lokomotive132";
// Initialize Telegram BOT
#define BOTtoken "5657548564:AAGvnIlqb-UVp6fik-6DC_sK2O7n_C05v3o"  // your Bot Token (Get from Botfather)

// Use @myidbot to find out the chat ID of an individual or a group
// Also note that you need to click "start" on a bot before it can
// message you
#define CHAT_ID "421330653"
WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

//inter
#endif

//display
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
//display
#include <Fonts/FreeSansBold9pt7b.h> 
#include <Fonts/FreeSansBold12pt7b.h>  // 9,12,18,24
#include <Fonts/FreeSansBold18pt7b.h>

#endif

#include "DHT.h"
#define DHTPIN 25
#define DHTTYPE DHT21   // DHT 21 (AM2301)
DHT dht(DHTPIN, DHTTYPE);

//
unsigned long previousMillis = 0;
const long interval = 1000; //milisec
void setup() {
pinMode(26, INPUT); // for PIR SENSOR
#if (_DEBUG == 1)
  Serial.begin(9600);
  Serial.println("DEBUG MOD STARTED");
#endif
  //ineet
#if (_INET == 1)
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
#if (_DEBUG == 1)
    Serial.println("Connecting to WiFi..");
#endif
  }
#endif
  //ineet

  ledcSetup(0, 5000, 8); //channel,hz,bit setup PWM OUT for buzzer
  ledcAttachPin(14, 0); //BUZZER_PIN, PWM_CHANNEL_BUZZER
  ledcSetup(2, 5000, 8); //channel,hz,bit setup PWM OUT for display
  ledcAttachPin(16, 2); //LED_PIN, PWM_CHANNEL_LED


  dht.begin();


#if (_DISPLAY == 1)
  set_brightness(50);
  tft.begin();
  tft.setRotation(3);
  //tft.fillScreen(ILI9341_WHITE);
  draw_menu();
  
  
#endif

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
  if (currentMillis - previousMillis >= interval) { //1 sec
    previousMillis = currentMillis;
#if (_DISPLAY == 1)
    UPDATE_DISPLAY();
#endif
#if (_DEBUG == 1)
    UPDATE_DEBUG();
#endif
#if (_INET == 1)
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    handleNewMessages(numNewMessages);
#endif

  }
}//loop
