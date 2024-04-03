
#define pir_sensor_pin 26

/*colors*/
#define pir_sensors_background ILI9341_BLACK
#define pir_sensors_color ILI9341_WHITE
/*colors*/

/*time values*/
constexpr uint16_t pir_delay = 30; // in sec
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
 
 void init_pir(){
  pinMode(pir_sensor_pin, INPUT); 
 }
bool read_pir_sensor(){
  return digitalRead(pir_sensor_pin);
}
bool active_pir_sensor(){
  return pir_sensor_feedback;
} 
void pir_sensor_to_display(){// 1 sec interval
static uint16_t cnt = 0;
cnt++;
  if (read_pir_sensor()) {
    set_brightness(100);
    tft.drawBitmap(x_start_pir, y_start_pir, pir_sensor, w_pir, h_pir, pir_sensors_background, pir_sensors_color);
    pir_sensor_feedback = 1;
    cnt=0;
  }else {
    if(cnt > pir_delay){
      cnt = pir_delay+1;
      set_brightness(0);
      pir_sensor_feedback = 0;     
    }
    tft.fillRect(260, 180, 53, 53, ILI9341_BLACK);
  }
}

void debug_pir_sensor() {
 if (digitalRead(pir_sensor_pin) == HIGH) {
    Serial.println("Somebody is here");   
    }else{
     Serial.println("Nobody is here"); 
    }
}
