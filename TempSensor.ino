/*colors*/
//#define temperature_sensors_background ILI9341_BLACK
#define temperature_sensors_color ILI9341_ORANGE
/*colors*/

/*time values*/
constexpr uint16_t temperature_interval = 9;  // 20in sec
/*time values*/

/*coordinates on display*/
constexpr int16_t x_start_temperature = 32;
constexpr int16_t x_start_humidity = 145;
constexpr int16_t y_start_temperature = 220;
constexpr int16_t y_start_humidity = 220;
/*coordinates on display*/

void init_dht() {
  dht.begin();
}
void temperature_sensor_to_display() {  //1 sec inter
  static uint16_t cnt = 0;
  cnt++;
  if (cnt > temperature_interval) {
    tft.setTextColor(temperature_sensors_color);
    //float temp = float(random(-7, 87)) * 1.1f;
    //uint16_t temp1 = random(0, 99);
    print_value_18(dht.readTemperature(),"%.1f", x_start_temperature, y_start_temperature);
    print_value_18(dht.readHumidity(),"%3.0f", x_start_humidity, y_start_humidity);
    cnt = 0;
  }
  if (dht.readTemperature() > 35 || dht.readHumidity() > 85) {
    alarm();
  } else {
    no_alarm();
  }
}
void debug_temperature_sensor() {
  Serial.println(dht.readTemperature());
  Serial.println(dht.readHumidity());
}
