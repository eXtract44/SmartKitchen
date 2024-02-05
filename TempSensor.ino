

#define TEMPERATURE_SENSOR_INTERVAL 20
#define TEMPERATURE_SENSOR_BACKGROUND ILI9341_BLACK 

constexpr uint16_t x_start_temperature = 24;
constexpr uint16_t x_start_humidity = 160;
constexpr uint16_t y_start_temperature = 220;
constexpr uint16_t y_start_humidity = 190;

void temperature_sensor_to_display() {//1 sec inter
static uint16_t TEMPERATURE_SENSOR_CNT = 0;
  TEMPERATURE_SENSOR_CNT++;
  if (TEMPERATURE_SENSOR_CNT > TEMPERATURE_SENSOR_INTERVAL) {
    tft.setTextColor(ILI9341_WHITE);
    tft.setFont(&FreeSansBold18pt7b);
    print_float(dht.readTemperature(),x_start_temperature, y_start_temperature);
    print_float(dht.readHumidity(),x_start_humidity, y_start_humidity);
    TEMPERATURE_SENSOR_CNT = 0;
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
