
#include <ChibiOS_AVR.h>
#include "IO_Def.h"
#include "Macros.h"

volatile long counts = 0;
volatile long low_speed_counts = 0;
volatile long target_counts = 0;
volatile long countCorrection = 0;

bool auto_mode_flag = false;
bool manual_mode_flag = false;


SEMAPHORE_DECL(auto_position_sem, 0);

/*
    1. Thread for receiving serial commands
    2. Thread for monitoring manual switches
    3. Thread for auto positioning
    4. Thread for sending serial communication
    5. Thread for flashing LED
*/

/*------------------------------------------------------------------------------
                 Thread 1, Thread for receiving serial commands
------------------------------------------------------------------------------*/
// 64 byte stack beyond task switch and interrupt needs
static THD_WORKING_AREA(waThread1, 64);

static THD_FUNCTION(Thread1, arg) {
	char RxBuffer[20];
	char command;
	bool rxFlag = false;
    int position = 0;

	while (1) {
		chThdSleepMilliseconds(5);
        while(Serial.available()){
            command = (char) Serial.read();
            if(command != '#'){
                // still receiving the data
                RxBuffer[position] = command;
                position++;
            }
            else{
                // End of the received frame
                // start data analysis
                SerialCommandAnalysis(RxBuffer, position);
                position = 0;
            }
        }
		
	}
}

/*------------------------------------------------------------------------------
                 Thread 2, for monitoring manual switches
------------------------------------------------------------------------------*/
// 64 byte stack beyond task switch and interrupt needs
static THD_WORKING_AREA(waThread2, 64);

static THD_FUNCTION(Thread2, arg) {

  while (1) {
       chThdSleepMilliseconds(200);
       /*
       1. if FL ONLY Turn FL ON and monitor sw3 if activated turn FL BH_OFF
       2. if FL & HS Turn FH ON and monitor sw2 ih activated witch to FL and go to step 1
       3. if BL turn BL ON and monitor sw1 if ON turn it BH_OFF
       4. if BH turn BH ON and monitor low_speed_counts if achived goto step 3
       */
      if(!auto_mode_flag && manual_mode_flag){
          /******************   BACKWARD MOVEMENT   ***************************/
          if(M_Backward && !M_HighSpeed && !AngleAtHome){
                BL;
            }
            else if(M_Backward && M_HighSpeed && !AngleAtHome){
                BH;
            }

            /******************   FORWARD MOVEMENT   ***************************/
            else if(M_Forward && !M_HighSpeed && !AngleAtEnd){
                FL;
            }

            else if(M_Forward && M_HighSpeed && AngleAtSW2 && !AngleAtEnd){
                FL;
            }

            else if(M_Forward && M_HighSpeed && !AngleAtSW2 && !AngleAtEnd){
                FH;
            }

            else{
                motor_stop;
        }
      }
  }
}

/*------------------------------------------------------------------------------
                 Thread 3, auto positioning
------------------------------------------------------------------------------*/
// 64 byte stack beyond task switch and interrupt needs
static THD_WORKING_AREA(waThread3, 64);

static THD_FUNCTION(Thread3, arg) {

  while (1) {
        if(auto_mode_flag){
            if(target_counts == 0){    // for home position
                moveAngle(5, 0);
                auto_mode_flag = false;
                beep();
            }

            else if(target_counts > counts){   // move forward
                moveAngle(1, (target_counts - low_speed_counts));
                moveAngle(2, target_counts);
                auto_mode_flag = false;
                beep();
            }

            else if(target_counts < counts){  // move backward
                if(target_counts - low_speed_counts <= 0){
                    moveAngle(5, 0);
                    moveAngle(2, target_counts);
                    auto_mode_flag = false;
                    beep();
                }

                else{
                    moveAngle(3, (target_counts - low_speed_counts));
                    moveAngle(2, target_counts);
                    auto_mode_flag = false;
                    beep();
                }
            }
         }
        
        
        chThdSleepMilliseconds(200);
  }
}

/*------------------------------------------------------------------------------
                 Thread 4, sending serial communication
------------------------------------------------------------------------------*/
// 64 byte stack beyond task switch and interrupt needs
static THD_WORKING_AREA(waThread4, 64);

static THD_FUNCTION(Thread4, arg) {

  while (1) {
      chThdSleepMilliseconds(300);
      if(auto_mode_flag || manual_mode_flag){
          Serial.print("$");
          Serial.print(counts);
          Serial.println("*");
      }
       
  }
}

/*------------------------------------------------------------------------------
                 Thread 5, flashing LED
------------------------------------------------------------------------------*/
// 64 byte stack beyond task switch and interrupt needs
static THD_WORKING_AREA(waThread5, 64);

static THD_FUNCTION(Thread5, arg) {

    int long_time = 300;
    int short_time = 50;
    while(1){
        LED_ON;
        chThdSleepMilliseconds(long_time);
        LED_OFF;
        chThdSleepMilliseconds(long_time);
        LED_ON;
        chThdSleepMilliseconds(short_time);
        LED_OFF;
        chThdSleepMilliseconds(short_time);
        LED_ON;
        chThdSleepMilliseconds(short_time);
        LED_OFF;
        chThdSleepMilliseconds(long_time);
    }
}

/*------------------------------------------------------------------------------
                 Thread 6, Checking home to reset counts
------------------------------------------------------------------------------*/
// 64 byte stack beyond task switch and interrupt needs
static THD_WORKING_AREA(waThread6, 64);

