
#if (_DISPLAY == 1)

uint16_t standby_time = 10;
void init_disp() {

  ledcSetup(2, 5000, 8);  //channel,hz,bit setup PWM OUT for display
  ledcAttachPin(16, 2);   //LED_PIN, PWM_CHANNEL_LED
  set_brightness(50);
  tft.begin();
  tft.setRotation(3);
  //tft.fillScreen(ILI9341_WHITE);
  draw_menu();
}

char buf[30] = { 0 };
void set_standby_time(uint16_t time){
  standby_time = time;
}
void set_brightness(uint8_t bri) {
  uint16_t temp_val = map(bri, 0, 100, 0, 255);
  ledcWrite(2, temp_val);
}
void set_brightness_pir() {
  static uint16_t cnt = 0;
  if(read_pir_sensor_delayed() == true) {
    set_brightness(100);
    cnt = 0;
  }
  else {
    cnt++;
    if (cnt > standby_time) {
      set_brightness(0);
      cnt = standby_time+1;
    }
  }
}
void print_text(char* text, int16_t x, int16_t y) {
  tft.setCursor(x, y);
  tft.print(text);
}
void print_value(float value, char* format, int16_t x, int16_t y, int16_t w_bg, int16_t h_bg, int16_t bg_y_off) {
  tft.fillRect(x, y - bg_y_off, w_bg, h_bg, ILI9341_BLACK);
  sprintf(buf, format, value);
  tft.setCursor(x, y);
  tft.print(buf);
}
void print_value_9(float value, char* format, int16_t x, int16_t y) {
  tft.setFont(&FreeSansBold9pt7b);
  print_value(value, format, x, y, 48, 16, 15);
}
void print_value_12(float value, char* format, int16_t x, int16_t y) {
  tft.setFont(&FreeSansBold12pt7b);
  print_value(value, format, x, y, 70, 21, 20);
}
void print_value_18(float value, char* format, int16_t x, int16_t y) {
  tft.setFont(&FreeSansBold18pt7b);
  print_value(value, format, x, y, 70, 35, 30);
}
void draw_menu() {
  //tft.fillScreen(ILI9341_YELLOW);
  tft.drawBitmap(0, 0, menu_main, 320, 240, ILI9341_BLACK, ILI9341_WHITE);
}
#endif