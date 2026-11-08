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
void drawNumberWithUnit(const char* value,int x,int baseline,bool temperature) {
  int16_t bx,by; uint16_t w,h;
  strip.setFont(&FreeSansBold18pt7b); strip.getTextBounds(value,x,baseline,&bx,&by,&w,&h);
  // Both units stay inside their 148px column, including -40.0 and 100%.
  if(w>105) { strip.setFont(&FreeSansBold12pt7b); strip.getTextBounds(value,x,baseline,&bx,&by,&w,&h); }
  strip.setCursor(x,baseline); strip.setTextColor(INK); strip.print(value);
  int unitX=bx+w+8;
  if(temperature) { strip.drawCircle(unitX+2,baseline-14,2,MUTED); textAt("C",unitX+7,baseline,MUTED,&FreeSansBold9pt7b); }
  else textAt("%",unitX,baseline,MUTED,&FreeSansBold9pt7b);
}
void drawStar(int x,int y,bool filled,uint16_t color) {
  const int8_t px[]={0,2,8,4,5,0,-5,-4,-8,-2};
  const int8_t py[]={-8,-3,-3,1,7,4,7,1,-3,-3};
  for(int i=0;i<10;i++) { int j=(i+1)%10;
    if(filled) strip.fillTriangle(x,y,x+px[i],y+py[i],x+px[j],y+py[j],color);
    else strip.drawLine(x+px[i],y+py[i],x+px[j],y+py[j],MUTED);
  }
}
void drawPerson(int x,int frame) {
  strip.fillCircle(x,5,3,COLOR_OK); strip.drawLine(x,9,x,18,COLOR_OK);
  int step=frame?5:-5;
  strip.drawLine(x,11,x+step,16,COLOR_OK); strip.drawLine(x,11,x-step,14,COLOR_OK);
  strip.drawLine(x,18,x+step,26,COLOR_OK); strip.drawLine(x,18,x-step,24,COLOR_OK);
}
void updateScreen(const Snapshot &s,uint32_t now) {
  static uint32_t lastDraw=0;
  static int currentBrightness=0;
  static uint32_t lastFade=0;
  int target=settings.brightness; brightnessMode="manual";
  struct tm local;
  if(settings.night && getLocalTime(&local,0) && Kitchen::nightHour(local.tm_hour,settings.nightStart,settings.nightEnd)) { target=settings.nightBrightness; brightnessMode="night"; }
  if(previewBrightness>=0 && !Kitchen::elapsed(now,previewAt,30000)) { target=previewBrightness; brightnessMode="preview"; }
  else previewBrightness=-1;
  if(settings.autoSleep && previewBrightness<0 && Kitchen::elapsed(now,lastMotion,uint32_t(settings.standby)*1000) && !s.alarm && !apActive) { target=0; brightnessMode="standby"; }
  if(s.alarm) { target=max(target,70); brightnessMode="alarm"; }
  effectiveBrightness=target;
  int pwm=target*255/100;
  if(Kitchen::elapsed(now,lastFade,10)) {
    lastFade=now;
    if(currentBrightness!=pwm) { currentBrightness+=constrain(pwm-currentBrightness,-3,3); ledcWrite(16,currentBrightness); }
  }
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
    textAt("TEMPERATUR",12,2,MUTED); textAt("FEUCHTE",174,2,MUTED);
    char b[20]; if(s.dhtValid) snprintf(b,sizeof(b),"%.1f",s.temperature); else strcpy(b,"--");
    drawNumberWithUnit(b,12,41,true);
    if(s.dhtValid) snprintf(b,sizeof(b),"%.0f",s.humidity); else strcpy(b,"--");
    drawNumberWithUnit(b,174,41,false); blit(129,48);
  }
  static bool pirSeen=false; static uint32_t pirAt=0;
  if(s.motion) { pirSeen=true; pirAt=now; }
  bool pirVisible=pirSeen && !Kitchen::elapsed(now,pirAt,3000);
  int frame=pirVisible?(now/500)%2:-1;
  String air=String(s.gasReady)+":"+String(s.gas)+":"+String(s.mqFault[0])+String(s.mqFault[1])+":"+String(s.airStars)+":"+String(frame);
  if(screenCache[5]!=air) {
    screenCache[5]=air; strip.fillScreen(BG);
    bool fault=s.mqFault[0]||s.mqFault[1];
    textAt(s.gas?"LUFT: ALARM":"LUFTBELASTUNG",12,13,s.gas?RED:INK,&FreeSansBold9pt7b);
    if(s.gas) textAt(s.mq[0]>=GAS_OFF[0]?"MQ2: Rauch / Gas":"MQ135: starke Reaktion",12,22,RED);
    else if(!s.gasReady) textAt("Aufwaermen",12,22,WARN);
    else if(fault) textAt("MQ-Sensor pruefen",12,22,WARN);
    else if(!s.airStars) textAt("Basis im Web setzen",12,22,WARN);
    else {
      uint16_t color=s.airStars>=4?COLOR_OK:s.airStars>=3?WARN:RED;
      textAt(s.airStars==5?"Basisniveau":s.airStars>=3?"Erhoeht":"Lueften",12,22,color);
      for(int i=0;i<5;i++) drawStar(194+i*20,11,i<s.airStars,color);
    }
    if(pirVisible) drawPerson(300,frame);
    blit(180,32);
  }
  String footer=apActive?apName+"\n"+settings.admin:WiFi.status()==WL_CONNECTED?WiFi.localIP().toString():"Offline: Sensoren aktiv";

  if(screenCache[6]!=footer) {
    screenCache[6]=footer; strip.fillScreen(BG);
    if(apActive) {
      textAt((apName+"  192.168.4.1").c_str(),8,1,MUTED);
      textAt((String("WiFi: ")+settings.admin).c_str(),8,13,INK);
    } else textAt((footer+"  v"+VERSION).c_str(),8,9,MUTED);
    blit(214,26);
  }
}