static THD_FUNCTION(Thread6, arg) {

    
    while(1){
        if(AngleAtHome){
            counts = 0;
        }
        chThdSleepMilliseconds(300);
    }
}


//------------------------------------------------------------------------------
void setup() {

  chBegin(chSetup);
  // chBegin never returns, main thread continues with mainThread()
  while(1) {
  }
}
//------------------------------------------------------------------------------
// main thread runs at NORMALPRIO
void chSetup() {

    Serial.begin(9600);
	/*****************  define INPUTS ************************/
	pinMode(EncoderA, INPUT_PULLUP);
	pinMode(EncoderB, INPUT_PULLUP);
	pinMode(limit_sw1, INPUT_PULLUP);
	pinMode(limit_sw2, INPUT_PULLUP);
	pinMode(limit_sw3, INPUT_PULLUP);
	pinMode(manualForward, INPUT_PULLUP);
	pinMode(manualBackward, INPUT_PULLUP);
	pinMode(highSpeed, INPUT_PULLUP);

	/*****************  define OUTPUTS ************************/
	pinMode(Froward_High, OUTPUT);
	pinMode(Forward_Low, OUTPUT);
	pinMode(Backward_High, OUTPUT);
	pinMode(Backward_Low, OUTPUT);
	pinMode(buzzer, OUTPUT);
	pinMode(pwm_out, OUTPUT);
	pinMode(led, OUTPUT);

	attachInterrupt(digitalPinToInterrupt(EncoderA), EncoderISR, RISING);        // Interrupt for Encoder A

    chThdCreateStatic(waThread1, sizeof(waThread1),
        NORMALPRIO + 1, Thread1, NULL);

    chThdCreateStatic(waThread2, sizeof(waThread2),
        NORMALPRIO + 2, Thread2, NULL);
    
    chThdCreateStatic(waThread3, sizeof(waThread3),
        NORMALPRIO + 1, Thread3, NULL);

    chThdCreateStatic(waThread4, sizeof(waThread4),
        NORMALPRIO + 3, Thread4, NULL);

    chThdCreateStatic(waThread5, sizeof(waThread5),
        NORMALPRIO + 4, Thread5, NULL);

    chThdCreateStatic(waThread6, sizeof(waThread6),
        NORMALPRIO + 4, Thread6, NULL);

}
//------------------------------------------------------------------------------
void loop() {
  // not used
}

void EncoderISR(){
    //unsigned long StartTime = micros();
    delayMicroseconds(interrupt_debounce_delay);
    switch( enA){
        case 1:
            switch(enB){
                case 1:
                    counts--;
                    break;
                case 0:
                    counts++;
                    break;
            }
    }
}

void findHome(){
    while(!AngleAtHome){
        BL;
    }
    motor_stop;
    beep();
    counts = 0;
    manual_mode_flag = true;
}

void beep(){
    Buzzer_ON;
	chThdSleepMilliseconds(300);
	Buzzer_OFF;;
	
	chThdSleepMilliseconds(100);
	
	Buzzer_ON;
	chThdSleepMilliseconds(200);
	Buzzer_OFF;
	
	chThdSleepMilliseconds(100);
	
	Buzzer_ON;
	chThdSleepMilliseconds(200);
	Buzzer_OFF;
	
	chThdSleepMilliseconds(100);
	
	Buzzer_ON;
	chThdSleepMilliseconds(300);
	Buzzer_OFF;
}

void SerialCommandAnalysis(char *comm, int len){
    
    if(comm[0] == 'H'){ // Target counts on auto mode
        char temp[len-1];
        for(int i=1; i<len; i++){
            temp[i-1] = comm[i];
        }
        target_counts = atol(temp);
        target_counts += countCorrection;
		Serial.println(target_counts);
		auto_mode_flag = true;
    }

    else if(comm[0] == 'K'){ // getting current counts
        Serial.print("$");
        Serial.print(counts);
        Serial.print("*");
    }

    else if(comm[0] == 'N'){   // stop auto positioning
        auto_mode_flag = false;
    }
    else if(comm[0] == 'S'){  //go to home position
        findHome();
        Serial.println("Zero");
        counts = 0;
    }

    else if(comm[0] == 'L'){ // settings up the low speed counts value
        char temp[len-1];
        for(int i=1; i<len; i++){
            temp[i-1] = comm[i];
        }
        low_speed_counts = atol(temp);
		Serial.println(low_speed_counts);
    }

    else if(comm[0] == 'M'){  // setting correction value of counts
        char temp[len-1];
        for(int i=1; i<len; i++){
            temp[i-1] = comm[i];
        }
        countCorrection = atol(temp);
		Serial.println(countCorrection);
    }
}

void moveAngle(int direction, long position){
    switch(direction){
        case 1:   // Forward high
            while((counts < position) && auto_mode_flag && !AngleAtSW2 && !AngleAtEnd){
                FH;
            }
            break;

        case 2:     // forward low
            while((counts < position) && auto_mode_flag && !AngleAtEnd){
                FL;
            }
            break;

        case 3:     // backward high
            while((counts > position) && auto_mode_flag && !AngleAtHome){
                BH;
            }
            break;

        case 4:     // backward low
            while((counts > position) && auto_mode_flag && !AngleAtHome){
                BL;
            }
            break;

        case 5:
            while((counts > low_speed_counts) && !AngleAtHome && auto_mode_flag){
                BH;
            }
            while(!AngleAtHome && auto_mode_flag){   
                BL;
            }
            break;
    }


    motor_stop;
    
}


