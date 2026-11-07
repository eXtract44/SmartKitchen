// SmartKitchen 2.0 — ESP32 Dev Module, Arduino-ESP32 3.3.5
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <ESPmDNS.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <Update.h>
#include <esp_ota_ops.h>
#include <esp_app_format.h>
#include <esp_app_desc.h>
#include <DHT.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <time.h>
#include "logic.h"
#if __has_include("secrets.h")
#include "secrets.h"
#else
#define INITIAL_WIFI_SSID ""
#define INITIAL_WIFI_PASSWORD ""
#endif

constexpr char VERSION[]="2.0.0";
constexpr int WATER_PINS[]={39,34,35}, WET_RAW[]={1700,1650,1900};
constexpr int MQ_PINS[]={32,33};
// Same raw thresholds as the former linear mapping at 3500; not calibrated ppm.
constexpr int GAS_ON[]={804,851}, GAS_OFF[]={724,766};
const char* const WATER_NAMES[]={"Wasseranschluss","Waschmaschine","Kuehlschrank"};
DHT dht(25,DHT21);
Adafruit_ILI9341 tft(22,21,17); // hardware SPI: SCK18, MOSI23
Preferences prefs;
WebServer web(80);
struct Settings {
  String ssid, password, token, chat, owner, admin;
  uint8_t brightness=65, nightBrightness=15, nightStart=22, nightEnd=7;
  uint16_t standby=60;
  bool autoSleep=true, night=true;
} settings;
struct Snapshot {
  uint32_t sampled=0,dhtOkAt=0,uptime=0;
  int water[3]={0,0,0},mq[2]={0,0};
  bool wet[3]={false,false,false},waterFault[3]={false,false,false};
  bool dhtValid=false,motion=false,gasReady=false,gas=false;
  float temperature=NAN,humidity=NAN;
  uint16_t alarm=0;
};
Snapshot sharedState;
portMUX_TYPE stateMux=portMUX_INITIALIZER_UNLOCKED;
QueueHandle_t tgConfigQueue,commandQueue,alarmQueue;
struct AlarmEvent { uint16_t alarm; bool dhtFault; uint32_t at; };
uint32_t droppedEvents=0;
struct TelegramConfig { char token[128],chat[32],owner[32]; };
enum Command : uint8_t { BEEP=1,STANDBY10,STANDBY60,WAKE };
volatile bool otaBusy=false;
bool apActive=false,mdnsActive=false;
String apName;
uint32_t rebootAt=0,wifiChangeAt=0;
volatile uint32_t lastMotion=0,beepUntil=0;
bool preferencesReady=false,bootMarked=false;

Snapshot snapshot() { Snapshot s; portENTER_CRITICAL(&stateMux); s=sharedState; portEXIT_CRITICAL(&stateMux); return s; }
void publish(const Snapshot &s) { portENTER_CRITICAL(&stateMux); sharedState=s; portEXIT_CRITICAL(&stateMux); }
void queueTelegramConfig() {
  TelegramConfig c={};
  settings.token.toCharArray(c.token,sizeof(c.token)); settings.chat.toCharArray(c.chat,sizeof(c.chat)); settings.owner.toCharArray(c.owner,sizeof(c.owner));
  xQueueOverwrite(tgConfigQueue,&c);
}
void loadSettings() {
  preferencesReady=prefs.begin("kitchen2",false);
  settings.ssid=prefs.getString("ssid",INITIAL_WIFI_SSID); settings.password=prefs.getString("wifiPass",INITIAL_WIFI_PASSWORD);
  settings.token=prefs.getString("tgToken",""); settings.chat=prefs.getString("tgChat",""); settings.owner=prefs.getString("tgOwner","");
  settings.admin=prefs.getString("admin","");
  if(settings.admin.length()<12) {
    char p[17]; snprintf(p,sizeof(p),"%08lX%08lX",(unsigned long)esp_random(),(unsigned long)esp_random()); settings.admin=p;
    prefs.putString("admin",settings.admin);
  }
  settings.brightness=constrain(prefs.getUChar("bri",65),5,100); settings.nightBrightness=constrain(prefs.getUChar("nightBri",15),1,100);
  settings.standby=constrain(prefs.getUShort("sleepSec",60),10,3600); settings.autoSleep=prefs.getBool("autoSleep",true);
  settings.night=prefs.getBool("night",true); settings.nightStart=constrain(prefs.getUChar("nightFrom",22),0,23); settings.nightEnd=constrain(prefs.getUChar("nightTo",7),0,23);
}
void saveDisplaySettings() {
  prefs.putUChar("bri",settings.brightness); prefs.putUChar("nightBri",settings.nightBrightness); prefs.putUShort("sleepSec",settings.standby);
  prefs.putBool("autoSleep",settings.autoSleep); prefs.putBool("night",settings.night); prefs.putUChar("nightFrom",settings.nightStart); prefs.putUChar("nightTo",settings.nightEnd);
}

#include "screen.h"
#include "telegram_worker.h"
#include "kitchen_network.h"

