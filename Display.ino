
char buf[20];

void set_brightness(uint8_t bri){
 uint16_t temp_val = map(bri, 0, 100, 0, 255);
  ledcWrite(2, temp_val);  
}
void print_text(char *text,uint16_t x, uint16_t y){
   tft.setCursor(x, y);
   //tft.print("        ");
   tft.print(text);
}
void print_float(float value,uint16_t x, uint16_t y){
   tft.setCursor(x, y);
   sprintf(buf, "%.1f");
   tft.print("     ");
   tft.print(buf);
}
void print_int(int value, uint16_t x, uint16_t y){
   tft.setCursor(x, y);
   sprintf(buf, "%d");
   tft.print("     ");
   tft.print(buf);
}

void draw_menu(){
 tft.drawBitmap(0, 0, MENU, 320, 240, ILI9341_BLACK, ILI9341_WHITE); 
}
