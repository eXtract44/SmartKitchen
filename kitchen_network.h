#pragma once
#include "mobile_ui.h"
const char KITCHEN_IMAGE_ID[32] __attribute__((section(".rodata_custom_desc"),used))="SmartKitchen:ESP32:layout-v2";
void startAP() {
  if(!apActive) apActive=WiFi.softAP(apName.c_str(),settings.admin.c_str(),1,false,2);
}
void startNetwork() {
  char suffix[7]; snprintf(suffix,sizeof(suffix),"%06X",(uint32_t)(ESP.getEfuseMac()&0xffffff)); apName=String("SmartKitchen-")+suffix;
  WiFi.persistent(false); WiFi.mode(WIFI_AP_STA); WiFi.setHostname("smartkitchen"); WiFi.setAutoReconnect(true);
  startAP(); if(settings.ssid.length()) WiFi.begin(settings.ssid.c_str(),settings.password.c_str());
  Serial.printf("Setup SSID: %s\nWiFi password: %s\n",apName.c_str(),settings.admin.c_str());
  Serial.printf("Image: %s\n",KITCHEN_IMAGE_ID);
  configTzTime("CET-1CEST,M3.5.0,M10.5.0/3","pool.ntp.org","time.nist.gov");
}
void serviceNetwork(uint32_t now) {
  static bool wasConnected=false;
  static uint32_t disconnectedAt=0,connectedAt=0,lastReconnect=0;
  bool connected=WiFi.status()==WL_CONNECTED;
  if(wifiChangeAt && int32_t(now-wifiChangeAt)>=0 && !otaBusy) {
    wifiChangeAt=0; startAP(); WiFi.disconnect(false,false); WiFi.begin(settings.ssid.c_str(),settings.password.c_str());
    wasConnected=false; disconnectedAt=now; connected=false;
  }
  if(connected) {
    if(!wasConnected) { connectedAt=now; mdnsActive=MDNS.begin("smartkitchen"); if(mdnsActive) MDNS.addService("http","tcp",80); Serial.print("Phone: http://"); Serial.println(WiFi.localIP()); }
    if(apActive && Kitchen::elapsed(now,connectedAt,60000) && !otaBusy) { WiFi.softAPdisconnect(true); apActive=false; }
    disconnectedAt=now;
  } else {
    if(wasConnected) { disconnectedAt=now; if(mdnsActive) MDNS.end(); mdnsActive=false; }
    if(Kitchen::elapsed(now,disconnectedAt,20000)) startAP();
    if(settings.ssid.length() && Kitchen::elapsed(now,lastReconnect,30000) && !otaBusy) { lastReconnect=now; WiFi.reconnect(); }
  }
  wasConnected=connected;
}
void jsonOK() { web.send(200,"application/json","{\"ok\":true}"); }
bool allowedHost() {
  String host=web.hostHeader(); host.toLowerCase();
  if(host.endsWith(":80")) host.remove(host.length()-3);
  return host=="smartkitchen.local" || host==WiFi.localIP().toString() || (apActive && host==WiFi.softAPIP().toString());
}
bool authenticateRequest(bool mutation=false) {
  web.sendHeader("Cache-Control","no-store"); web.sendHeader("X-Content-Type-Options","nosniff");
  if(!allowedHost()) { web.send(403,"text/plain","Host rejected"); return false; }
  if(mutation) {
    if(web.header("X-Kitchen-Control")!="1") { web.send(403,"text/plain","Missing control header"); return false; }
    String origin=web.header("Origin");
    if(origin.length() && origin!="http://"+web.hostHeader()) { web.send(403,"text/plain","Origin rejected"); return false; }
  }
  return true;
}
bool validID(const String &s,bool group=false) {
  if(s.isEmpty() || s.length()>20) return false;
  size_t start=group && s[0]=='-'?1:0; if(start==s.length()) return false;
  for(size_t i=start;i<s.length();i++) if(s[i]<'0'||s[i]>'9') return false;
  return s!="0";
}
bool intArg(const char* name,int low,int high,int &value) {
  String a=web.arg(name); if(a.isEmpty() || a.length()>6) return false;
  for(char c:a) if(c<'0'||c>'9') return false;
  value=a.toInt(); return value>=low && value<=high;
}
void sendStatus() {
  if(!authenticateRequest()) return;
  Snapshot s=snapshot(); TelegramStatus tg=telegramStatus();
  StaticJsonDocument<3072> doc;
  doc["version"]=VERSION; doc["ip"]=WiFi.status()==WL_CONNECTED?WiFi.localIP().toString():WiFi.softAPIP().toString();
  doc["uptime"]=millis()/1000; doc["heap"]=ESP.getFreeHeap(); doc["rssi"]=WiFi.status()==WL_CONNECTED?WiFi.RSSI():0;
  doc["motion"]=s.motion; doc["dhtValid"]=s.dhtValid;
  if(s.dhtValid) { doc["temperature"]=s.temperature; doc["humidity"]=s.humidity; }
  doc["gasReady"]=s.gasReady; doc["gas"]=s.gas; doc["warmupSeconds"]=millis()>=Kitchen::WARMUP_MS?0:(Kitchen::WARMUP_MS-millis())/1000;
  JsonArray water=doc.createNestedArray("water");
  for(int i=0;i<3;i++) { auto w=water.createNestedObject(); w["percent"]=s.water[i]; w["wet"]=s.wet[i]; w["fault"]=s.waterFault[i]; }
  auto mq=doc.createNestedArray("mq"); mq.add(s.mq[0]); mq.add(s.mq[1]);
  auto air=doc.createNestedObject("air"); air["stars"]=s.airStars; air["fault"]=s.mqFault[0]||s.mqFault[1]; air["stable"]=s.mqStable;
  auto base=air.createNestedArray("baseline"); base.add(s.mqBaseline[0]); base.add(s.mqBaseline[1]);
  auto smooth=air.createNestedArray("filtered"); smooth.add(s.mqFiltered[0]); smooth.add(s.mqFiltered[1]);
  auto display=doc.createNestedObject("display"); display["effectiveBrightness"]=effectiveBrightness; display["mode"]=brightnessMode;
  display["pwmOK"]=backlightPwmOK; display["pwmDuty"]=ledcRead(16); display["pwmHz"]=ledcReadFreq(16);
  doc["alarm"]=s.alarm; doc["alarmText"]=alarmDescription(s.alarm);
  auto bot=doc.createNestedObject("telegram"); bot["configured"]=tg.configured; bot["online"]=tg.online; bot["failures"]=tg.failures; bot["lastOk"]=tg.lastOk;
  auto cfg=doc.createNestedObject("settings"); cfg["brightness"]=settings.brightness; cfg["nightBrightness"]=settings.nightBrightness;
  cfg["nightStart"]=settings.nightStart; cfg["nightEnd"]=settings.nightEnd; cfg["night"]=settings.night;
  cfg["standby"]=settings.standby; cfg["autoSleep"]=settings.autoSleep; cfg["ssid"]=settings.ssid; cfg["chat"]=settings.chat; cfg["owner"]=settings.owner;
  String body; serializeJson(doc,body); web.send(200,"application/json",body);
}
bool otaAccepted=false,otaComplete=false;
String otaError;
size_t otaExpected=0,otaReceived=0,otaPrefixSize=0;
uint8_t otaPrefix[sizeof(esp_image_header_t)+sizeof(esp_image_segment_header_t)+sizeof(esp_app_desc_t)+sizeof(KITCHEN_IMAGE_ID)];
void failOTA(const String &error) { otaError=error; if(Update.isRunning()) Update.abort(); otaAccepted=false; otaBusy=false; }
bool startImage() {
  esp_image_header_t header; esp_app_desc_t desc;
  memcpy(&header,otaPrefix,sizeof(header)); memcpy(&desc,otaPrefix+sizeof(header)+sizeof(esp_image_segment_header_t),sizeof(desc));
  const uint8_t* identity=otaPrefix+sizeof(header)+sizeof(esp_image_segment_header_t)+sizeof(desc);
  if(header.magic!=ESP_IMAGE_HEADER_MAGIC || header.chip_id!=0 || desc.magic_word!=ESP_APP_DESC_MAGIC_WORD || memcmp(identity,KITCHEN_IMAGE_ID,sizeof(KITCHEN_IMAGE_ID))!=0) { failOTA("Wrong image: use SmartKitchen application for ESP32"); return false; }
  if(!Update.begin(otaExpected,U_FLASH)) { failOTA(Update.errorString()); return false; }
  if(Update.write(otaPrefix,sizeof(otaPrefix))!=sizeof(otaPrefix)) { failOTA(Update.errorString()); return false; }
  return true;
}
void receiveOTA() {
  HTTPUpload &u=web.upload();
  if(u.status==UPLOAD_FILE_START) {
    otaError=""; otaComplete=false; otaAccepted=false; otaReceived=0; otaPrefixSize=0;
    if(!allowedHost()) { otaError="Host rejected"; return; }
    String origin=web.header("Origin");
    if(web.header("X-Kitchen-Control")!="1" || (origin.length() && origin!="http://"+web.hostHeader())) { otaError="Forbidden"; return; }
    String size=web.header("X-Firmware-Size");
    if(size.isEmpty() || size.length()>8) { otaError="Missing image size"; return; }
    for(char c:size) if(c<'0'||c>'9') { otaError="Invalid size"; return; }
    otaExpected=size.toInt(); const esp_partition_t* part=esp_ota_get_next_update_partition(nullptr);
    if(!part || otaExpected<65536 || otaExpected>part->size) { otaError="Image does not fit OTA partition"; return; }
    if(snapshot().alarm) { otaError="Clear active sensor alarm before updating"; return; }
    otaAccepted=true; otaBusy=true;
  } else if(u.status==UPLOAD_FILE_WRITE && otaAccepted) {
    if(otaReceived+u.currentSize>otaExpected) { failOTA("Image exceeds declared size"); return; }
    otaReceived+=u.currentSize; size_t offset=0;
    if(otaPrefixSize<sizeof(otaPrefix)) {
      size_t n=min(u.currentSize,sizeof(otaPrefix)-otaPrefixSize); memcpy(otaPrefix+otaPrefixSize,u.buf,n); otaPrefixSize+=n; offset=n;
      if(otaPrefixSize==sizeof(otaPrefix) && !startImage()) return;
    }
    size_t rest=u.currentSize-offset;
    if(rest && Update.write(u.buf+offset,rest)!=rest) { failOTA(Update.errorString()); return; }
    if(snapshot().alarm) { failOTA("Sensor alarm: update cancelled"); return; }
    updateScreen(snapshot(),millis());
  } else if(u.status==UPLOAD_FILE_END && otaAccepted) {
    if(otaReceived!=otaExpected || otaPrefixSize!=sizeof(otaPrefix)) { failOTA("Incomplete image"); return; }
    if(!Update.end(false)) { failOTA(Update.errorString()); return; }
    otaComplete=true; otaAccepted=false; otaBusy=false;
  } else if(u.status==UPLOAD_FILE_ABORTED) failOTA("Upload interrupted");
}
void setupWeb() {
  const char* headers[]={"X-Kitchen-Control","Origin","X-Firmware-Size"}; web.collectHeaders(headers,3);
  web.on("/",HTTP_GET,[]{ if(!authenticateRequest()) return; web.sendHeader("X-Frame-Options","DENY"); web.sendHeader("Content-Security-Policy","default-src 'self'; script-src 'self' 'unsafe-inline'; style-src 'self' 'unsafe-inline'; frame-ancestors 'none'; object-src 'none'; base-uri 'none'"); web.send_P(200,"text/html; charset=utf-8",MOBILE_UI); });
  web.on("/api/status",HTTP_GET,sendStatus);
  web.on("/api/display/preview",HTTP_POST,[]{
    if(!authenticateRequest(true)) return;
    int value; if(!intArg("brightness",5,100,value)) { web.send(400,"text/plain","Invalid brightness"); return; }
    previewBrightness=value; previewAt=millis(); lastMotion=previewAt; jsonOK();
  });
  web.on("/api/air/baseline",HTTP_POST,[]{
    if(!authenticateRequest(true)) return;
    if(web.arg("confirm")!="1") { web.send(400,"text/plain","Fresh-air confirmation required"); return; }
    Snapshot s=snapshot();
    if(!s.gasReady||!s.mqStable||s.mqFault[0]||s.mqFault[1]||s.gas||Kitchen::elapsed(millis(),s.sampled,500)) { web.send(409,"text/plain","Wait for warm, stable sensors without gas alarm"); return; }
    if(prefs.putUShort("mqBase2",s.mqFiltered[0])!=sizeof(uint16_t)||prefs.putUShort("mqBase135",s.mqFiltered[1])!=sizeof(uint16_t)) { web.send(503,"text/plain","Cannot save baseline"); return; }
    portENTER_CRITICAL(&stateMux); for(int i=0;i<2;i++) referenceMq[i]=s.mqFiltered[i]; portEXIT_CRITICAL(&stateMux);
    jsonOK();
  });
  web.on("/api/display",HTTP_POST,[]{
    if(!authenticateRequest(true)) return;
    int bri,nightBri,standby,from,to;
    if(!intArg("brightness",5,100,bri)||!intArg("nightBrightness",1,100,nightBri)||!intArg("standby",10,3600,standby)||!intArg("nightStart",0,23,from)||!intArg("nightEnd",0,23,to)) { web.send(400,"text/plain","Invalid display settings"); return; }
    settings.brightness=bri; settings.nightBrightness=nightBri; settings.standby=standby; settings.nightStart=from; settings.nightEnd=to;
    settings.autoSleep=web.arg("autoSleep")=="1"; settings.night=web.arg("night")=="1"; saveDisplaySettings();
    previewBrightness=bri; previewAt=millis(); lastMotion=previewAt; jsonOK();
  });
  web.on("/api/command",HTTP_POST,[]{ if(!authenticateRequest(true)) return; Command c;
    if(web.arg("command")=="beep") c=BEEP; else if(web.arg("command")=="wake") c=WAKE; else { web.send(400,"text/plain","Unknown command"); return; }
    if(xQueueSend(commandQueue,&c,0)!=pdTRUE) { web.send(503,"text/plain","Busy"); return; } jsonOK();
  });
  web.on("/api/wifi",HTTP_POST,[]{
    if(!authenticateRequest(true)) return;
    String ssid=web.arg("ssid"),pass=web.arg("password");
    if(ssid.isEmpty()||ssid.length()>32||pass.length()>63||(pass.length()>0&&pass.length()<8)) { web.send(400,"text/plain","SSID or password invalid"); return; }
    settings.ssid=ssid; if(web.arg("open")=="1") settings.password=""; else if(pass.length()) settings.password=pass;
    prefs.putString("ssid",settings.ssid); prefs.putString("wifiPass",settings.password); wifiChangeAt=millis()+1500; jsonOK();
  });
  web.on("/api/telegram",HTTP_POST,[]{
    if(!authenticateRequest(true)) return;
    if(web.arg("disable")=="1") settings.token="";
    else {
      String token=web.arg("token"),chat=web.arg("chat"),owner=web.arg("owner");
      if(!validID(chat,true)||!validID(owner)) { web.send(400,"text/plain","Valid chat and owner IDs required"); return; }
      if(token.length()) {
        int colon=token.indexOf(':'); bool valid=colon>=6&&token.length()<121;
        for(size_t i=0;i<token.length();i++) if(!(isAlphaNumeric(token[i]) || token[i]==':' || token[i]=='_' || token[i]=='-')) valid=false;
        if(!valid) { web.send(400,"text/plain","Invalid bot token"); return; }
      } else if(settings.token.isEmpty()) { web.send(400,"text/plain","New token required"); return; }
      if(token.length()) settings.token=token; settings.chat=chat; settings.owner=owner;
    }
    prefs.putString("tgToken",settings.token); prefs.putString("tgChat",settings.chat); prefs.putString("tgOwner",settings.owner); queueTelegramConfig(); jsonOK();
  });
  web.on("/api/update",HTTP_POST,[]{
    if(!authenticateRequest(true)) return;
    if(!otaComplete) { web.send(400,"text/plain",otaError.length()?otaError:"No complete image"); return; }
    jsonOK(); otaComplete=false; rebootAt=millis()+1200;
  },receiveOTA);
  web.onNotFound([]{ if(authenticateRequest()) web.send(404,"text/plain","Not found"); });
  web.begin();
}
