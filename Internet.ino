

#define INET_INTERVAL_RECONNECT 180


#if (_INET == 1)
void handleNewMessages(int numNewMessages) {
static uint16_t INET_CNT = 0;
  if (WiFi.status() == WL_CONNECTED) {
    tft.fillRect(289, 1, 30, 30, ILI9341_BLACK);
    tft.drawBitmap(289, 1, WIFI_ON1, 30, 30, ILI9341_BLACK, ILI9341_WHITE);
  } else {

    tft.fillRect(289, 1, 30, 30, ILI9341_BLACK);
    tft.drawBitmap(289, 1, WIFI_OFF1, 30, 30, ILI9341_BLACK, ILI9341_WHITE);
    INET_CNT++;
    if (INET_CNT > INET_INTERVAL_RECONNECT) {
      WiFi.begin(ssid, password);
      INET_CNT = 0;
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
    }

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
          message +="Качество воздуха в кухне хорошее \n";
          break;
        case AIR_SENSOR_STATUS_NORMAL:
          message +="Качество воздуха в кухне нормальное \n";
          break;
        case AIR_SENSOR_STATUS_BAD:
          message +="Качество воздуха в кухне плохое \n";
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

    }//// /check_kitchen

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
  //INET_CNT = 0;
  //}
}
#endif
