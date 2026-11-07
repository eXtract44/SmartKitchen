#pragma once
struct TelegramStatus { bool configured=false,online=false; uint32_t lastOk=0,failures=0; };
TelegramStatus tgStatus;
TelegramStatus telegramStatus() { TelegramStatus s; portENTER_CRITICAL(&stateMux); s=tgStatus; portEXIT_CRITICAL(&stateMux); return s; }
void setTelegramStatus(bool configured,bool online,bool failed=false) {
  portENTER_CRITICAL(&stateMux); tgStatus.configured=configured; tgStatus.online=online;
  if(online) tgStatus.lastOk=millis(); if(failed) tgStatus.failures++;
  portEXIT_CRITICAL(&stateMux);
}
String alarmDescription(uint16_t mask) {
  String out;
  for(int i=0;i<3;i++) if(mask&(1<<i)) out+=String("Wasser: ")+WATER_NAMES[i]+"\n";
  if(mask&8) out+="Hohe Temperatur\n"; if(mask&16) out+="Hohe Luftfeuchtigkeit\n"; if(mask&32) out+="MQ-Sensoralarm (nicht kalibriert)\n";
  return out;
}
String kitchenReport(const Snapshot &s) {
  String out="SmartKitchen "+String(VERSION)+"\n";
  out+=s.dhtValid?String(s.temperature,1)+" C / "+String(s.humidity,0)+" %\n":"DHT: Sensorfehler\n";
  for(int i=0;i<3;i++) out+=String(WATER_NAMES[i])+": "+(s.waterFault[i]?"Pruefen / ":s.wet[i]?"WASSER / ":"Trocken / ")+String(s.water[i])+" %\n";
  out+="MQ2: "+String(s.mq[0])+" ADC; MQ135: "+String(s.mq[1])+" ADC\n";
  out+=s.gasReady?"MQ aufgewaermt; Werte nicht als ppm kalibriert\n":"MQ: Aufwaermen\n";
  out+=s.alarm?alarmDescription(s.alarm):"Kein aktiver Sensoralarm\n";
  return out;
}
// Filter metadata instead of storing arbitrary Telegram entities/files in RAM.
// Oversized text is skipped after acknowledging its update_id, so it cannot wedge polling.
int pollTelegram(UniversalTelegramBot &bot,WiFiClientSecure &tls,const TelegramConfig &config) {
  HTTPClient http; http.setConnectTimeout(4000); http.setTimeout(4000); http.useHTTP10(true); http.setReuse(false);
  String url="https://api.telegram.org/bot"+String(config.token)+"/getUpdates?offset="+String(bot.last_message_received+1)+"&limit=1&timeout=0&allowed_updates=%5B%22message%22%5D";
  if(!http.begin(tls,url)) return -1;
  int code=http.GET(); if(code!=200) { http.end(); return -1; }
  StaticJsonDocument<512> filter;
  filter["ok"]=true;
  auto result=filter["result"][0]; result["update_id"]=true;
  result["message"]["chat"]["id"]=true; result["message"]["from"]["id"]=true; result["message"]["text"]=true;
  DynamicJsonDocument doc(4096);
  DeserializationError err=deserializeJson(doc,http.getStream(),DeserializationOption::Filter(filter),DeserializationOption::NestingLimit(12));
  http.end();
  if(!doc["ok"].as<bool>()) return -1;
  JsonObject update=doc["result"][0];
  if(update.isNull()) return err?-1:0;
  if(!update["update_id"].is<long>()) return -1;
  bot.last_message_received=update["update_id"].as<long>();
  if(err) return 0;
  JsonObject message=update["message"];
  if(message.isNull()) return 0;
  String chat=message["chat"]["id"].as<String>(),owner=message["from"]["id"].as<String>();
  if(chat!=config.chat || owner!=config.owner) return 0;
  const char* text=message["text"]|"";
  if(strlen(text)>128) return 0;
  bot.messages[0].chat_id=chat; bot.messages[0].from_id=owner; bot.messages[0].text=text;
  return 1;
}
void telegramWorker(void*) {
  WiFiClientSecure tls;
  tls.setCACert(TELEGRAM_CERTIFICATE_ROOT); tls.setTimeout(4000); tls.setHandshakeTimeout(5);
  UniversalTelegramBot* bot=nullptr;
  TelegramConfig config={};
  uint32_t lastPoll=0,lastAttempt=0,lastReminder=0,lastCommand=0;
  uint32_t lastHealth=0;
  bool healthy=false;
  bool webhookCleared=false;
  uint16_t deliveredAlarm=0;
  bool dhtFaultDelivered=false;
  bool pending=false;
  AlarmEvent event={};
  for(;;) {
    TelegramConfig incoming;
    if(xQueueReceive(tgConfigQueue,&incoming,0)==pdTRUE) {
      tls.stop(); delete bot; bot=nullptr; config=incoming;
      if(strlen(config.token)>20 && strlen(config.chat)>0 && strlen(config.owner)>0) {
        bot=new UniversalTelegramBot(config.token,tls); bot->longPoll=0; bot->waitForResponse=1500;
      }
      deliveredAlarm=0; dhtFaultDelivered=false;
      pending=false;
      healthy=false; lastHealth=0;
      webhookCleared=false;
      setTelegramStatus(bot!=nullptr,false);
    }
    if(!bot || WiFi.status()!=WL_CONNECTED || time(nullptr)<1700000000 || otaBusy) {
      if(bot) setTelegramStatus(true,false);
      vTaskDelay(pdMS_TO_TICKS(250)); continue;
    }
    uint32_t now=millis(); const Snapshot s=snapshot();
    if(lastHealth==0 || Kitchen::elapsed(now,lastHealth,60000)) {
      lastHealth=now; healthy=bot->getMe();
      if(healthy && !webhookCleared) {
        String response=bot->sendGetToTelegram(bot->buildCommand("deleteWebhook"));
        StaticJsonDocument<512> result;
        webhookCleared=!deserializeJson(result,response) && result["ok"].as<bool>() && result["result"].as<bool>();
        tls.stop(); healthy=webhookCleared;
      }
      setTelegramStatus(true,healthy,!healthy);
    }
    if(!healthy) { vTaskDelay(pdMS_TO_TICKS(250)); continue; }
    // Notifications are independent from incoming commands; retry on failed delivery.
    bool dhtFault=!s.dhtValid && now>15000;
    if(!pending) pending=xQueueReceive(alarmQueue,&event,0)==pdTRUE;
    if(!pending && (s.alarm!=deliveredAlarm || dhtFault!=dhtFaultDelivered)) { event={s.alarm,dhtFault,now}; pending=true; }
    bool remind=s.alarm && Kitchen::elapsed(now,lastReminder,300000);
    if(!pending && remind) { event={s.alarm,dhtFault,now}; pending=true; }
    if(pending && Kitchen::elapsed(now,lastAttempt,10000)) {
      lastAttempt=now;
      String message="SmartKitchen Ereignis (vor "+String(uint32_t(now-event.at)/1000)+" s)\n";
      message+=event.alarm?"ALARM\n"+alarmDescription(event.alarm):"Alarmbedingungen nicht mehr aktiv.\n";
      if(event.dhtFault) message+="DHT: Messung ausgefallen.\n";
      else if(dhtFaultDelivered) message+="DHT: Messung wieder verfuegbar.\n";
      message+="\nAktueller Zustand:\n"+(s.alarm?alarmDescription(s.alarm):String("Kein Sensoralarm\n"));
      bool ok=bot->sendMessage(config.chat,message,"");
      setTelegramStatus(true,ok,!ok);
      if(ok) { deliveredAlarm=event.alarm; dhtFaultDelivered=event.dhtFault; lastReminder=now; pending=false; }
    }
    if(Kitchen::elapsed(now,lastPoll,2500)) {
      lastPoll=now;
      int n=pollTelegram(*bot,tls,config);
      setTelegramStatus(true,n>=0,n<0);
      for(int i=0;i<n && i<10;i++) {
        auto &m=bot->messages[i];
        if(m.chat_id!=config.chat || m.from_id!=config.owner) continue;
        if(!Kitchen::elapsed(millis(),lastCommand,2000)) continue;
        lastCommand=millis();
        String text=m.text; int at=text.indexOf('@'); if(at>0) text=text.substring(0,at);
        String response;
        if(text=="/start" || text=="/help") response="SmartKitchen\n/check_kitchen — Sensoren\n/beep — Testton\n/standby10 /standby60 — Displaytimer\n/wake — Display aktivieren";
        else if(text=="/check_kitchen" || text=="/status") response=kitchenReport(snapshot());
        else {
          Command c;
          if(text=="/beep") c=BEEP; else if(text=="/standby10") c=STANDBY10; else if(text=="/standby60") c=STANDBY60; else if(text=="/wake") c=WAKE; else continue;
          response=xQueueSend(commandQueue,&c,0)==pdTRUE?"OK":"Geraet beschaeftigt";
        }
        if(response.length()) { bool sent=bot->sendMessage(config.chat,response,""); setTelegramStatus(true,sent,!sent); }
      }
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
