
#define WATER_SENSOR_PIN_1 A3  // WASSERANSCH
#define WATER_SENSOR_PIN_2 A6  //WASCHMA
#define WATER_SENSOR_PIN_3 A7  //KÜHL

#define WATER_SENSOR_INTERVAL 15  //in sec
constexpr uint8_t WATER_ALARM = 10;  //in %

constexpr uint16_t AIR_VALUE_1 = 4095;    //wass ansch
constexpr uint16_t WATER_VALUE_1 = 1700;  //was ansch
constexpr uint16_t AIR_VALUE_2 = 4095;    //wasch
constexpr uint16_t WATER_VALUE_2 = 1650;  //wasch
constexpr uint16_t AIR_VALUE_3 = 4095;    //ku
constexpr uint16_t WATER_VALUE_3 = 1900;  //ku

#define WATER_SENSOR_BACKGROUND ILI9341_BLACK
#define WATER_SENSOR_TEXT_FILL

uint16_t x_start_water = 240;
uint16_t y_interval_water = 23;
uint16_t y_start_water = 32;


uint8_t read_water_alarm(){
  return WATER_ALARM;
}
uint8_t read_water_sensor(uint16_t analog_value, uint16_t air_value, uint16_t water_value) {
  uint8_t temp = map(analog_value, air_value, water_value, 0, 100);
  temp = constrain(temp, 1, 99);
  return temp;
}
uint8_t read_water_sensor_1() {
  return read_water_sensor(analogRead(WATER_SENSOR_PIN_1), AIR_VALUE_1, WATER_VALUE_1);
}
uint8_t read_water_sensor_2() {
  return read_water_sensor(analogRead(WATER_SENSOR_PIN_2), AIR_VALUE_2, WATER_VALUE_2);
}
uint8_t read_water_sensor_3() {
  return read_water_sensor(analogRead(WATER_SENSOR_PIN_3), AIR_VALUE_3, WATER_VALUE_3);
}

void water_sensor_to_display() {
  static uint16_t WATER_SENSOR_CNT = 0;
  WATER_SENSOR_CNT++;
  if (WATER_SENSOR_CNT > WATER_SENSOR_INTERVAL) {
    tft.setFont(&FreeSansBold9pt7b);
    if (read_water_sensor_1() > WATER_ALARM) {
      tft.setTextColor(ILI9341_RED);
      print_int(read_water_sensor_1(), x_start_water, y_start_water);
    } else {
      tft.setTextColor(ILI9341_WHITE);
      print_int(read_water_sensor_1(), x_start_water, y_start_water);
    }
    if (read_water_sensor_2() > WATER_ALARM) {
      tft.setTextColor(ILI9341_RED);
      print_int(read_water_sensor_2(), x_start_water, y_start_water + y_interval_water);
    } else {
      tft.setTextColor(ILI9341_WHITE);
      print_int(read_water_sensor_2(), x_start_water, y_start_water + y_interval_water);
    }
    if (read_water_sensor_3() > WATER_ALARM) {
      tft.setTextColor(ILI9341_RED);
      print_int(read_water_sensor_3(), x_start_water, y_start_water + y_interval_water * 2);
    } else {
      tft.setTextColor(ILI9341_WHITE);
      print_int(read_water_sensor_3(), x_start_water, y_start_water + y_interval_water * 2);
    }
    WATER_SENSOR_CNT = 0;
  }
  if (read_water_sensor_1() > WATER_ALARM || read_water_sensor_2() > WATER_ALARM || read_water_sensor_3() > WATER_ALARM) {
    alarm();
  } else {
    no_alarm();
  }
}

void debug_water_sensor() {
  Serial.print("Analog A7 digit ");
  Serial.println(analogRead(A7));
  Serial.print("Analog A6 digit ");
  Serial.println(analogRead(A6));
  Serial.print("Analog A3 digit ");
  Serial.println(analogRead(A3));
}
