#if (_INET == 1)

/*colors*/
#define wifi_background ILI9341_BLACK
#define wifi_color ILI9341_GREEN
/*colors*/


/*time values*/
constexpr uint16_t reconnect_time = 180;  // in sec
/*time values*/

/*coordinates on display*/
constexpr int16_t x_start_wifi = 289;
constexpr int16_t y_start_wifi = 1;
/*coordinates on display*/

/*bitmap size*/
constexpr uint16_t w_wifi = 30;
constexpr uint16_t h_wifi = 30;
/*bitmap size*/

void init_inet() {

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT);  // Add root certificate for api.telegram.org
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.println("Connecting to WiFi..");
  }
}
void handleNewMessages(int numNewMessages) {
  static uint16_t cnt = 0;
  if (WiFi.status() == WL_CONNECTED) {
    tft.drawBitmap(x_start_wifi, y_start_wifi, wifi_on, w_wifi, h_wifi, wifi_background, wifi_color);
  } else {
    tft.drawBitmap(x_start_wifi, y_start_wifi, wifi_off, w_wifi, h_wifi, wifi_background, ILI9341_RED);
    cnt++;
    if (cnt > reconnect_time) {
      WiFi.begin(ssid, password);
      cnt = 0;
    }
  }

  Serial.println("handleNewMessages");
  Serial.println(String(numNewMessages));

  for (int i = 0; i < numNewMessages; i++) {
    // Chat id of the requester
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != CHAT_ID) {
      bot.sendMessage(chat_id, "Unauthorized user", "");
      continue;
    }

    // Print the received message
    String text = bot.messages[i].text;

    Serial.println(text);

    String from_name = bot.messages[i].from_name;

    if (text == "/start") {
      String welcome = "Willkommen in der Küche, " + from_name + ".\n";
      welcome += "Verwenden Sie die folgenden Befehle.\n\n";
      welcome += "/check_kitchen zur Überprüfung der Sensoren in der Küche. \n";
      bot.sendMessage(chat_id, welcome, "");
    }
    if (text == "/beep") {
      beep();
      beep();
    }
    if (text == "/check_kitchen") {
      String message = "Die Temperatur in der Küche beträgt: " + String(dht.readTemperature()) + " ºC \n";
      message += "Die Luftfeuchtigkeit in der Küche beträgt: " + String(dht.readHumidity()) + " % \n";
      switch (read_air_status()) {
        case AIR_SENSOR_STATUS_GOOD:
          message += "Die Luftqualität in der Küche ist ausgezeichnet. \n";
          break;
        case AIR_SENSOR_STATUS_OK:
          message += "Die Luftqualität in der Küche ist gut. \n";
          break;
        case AIR_SENSOR_STATUS_NORMAL:
          message += "Die Luftqualität in der Küche ist ok. \n";
          break;
        case AIR_SENSOR_STATUS_BAD:
          message += "Die Luftqualität in der Küche ist schlecht. \n";
          break;
        case AIR_SENSOR_STATUS_VERY_BAD:
          message += "Die Luftqualität in der Küche ist gefährlich! \n";
          break;
      }
      message += "MQ2 " + String(read_mq2()) + " ppm \n";
      message += "MQ135 " + String(read_mq135()) + " ppm \n";
      ////
      if (active_pir_sensor()) {
        message += "In der Küche ist jemand. \n";
      } else {
        message += "In der Küche ist niemand. \n";
      }
      ////
      message += "Leckage-Sensoren: \n";
      message += "Die Feuchtigkeit unter dem Kühlschrank beträgt: " + String(read_water_sensor_1()) + " % \n";
      message += "Die Feuchtigkeit unter der Waschmaschine beträgt: "  + String(read_water_sensor_2()) + " % \n";
      message += "Die Feuchtigkeit unter dem Waschbecken beträgt: "  + String(read_water_sensor_3()) + " % \n";
      bot.sendMessage(chat_id, message, "");

    }

    if (read_water_sensor_3() > read_water_alarm()) {
      bot.sendMessage(chat_id, "Achtung! Es gibt eine Undichtigkeit unter dem Kühlschrank!!!", "");
    }
    if (read_water_sensor_2() > read_water_alarm()) {
      bot.sendMessage(chat_id, "Achtung! Es gibt eine Undichtigkeit unter der Waschmaschine!!!", "");
    }
    if (read_water_sensor_1() > read_water_alarm()) {
      bot.sendMessage(chat_id, "Achtung! Es gibt eine Undichtigkeit hinter dem Mülleimer!!!", "");
    }
    if (dht.readTemperature() > 35 || dht.readHumidity() > 85) {
      bot.sendMessage(chat_id, "Achtung! Hohe Temperatur/Feuchtigkeit in der Küche!!!", "");
    }
  }
}
#endif
