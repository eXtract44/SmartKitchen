/* Setting PWM Properties */

#define ALARM_INTERVAL_BUZZER_OFF 10
#define ALARM_INTERVAL_BUZZER_ON 1

void buzzer_on() {
  ledcWrite(0, 10);  //on
}
void buzzer_off() {
  ledcWrite(0, 0);  //on
}
void alarm() {
  static uint16_t ALARM_CNT = 0;
  ALARM_CNT++;
  if (ALARM_CNT > ALARM_INTERVAL_BUZZER_OFF) {
    buzzer_on();
    if (ALARM_CNT > ALARM_INTERVAL_BUZZER_ON + ALARM_INTERVAL_BUZZER_OFF) {
      ALARM_CNT = 0;
      buzzer_off();
    }
  }
}

void no_alarm() {
  buzzer_off();
}
void beep() {
  ledcWrite(0, 10);
  delay(200);
  ledcWrite(0, 0);
}
