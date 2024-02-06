#include "air_sensor.h"

/*colors*/
#define air_sensors_background ILI9341_BLACK
#define air_sensors_color ILI9341_YELLOW
#define air_stars_color ILI9341_GREEN
/*colors*/

/*time values*/
uint16_t air_sensors_update_inteval = 10; //10 in sec
/*time values*/

/*coordinates on display*/
constexpr int16_t x_start_air = 65;
constexpr int16_t y_start_air = 106;
constexpr int16_t x_start_air2 = 235;

constexpr int16_t x_start_stars = 113;
constexpr int16_t y_start_stars = 136;
constexpr int16_t x_start_stars_offset = 38;
/*coordinates on display*/

/*bitmap size*/
constexpr uint16_t w_star = 35;
constexpr uint16_t h_star = 31;
/*bitmap size*/

void draw_stars(uint8_t status_) {
  tft.drawBitmap(x_start_stars, y_start_stars, star, w_star, h_star, air_sensors_background, air_stars_color);
  tft.drawBitmap(x_start_stars + x_start_stars_offset, y_start_stars, star, w_star, h_star, air_sensors_background, air_stars_color);
  tft.drawBitmap(x_start_stars + x_start_stars_offset * 2, y_start_stars, star, w_star, h_star, air_sensors_background, air_stars_color);
  tft.drawBitmap(x_start_stars + x_start_stars_offset * 3, y_start_stars, star, w_star, h_star, air_sensors_background, air_stars_color);
  tft.drawBitmap(x_start_stars + x_start_stars_offset * 4, y_start_stars, star, w_star, h_star, air_sensors_background, air_stars_color);
  switch (status_) {
    case AIR_SENSOR_STATUS_GOOD:  //5 star good
      break;
    case AIR_SENSOR_STATUS_OK:  //4 star ok
      tft.drawBitmap(x_start_stars + x_start_stars_offset * 4, y_start_stars, no_star, w_star, h_star, air_sensors_background, air_stars_color);
      break;
    case AIR_SENSOR_STATUS_NORMAL:  //3 star normal
      tft.drawBitmap(x_start_stars + x_start_stars_offset * 3, y_start_stars, no_star, w_star, h_star, air_sensors_background, air_stars_color);
      tft.drawBitmap(x_start_stars + x_start_stars_offset * 4, y_start_stars, no_star, w_star, h_star, air_sensors_background, air_stars_color);
      break;
    case AIR_SENSOR_STATUS_BAD:  //2 star bad
      tft.drawBitmap(x_start_stars + x_start_stars_offset * 2, y_start_stars, no_star, w_star, h_star, air_sensors_background, air_stars_color);
      tft.drawBitmap(x_start_stars + x_start_stars_offset * 3, y_start_stars, no_star, w_star, h_star, air_sensors_background, air_stars_color);
      tft.drawBitmap(x_start_stars + x_start_stars_offset * 4, y_start_stars, no_star, w_star, h_star, air_sensors_background, air_stars_color);
      break;
    case AIR_SENSOR_STATUS_VERY_BAD:  //1 star to bad
      tft.drawBitmap(x_start_stars + x_start_stars_offset, y_start_stars, no_star, w_star, h_star, air_sensors_background, air_stars_color);
      tft.drawBitmap(x_start_stars + x_start_stars_offset * 2, y_start_stars, no_star, w_star, h_star, air_sensors_background, air_stars_color);
      tft.drawBitmap(x_start_stars + x_start_stars_offset * 3, y_start_stars, no_star, w_star, h_star, air_sensors_background, air_stars_color);
      tft.drawBitmap(x_start_stars + x_start_stars_offset * 4, y_start_stars, no_star, w_star, h_star, air_sensors_background, air_stars_color);
      break;
  }
}
void draw_mq_value(float value, uint16_t x, uint16_t y) {
  tft.setTextColor(air_sensors_color);
  print_value_12(value,"%5.0f", x, y);
}
void air_sensor_to_display() {
  static uint16_t cnt = 0;
  cnt++;
  if (cnt > air_sensors_update_inteval) {
    draw_mq_value(read_mq2(), x_start_air, y_start_air);
    draw_mq_value(read_mq135(), x_start_air2, y_start_air);
    draw_stars(read_air_status());
    cnt = 0;
  }
  if (read_mq135() > MQ135_LVL_ALARM || read_mq2() > MQ2_LVL_ALARM) {
    alarm();
  }else{
    no_alarm();
  }
}
uint16_t read_mq2(){
  //uint16_t temp =random(0,655);
  return map(analogRead(MQ2_ANALOG_PIN), MQ2_MIN_DIGIT, MQ2_MAX_DIGIT, MQ2_MIN_PPM, MQ2_MAX_PPM);
}
uint16_t read_mq135(){
  //uint16_t temp =random(0,500);
  return map(analogRead(MQ135_ANALOG_PIN), MQ135_MIN_DIGIT, MQ135_MAX_DIGIT, MQ135_MIN_PPM, MQ135_MAX_PPM);
}
uint8_t read_air_status() {
  if (read_mq135() > MQ135_LVL4 || read_mq2() > MQ2_LVL4) {
    return AIR_SENSOR_STATUS_VERY_BAD;
  } else if (read_mq135() > MQ135_LVL3 || read_mq2() > MQ2_LVL3) {
    return AIR_SENSOR_STATUS_BAD;
  } else if (read_mq135() > MQ135_LVL2 || read_mq2() > MQ2_LVL2) {
    return AIR_SENSOR_STATUS_NORMAL;
  } else if (read_mq135() > MQ135_LVL1 || read_mq2() > MQ2_LVL1) {
    return AIR_SENSOR_STATUS_NORMAL;
  } else if (read_mq135() > MQ135_LVL0 || read_mq2() > MQ2_LVL0) {
    return AIR_SENSOR_STATUS_GOOD;
  } else return AIR_SENSOR_STATUS_VERY_BAD;
}
void debug_air_sensor() {
  Serial.println("************************");
  Serial.println("MQ135 ppm");
  Serial.println(read_mq135());
  Serial.println("----------------------");
  Serial.println("MQ2 ppm");
  Serial.println(read_mq2());
  Serial.println("----------------------");
  Serial.println("AIR_SENSOR_STATUS SKALA 0 - 4");
  Serial.println(read_air_status());
  Serial.println("************************");
}
