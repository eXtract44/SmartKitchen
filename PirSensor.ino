
#define pir_sensor_pin 26

/*colors*/
#define pir_sensors_background ILI9341_BLACK
#define pir_sensors_color ILI9341_WHITE
/*colors*/

/*time values*/
constexpr uint16_t time_pir_delay = 30;      // in sec
constexpr uint16_t time_activate_delay = 3;  // in sec
/*time values*/

/*coordinates on display*/
constexpr int16_t x_start_pir = 260;
constexpr int16_t y_start_pir = 180;
/*coordinates on display*/

/*bitmap size*/
constexpr uint16_t w_pir = 53;
constexpr uint16_t h_pir = 53;
/*bitmap size*/

bool pir_sensor_feedback = 0;

void init_pir() {
  pinMode(pir_sensor_pin, INPUT);
}
bool read_pir_sensor() {
  return digitalRead(pir_sensor_pin);
}
bool read_pir_sensor_delayed() {
  static uint16_t cnt_delay_activate = 0;
  if (read_pir_sensor() == true) {
    cnt_delay_activate++;
  }
  if (cnt_delay_activate > time_activate_delay) {
    cnt_delay_activate = 0;
    return true;
  } else {
    return false;
  }
}
bool active_pir_sensor() {
  return pir_sensor_feedback;
}

void pir_sensor_to_display() {  // 1 sec interval
  if (digitalRead(pir_sensor_pin) == HIGH) {
    tft.drawBitmap(x_start_pir, y_start_pir, pir_sensor, w_pir, h_pir, pir_sensors_background, pir_sensors_color);
  } else {
    tft.fillRect(260, 180, 53, 53, ILI9341_BLACK);
  }
}

void debug_pir_sensor() {
  if (digitalRead(pir_sensor_pin) == HIGH) {
    Serial.println("Somebody is here");
  } else {
    Serial.println("Nobody is here");
  }
}
