
constexpr uint16_t alarm_interval_off = 5;
constexpr uint16_t alarm_interval_on = 1;
#define buzzer_sensor_pin 14

void init_buzzer(){
  ledcSetup(0, 5000, 8);  //channel,hz,bit setup PWM OUT for buzzer
  ledcAttachPin(buzzer_sensor_pin, 0);   //BUZZER_PIN, PWM_CHANNEL_BUZZER
}

void buzzer_on() {
  ledcWrite(0, 10);  //on
}
void buzzer_off() {
  ledcWrite(0, 0);  //on
}
void alarm() {
  static uint16_t cnt = 0;
  cnt++;
  if (cnt > alarm_interval_off) {
    buzzer_on();
    if (cnt > alarm_interval_on + alarm_interval_off) {
      cnt = 0;
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