void sampleSensors(uint32_t now) {
  static uint32_t lastAnalog=0,lastDht=0;
  static Kitchen::WaterLatch waterLatch[3];
  Snapshot s=snapshot(); s.uptime=now;
  s.motion=digitalRead(26); if(s.motion) lastMotion=now;
  if(Kitchen::elapsed(now,lastAnalog,100)) {
    lastAnalog=now;
    for(int i=0;i<3;i++) {
      int raw=0; for(int j=0;j<8;j++) raw+=analogRead(WATER_PINS[i]); raw/=8;
      s.water[i]=Kitchen::waterPercent(raw,WET_RAW[i]);
      // Rail-low is a wiring fault OR extreme wetness; keep alarm active either way.
      s.waterFault[i]=raw<8;
      s.wet[i]=waterLatch[i].update(s.water[i],now);
    }
    for(int i=0;i<2;i++) { int raw=0; for(int j=0;j<8;j++) raw+=analogRead(MQ_PINS[i]); s.mq[i]=raw/8; }
    s.gasReady=s.gasReady || now>=Kitchen::WARMUP_MS;
    s.gas=s.gasReady && (Kitchen::hysteresis(s.gas,s.mq[0],GAS_ON[0],GAS_OFF[0]) || Kitchen::hysteresis(s.gas,s.mq[1],GAS_ON[1],GAS_OFF[1]));
    s.sampled=now;
  }
  if(Kitchen::elapsed(now,lastDht,2500)) {
    lastDht=now;
    float t=dht.readTemperature(),h=dht.readHumidity();
    s.dhtValid=isfinite(t)&&isfinite(h)&&t>=-40&&t<=80&&h>=0&&h<=100;
    if(s.dhtValid) { s.temperature=t; s.humidity=h; s.dhtOkAt=now; }
  }
  static bool hot=false,humid=false;
  if(s.dhtValid) { hot=hot?s.temperature>33:s.temperature>=35; humid=humid?s.humidity>80:s.humidity>=85; }
  // A failed reading must not clear a previously measured hazardous condition.
  s.alarm=Kitchen::alarms(s.wet,hot,humid,s.gas);
  publish(s);
  static uint16_t previousAlarm=0;
  static bool previousFault=false;
  bool fault=!s.dhtValid && now>15000;
  if(s.alarm!=previousAlarm || fault!=previousFault) {
    AlarmEvent event={s.alarm,fault,now};
    if(xQueueSend(alarmQueue,&event,0)!=pdTRUE) {
      AlarmEvent oldest; xQueueReceive(alarmQueue,&oldest,0); xQueueSend(alarmQueue,&event,0);
      portENTER_CRITICAL(&stateMux); droppedEvents++; portEXIT_CRITICAL(&stateMux);
    }
    previousAlarm=s.alarm; previousFault=fault;
  }
}

void sensorWorker(void*) {
  TickType_t wake=xTaskGetTickCount();
  for(;;) {
    uint32_t now=millis(); sampleSensors(now);
    const Snapshot s=snapshot();
    bool beepActive=beepUntil!=0 && int32_t(beepUntil-now)>0;
    if(!beepActive) beepUntil=0;
    ledcWrite(14,Kitchen::buzzer(now,s.alarm!=0) || beepActive?10:0);
    vTaskDelayUntil(&wake,pdMS_TO_TICKS(20));
  }
}

void setup() {
  Serial.begin(115200); delay(80);
  pinMode(26,INPUT); analogReadResolution(12);
  for(int p:WATER_PINS) { pinMode(p,INPUT); analogSetPinAttenuation(p,ADC_11db); }
  for(int p:MQ_PINS) { pinMode(p,INPUT); analogSetPinAttenuation(p,ADC_11db); }
  ledcAttach(14,5000,8); ledcWrite(14,0);
  ledcAttach(16,5000,8); ledcWrite(16,0);
  SPI.begin(18,-1,23,22); tft.begin(20000000); tft.setRotation(3); tft.setTextWrap(false);
  dht.begin(); loadSettings();
  tgConfigQueue=xQueueCreate(1,sizeof(TelegramConfig)); commandQueue=xQueueCreate(8,sizeof(Command)); alarmQueue=xQueueCreate(16,sizeof(AlarmEvent));
  if(!tgConfigQueue || !commandQueue || !alarmQueue || !strip.getBuffer()) { Serial.println("FATAL: allocation"); while(true) { delay(1000); } }
  drawFrame(); lastMotion=millis();
  if(xTaskCreatePinnedToCore(sensorWorker,"sensors",4096,nullptr,2,nullptr,1)!=pdPASS) {
    Serial.println("FATAL: sensor task allocation"); ESP.restart();
  }
  startNetwork(); setupWeb(); queueTelegramConfig();
  if(xTaskCreatePinnedToCore(telegramWorker,"telegram",14336,nullptr,1,nullptr,0)!=pdPASS) Serial.println("Telegram task failed");
  Serial.printf("SmartKitchen %s ready; local sensors run without Wi-Fi.\n",VERSION);
}
void loop() {
  uint32_t now=millis();
  Command c;
  while(xQueueReceive(commandQueue,&c,0)==pdTRUE) {
    if(c==BEEP) beepUntil=now+200;
    if(c==WAKE) lastMotion=now;
    if(c==STANDBY10 || c==STANDBY60) { settings.standby=c==STANDBY10?10:60; saveDisplaySettings(); }
  }
  const Snapshot s=snapshot();
  serviceNetwork(now); updateScreen(s,now);
  web.handleClient();
  if(otaBusy) { // WebServer calls this loop between upload chunks; progress is also serviced in the callback.
    yield();
  }
  if(rebootAt && int32_t(now-rebootAt)>=0) ESP.restart();
  if(!bootMarked && now>30000 && s.sampled && preferencesReady) { esp_ota_mark_app_valid_cancel_rollback(); bootMarked=true; }
  static uint32_t lastDiagnostic=0;
  if(Kitchen::elapsed(now,lastDiagnostic,10000)) {
    lastDiagnostic=now;
    Serial.printf("Status: uptime=%lu heap=%lu alarm=%u DHT=%s water=%d/%d/%d MQ=%d/%d\n",(unsigned long)(now/1000),(unsigned long)ESP.getFreeHeap(),s.alarm,s.dhtValid?"ok":"fault",s.water[0],s.water[1],s.water[2],s.mq[0],s.mq[1]);
  }
  delay(2);
}
