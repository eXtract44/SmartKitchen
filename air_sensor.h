#ifndef AIR_SENSOR_H_
#define AIR_SENSOR_H_

#define MQ135_ANALOG_PIN 33  //GPIO33 MQ135 CO2 10 ... 10.000 ppm (0,1 ... 4 V) 0-2437 digit

#define MQ135_MIN_DIGIT 0 //digit
#define MQ135_MAX_DIGIT 4095 //digit
#define MQ135_MIN_PPM 10
#define MQ135_MAX_PPM 10000

#define MQ135_LVL0 10
#define MQ135_LVL1 50
#define MQ135_LVL2 200
#define MQ135_LVL3 300
#define MQ135_LVL4 500

#define MQ135_LVL_ALARM 3500

#define MQ2_ANALOG_PIN 32 //GPIO32 MQ2  RAUCH 300 - 10.000 ppm 0-2437 digit

#define MQ2_MIN_DIGIT 0 //digit
#define MQ2_MAX_DIGIT 4095 //digit
#define MQ2_MIN_PPM 300
#define MQ2_MAX_PPM 10000

#define MQ2_LVL0 311
#define MQ2_LVL1 600
#define MQ2_LVL2 1200
#define MQ2_LVL3 1800
#define MQ2_LVL4 2500

#define MQ2_LVL_ALARM 3500


enum air_status{
AIR_SENSOR_STATUS_GOOD,
AIR_SENSOR_STATUS_OK,
AIR_SENSOR_STATUS_NORMAL,
AIR_SENSOR_STATUS_BAD,
AIR_SENSOR_STATUS_VERY_BAD
};
#endif 