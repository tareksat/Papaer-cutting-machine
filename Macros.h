#define ActiveHigh 0
#define ActiveLow  1

#define enA digitalRead(EncoderA)
#define enB digitalRead(EncoderB)

#define HomeSW digitalRead(limit_sw1)
#define LowSpeedSW digitalRead(limit_sw2)
#define EndSW digitalRead(limit_sw3)

#define MF digitalRead(manualForward)
#define MB digitalRead(manualBackward)
#define MHS digitalRead(highSpeed)

#define FH_ON digitalWrite(Froward_High, HIGH)
#define FH_OFF digitalWrite(Froward_High, LOW)

#define FL_ON digitalWrite(Forward_Low, HIGH)
#define FL_OFF digitalWrite(Forward_Low, LOW)

#define BH_ON digitalWrite(Backward_High, HIGH)
#define BH_OFF digitalWrite(Backward_High, LOW)

#define BL_ON digitalWrite(Backward_Low, HIGH)
#define BL_OFF digitalWrite(Backward_Low, LOW)

#define motor_stop {FH_OFF; FL_OFF; BH_OFF; BL_OFF; chThdSleepMilliseconds(500); detachInterrupt(digitalPinToInterrupt(EncoderA));}

#define Buzzer_ON  digitalWrite(buzzer, HIGH)
#define Buzzer_OFF digitalWrite(buzzer, LOW)

#define LED_ON  digitalWrite(led, HIGH)
#define LED_OFF digitalWrite(led, LOW)

/******************************************************************************************************/
#define AngleAtHome         HomeSW == ActiveLow
#define AngleAtSW2      LowSpeedSW == ActiveLow
#define AngleAtEnd           EndSW == ActiveLow

#define M_Forward   MF  == ActiveLow
#define M_Backward  MB  == ActiveLow
#define M_HighSpeed MHS == ActiveLow

#define FL {FH_OFF; FL_ON; BH_OFF; BL_OFF; attachInterrupt(digitalPinToInterrupt(EncoderA), EncoderISR, RISING);}
#define FH {FH_ON; FL_OFF; BH_OFF; BL_OFF; attachInterrupt(digitalPinToInterrupt(EncoderA), EncoderISR, RISING);}
#define BL {FH_OFF; FL_OFF; BH_OFF; BL_ON; attachInterrupt(digitalPinToInterrupt(EncoderA), EncoderISR, RISING);}
#define BH {FH_OFF; FL_OFF; BH_ON; BL_OFF; attachInterrupt(digitalPinToInterrupt(EncoderA), EncoderISR, RISING);}

#define interrupt_debounce_delay 5

