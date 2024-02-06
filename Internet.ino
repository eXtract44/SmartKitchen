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
      String welcome = "Добро пожаловать в кухню, " + from_name + ".\n";
      welcome += "Используйте следуйщие команды.\n\n";
      welcome += "/check_kitchen для проверики датчиков на кухне \n";
      bot.sendMessage(chat_id, welcome, "");
    }
    if (text == "/beep") {
      beep();
      beep();
    }
    /*if(text == "/set_standby_time"){
      bot.sendMessage(chat_id, "min value 0 sec max value 65000 sec \n ", "");
        pir_delay
    }*/
    //bot.sendMessage(chat_id, "HELLO FROM ESP32", "");
    ////
    if (text == "/check_kitchen") {
      String message = "Температура в кухне составляет: " + String(dht.readTemperature()) + " ºC \n";
      message += "Влажность в кухне составляет: " + String(dht.readHumidity()) + " % \n";
      switch (read_air_status()) {
        case AIR_SENSOR_STATUS_GOOD:
          message += "Качество воздуха в кухне отличное \n";
          break;
        case AIR_SENSOR_STATUS_OK:
          message += "Качество воздуха в кухне хорошее \n";
          break;
        case AIR_SENSOR_STATUS_NORMAL:
          message += "Качество воздуха в кухне нормальное \n";
          break;
        case AIR_SENSOR_STATUS_BAD:
          message += "Качество воздуха в кухне плохое \n";
          break;
        case AIR_SENSOR_STATUS_VERY_BAD:
          message += "Качество воздуха в кухне ужасное/опасность! \n";
          break;
      }
      message += "MQ2 " + String(read_mq2()) + " ppm \n";
      message += "MQ135 " + String(read_mq135()) + " ppm \n";
      ////
      if (active_pir_sensor()) {
        message += "В кухне кто-то есть \n";
      } else {
        message += "В кухне никого нету \n";
      }
      ////
      message += "Датчики протечки: \n";
      message += "Под холодильником влажонсть состовляет: " + String(read_water_sensor_1()) + " % \n";
      message += "Под стиральной машинкой влажонсть состовляет: " + String(read_water_sensor_2()) + " % \n";
      message += "Под умывальником влажонсть состовляет: " + String(read_water_sensor_3()) + " % \n";
      bot.sendMessage(chat_id, message, "");

    }  //// /check_kitchen

    if (read_water_sensor_3() > read_water_alarm()) {
      bot.sendMessage(chat_id, "ВНИМАНИЕ ПРОТЕЧКА ПОД ХОЛОДИЛЬНИКОМ !!!", "");
    }
    if (read_water_sensor_2() > read_water_alarm()) {
      bot.sendMessage(chat_id, "ВНИМАНИЕ ПРОТЕЧКА ПОД СТИРАЛКОЙ !!!", "");
    }
    if (read_water_sensor_1() > read_water_alarm()) {
      bot.sendMessage(chat_id, "ВНИМАНИЕ ПРОТЕЧКА ЗА МУСОРНИКОМ !!!", "");
    }
    if (dht.readTemperature() > 35 || dht.readHumidity() > 85) {
      bot.sendMessage(chat_id, "ВНИМАНИЕ ВЫСОКАЯ ТЕМПЕРАТУРА/ВЛАЖОНСТЬ В КУХНЕ !!!", "");
    }
  }
  //cnt = 0;
  //}
}
#endif
