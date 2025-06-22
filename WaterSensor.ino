
#define water_sensor_pin_1 A3  //Wasser Anschlüss
#define water_sensor_pin_2 A6  //Waschmaschine
#define water_sensor_pin_3 A7  //Kühlschrank

/*colors*/
#define water_sensor_background ILI9341_BLACK
#define water_sensor_color_text ILI9341_BLUE
#define water_sensor_color_alarm ILI9341_RED
/*colors*/

/*time values*/
constexpr uint16_t water_interval = 2; // sec
/*time values*/

/*coordinates on display*/
constexpr int16_t x_start_water = 230;
constexpr int16_t y_interval_water = 22;
constexpr int16_t y_start_water = 33;
/*coordinates on display*/

constexpr uint16_t water_alarm = 10;  //in %

/*raw const values*/
constexpr uint16_t air_value_1 = 4095;    //Wasser Anschlüss
constexpr uint16_t water_value_1 = 1700;  //Wasser Anschlüss
constexpr uint16_t air_value_2 = 4095;    //Waschmaschine
constexpr uint16_t water_value_2 = 1650;  //Waschmaschine
constexpr uint16_t air_value_3 = 4095;    //Kühlschrank
constexpr uint16_t water_value_3 = 1900;  //Kühlschrank
/*raw const values*/

uint8_t read_water_alarm(){
  return water_alarm;
}
uint8_t read_water_sensor(uint16_t analog_value, uint16_t air_value, uint16_t water_value) {
  uint8_t temp = map(analog_value, air_value, water_value, 0, 100);
  temp = constrain(temp, 0, 100);
  return temp;
}
uint8_t read_water_sensor_1() {
  //uint16_t temp =random(0,4095);
  return read_water_sensor(analogRead(water_sensor_pin_1), air_value_1, water_value_1);
}
uint8_t read_water_sensor_2() {
  //uint16_t temp =random(0,4095);
  return read_water_sensor(analogRead(water_sensor_pin_2), air_value_2, water_value_2);
}
uint8_t read_water_sensor_3() {
  //uint16_t temp =random(0,4095);
  return read_water_sensor(analogRead(water_sensor_pin_3), air_value_3, water_value_3);
}

void water_sensor_to_display() {
  static uint16_t cnt = 0;
  cnt++;
  if (cnt > water_interval) {
    if (read_water_sensor_1() > water_alarm) {
      tft.setTextColor(water_sensor_color_alarm);
      print_value_9(read_water_sensor_1(),"%3.0f%%", x_start_water, y_start_water);
    } else {
      tft.setTextColor(water_sensor_color_text);
      print_value_9(read_water_sensor_1(),"%3.0f%%", x_start_water, y_start_water);
    }
    if (read_water_sensor_2() > water_alarm) {
      tft.setTextColor(water_sensor_color_alarm);
     print_value_9(read_water_sensor_2(),"%3.0f%%", x_start_water, y_start_water+y_interval_water+1);
    } else {
      tft.setTextColor(water_sensor_color_text);
      print_value_9(read_water_sensor_2(),"%3.0f%%", x_start_water, y_start_water+y_interval_water+1);
    }
    if (read_water_sensor_3() > water_alarm) {
      tft.setTextColor(water_sensor_color_alarm);
      print_value_9(read_water_sensor_3(),"%3.0f%%", x_start_water, y_start_water + y_interval_water * 2);
    } else {
      tft.setTextColor(water_sensor_color_text);
      print_value_9(read_water_sensor_3(),"%3.0f%%", x_start_water, y_start_water + y_interval_water * 2);
    }
    cnt = 0;
  }
  if (read_water_sensor_1() > water_alarm || read_water_sensor_2() > water_alarm || read_water_sensor_3() > water_alarm) {
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
