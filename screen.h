#pragma once
// A small strip buffer avoids erase/print flicker without a full-screen framebuffer.
GFXcanvas16 strip(320,48);
String screenCache[7];
constexpr uint16_t BG=0x0843,CARD=0x10A5,INK=0xEF7D,MUTED=0x9CD3,COLOR_OK=0x66B3,WARN=0xFD8A,RED=0xFAAA;
void textAt(const char* text,int x,int y,uint16_t color,const GFXfont* font=nullptr) {
  strip.setFont(font); strip.setTextColor(color); strip.setCursor(x,y); strip.print(text);
}
void blit(int y,int height) { tft.drawRGBBitmap(0,y,strip.getBuffer(),320,height); }
void drawFrame() { tft.fillScreen(BG); for(String &v:screenCache) v=""; }
void updateScreen(const Snapshot &s,uint32_t now) {
  static uint32_t lastDraw=0;
  static int currentBrightness=0;
  int target=settings.brightness;
  struct tm local;
  if(settings.night && getLocalTime(&local,0) && Kitchen::nightHour(local.tm_hour,settings.nightStart,settings.nightEnd)) target=settings.nightBrightness;
  if(settings.autoSleep && Kitchen::elapsed(now,lastMotion,uint32_t(settings.standby)*1000) && !s.alarm && !apActive) target=0;
  if(s.alarm) target=max(target,70);
  int pwm=target*255/100;
  if(currentBrightness!=pwm) { currentBrightness+=constrain(pwm-currentBrightness,-3,3); ledcWrite(16,currentBrightness); }
  if(!Kitchen::elapsed(now,lastDraw,250)) return;
  lastDraw=now;
  String head=String(s.alarm)+":"+String(WiFi.status())+":"+String(apActive)+":"+String(otaBusy);
  if(head!=screenCache[0]) {
    screenCache[0]=head; strip.fillScreen(BG);
    textAt(s.alarm?"ALARM":"SMART KITCHEN",12,21,s.alarm?RED:INK,&FreeSansBold9pt7b);
    textAt(otaBusy?"UPDATE":WiFi.status()==WL_CONNECTED?"Wi-Fi":apActive?"SETUP":"OFFLINE",246,19,COLOR_OK);
    blit(0,30);
  }
  for(int i=0;i<3;i++) {
    String value=String(s.water[i])+":"+String(s.wet[i])+":"+String(s.waterFault[i]);
    if(screenCache[i+1]==value) continue;
    screenCache[i+1]=value; strip.fillScreen(BG); strip.fillRoundRect(6,1,308,29,5,CARD);
    textAt(WATER_NAMES[i],13,20,INK,&FreeSansBold9pt7b);
    char b[30]; snprintf(b,sizeof(b),"%s %d%%",s.waterFault[i]?"Pruefen":s.wet[i]?"WASSER":"Trocken",s.water[i]);
    textAt(b,220,12,s.wet[i]?RED:COLOR_OK); blit(30+i*31,31);
  }
  char env[90];
  if(s.dhtValid) snprintf(env,sizeof(env),"%.1f C | %.0f %%",s.temperature,s.humidity); else snprintf(env,sizeof(env),"-- C | -- %%");
  if(screenCache[4]!=env) {
    screenCache[4]=env; strip.fillScreen(BG);
    textAt("TEMPERATUR       FEUCHTE",12,2,MUTED);
    char b[20]; if(s.dhtValid) snprintf(b,sizeof(b),"%.1f",s.temperature); else strcpy(b,"--");
    textAt(b,14,41,INK,&FreeSansBold18pt7b); textAt("C",105,21,MUTED,&FreeSansBold9pt7b);
    if(s.dhtValid) snprintf(b,sizeof(b),"%.0f",s.humidity); else strcpy(b,"--");
    textAt(b,174,41,INK,&FreeSansBold18pt7b); textAt("%",260,21,MUTED,&FreeSansBold9pt7b); blit(129,48);
  }
  String air=String(s.gasReady)+":"+String(s.gas)+":"+String(s.mq[0])+":"+String(s.mq[1])+":"+String(s.motion);
  if(screenCache[5]!=air) {
    screenCache[5]=air; strip.fillScreen(BG);
    textAt(!s.gasReady?"LUFT: Aufwaermen":s.gas?"LUFT: Sensoralarm":"LUFT: Kein Sensoralarm",12,14,s.gas?RED:!s.gasReady?WARN:COLOR_OK,&FreeSansBold9pt7b);
    char b[64]; snprintf(b,sizeof(b),"MQ2 %d  MQ135 %d ADC  %s",s.mq[0],s.mq[1],s.motion?"PIR":""); textAt(b,12,22,MUTED); blit(180,32);
  }
  String footer=apActive?apName+"\n"+settings.admin:WiFi.status()==WL_CONNECTED?WiFi.localIP().toString():"Offline: Sensoren aktiv";
  if(!s.dhtValid && now>10000 && !apActive) footer="DHT: Sensor pruefen";
  if(screenCache[6]!=footer) {
    screenCache[6]=footer; strip.fillScreen(BG);
    if(apActive) {
      textAt((apName+"  192.168.4.1").c_str(),8,1,MUTED);
      textAt((String("WiFi: ")+settings.admin).c_str(),8,13,INK);
    } else textAt((footer+"  v"+VERSION).c_str(),8,9,MUTED);
    blit(214,26);
  }
}
