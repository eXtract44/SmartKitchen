/*colors*/
//#define temperature_sensors_background ILI9341_BLACK
#define temperature_sensors_color ILI9341_WHITE
/*colors*/

/*time values*/
constexpr uint16_t temperature_interval = 20; // in sec
/*time values*/

/*coordinates on diplay*/
constexpr uint16_t x_start_temperature = 24;
constexpr uint16_t x_start_humidity = 160;
constexpr uint16_t y_start_temperature = 220;
constexpr uint16_t y_start_humidity = 190;
/*coordinates on diplay*/

void init_dht(){
  dht.begin();
}
void temperature_sensor_to_display() {//1 sec inter
static uint16_t cnt = 0;
  cnt++;
  if (cnt > temperature_interval) {
    tft.setTextColor(temperature_sensors_color);
    tft.setFont(&FreeSansBold18pt7b);
    print_float(dht.readTemperature(),x_start_temperature, y_start_temperature);
    print_float(dht.readHumidity(),x_start_humidity, y_start_humidity);
    cnt = 0;
  }
  if(dht.readTemperature() > 35 || dht.readHumidity()>85){
    alarm();
  }else{
    no_alarm();
  }
}
void debug_temperature_sensor() {
  Serial.println(dht.readTemperature());
  Serial.println(dht.readHumidity());
}
