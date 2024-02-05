

#define PIR_SENSOR_PIN 26
#define PIR_SENSOR_INTERVAL 600 //in sec
uint16_t PIR_SENSOR_CNT = 0;
bool PIR_SENSOR_AKTIV = 0;

bool read_pir_sensor(){
  return digitalRead(PIR_SENSOR_PIN);
}
bool active_pir_sensor(){
  return PIR_SENSOR_AKTIV;
} 
void pir_sensor_to_display(){// 1 sec interval
  if (read_pir_sensor()) {
    set_brightness(100);
    tft.drawBitmap(260, 180, PIR_SEN, 53, 53, ILI9341_BLACK, ILI9341_WHITE);
    PIR_SENSOR_AKTIV = 1;
    PIR_SENSOR_CNT=0;
  }else {
    PIR_SENSOR_CNT++;
    if(PIR_SENSOR_CNT > PIR_SENSOR_INTERVAL){// 10 min
      PIR_SENSOR_CNT = PIR_SENSOR_INTERVAL+1;
      set_brightness(0);
      PIR_SENSOR_AKTIV = 0;     
      //
    }
    //PIR_SENSOR_AKTIV = false;
    tft.fillRect(260, 180, 53, 53, ILI9341_BLACK);
  }
}

void debug_pir_sensor() {
 if (digitalRead(PIR_SENSOR_PIN) == HIGH) {
    Serial.println("Somebody is here");   
    }else{
     Serial.println("Nobody is here"); 
    }
}
