#include <MsTimer2.h>

#include <EEPROM.h>
#define touchBTN0pin 2 //interruptPin  터치센서1
#define touchBTN1pin 3 // 터치센서2(높이실린더 DOWN)
#define touchBTN2pin 4 // 터치센서3(높이실린더 UP)
#define touchBTN3pin 5
#define touchBTN4pin 5
#define angleCylinderR 7
#define angleCylinderL 6 
#define moveCylinderR 9
#define moveCylinderL 10
#define deskCylinderR 11
#define deskCylinderL 12

#define LEDPIN 13      // LED 핀 설정
#define ADDR_CYL_MAIN 0 //메인실린더 값을 저장하는 주소

int touch_flag = 0;     //터치버튼1의 엣지체크를 위한 플래그
int touch_flag2 = 0;    //터치버튼2의 엣지체크를 위한 플래그
int touch_flag3 = 0;    //터치버튼3의 엣지체크를 위한 플래그
int tim1_run_flag = 0;  //타이머가 실행되고 있는지 여부를 알리는 flag
int tim_cnt = 0;        //타이머가 실행되는 시간을 누적하는 변수
int tim2_cnt = 0;       //터치2가 눌리는 동안의 시간이 누적되는 변수
int tim3_cnt = 0;       //터치3가 눌리는 동안의 시간이 누적되는 변수
int btn_tim = 0;        //현재 타이머를 실행시킨 버튼(현재 누른 버튼)

struct Cylinder {

  char pinR;
  char pinL;
};

struct Cylinder tCylinder = { angleCylinderR,angleCylinderL};

void fCylinderSTOP(struct Cylinder mCylinder) {

  Serial.println("stop");
  digitalWrite(mCylinder.pinR, LOW);
  digitalWrite(mCylinder.pinL, LOW);
  Serial.print("cnt1 : ");
  Serial.print(tim_cnt);
  Serial.print("     cnt2 : ");
  Serial.print(tim2_cnt);
  Serial.print("     cnt3 : ");
  Serial.println(tim3_cnt);
}

 

void fCylinderUP (struct Cylinder mCylinder) {

  Serial.println("up");
  digitalWrite(mCylinder.pinR, HIGH);
  digitalWrite(mCylinder.pinL, LOW);
}

 

void fCylinderDOWN (struct Cylinder mCylinder) {

  Serial.println("down");
  digitalWrite(mCylinder.pinR, LOW);
  digitalWrite(mCylinder.pinL, HIGH);
}

void setup() {

    Serial.begin(9600);

    pinMode(LEDPIN, OUTPUT);
    pinMode(touchBTN0pin, INPUT);
    pinMode(touchBTN1pin, INPUT);
    pinMode(touchBTN2pin, INPUT);
    pinMode(tCylinder.pinR, OUTPUT);
    pinMode(tCylinder.pinL, OUTPUT);

    if (EEPROM.read(ADDR_CYL_MAIN) < 1) {

        EEPROM.write(ADDR_CYL_MAIN, 1);
    }

    MsTimer2::set(100, count);
}

 

//실리더를 움직인 시간 카운터

void count() {

    if (btn_tim == touchBTN0pin) {

        tim_cnt++;
    }
    else if (btn_tim == touchBTN1pin) {

        tim2_cnt++;
    }
    else if (btn_tim == touchBTN2pin) {
        tim3_cnt++;
    }
}

 

void startTimer(int btn) {

    btn_tim = btn;
    MsTimer2::start();
}

void stopTimer(int btn){
    btn_tim = btn;
    MsTimer2::stop();
}

void loop() {

//    터치값 읽음

    int touchValue1 = digitalRead(touchBTN0pin);
    int touchValue2 = digitalRead(touchBTN1pin);
    int touchValue3 = digitalRead(touchBTN2pin);

//    if (btn_tim == 0 || btn_tim == touchBTN0pin) {
//
//        // 터치버튼1 터치됨
//        if (touchValue1 == HIGH) {
//
//            // 터치가 되었을 때 엣지체크
//            if (touch_flag == 0) {
//              
//                touch_flag = 1;
//
//                //타이머가 실행되고 있는지 체크
//                if (tim1_run_flag == 0) {
//
//                    tim1_run_flag = 1;
//                    startTimer(touchBTN0pin);
//                }
//                else {
//
//                    tim1_run_flag = 0;
//                    MsTimer2::stop();
//                    btn_tim = 0;
//                }
//            }
//        }
//        else {
//
//            touch_flag = 0;
//        }
//    }

    if (btn_tim == 0 || btn_tim == touchBTN0pin) {

        //터치버튼2 터치됨
        if (touchValue1 == HIGH) {

            // 터치가 되었을 때 엣지체크
            if (touch_flag == 0) {

                touch_flag = 1;
                
                //타이머가 실행되고 있는지 체크
                if (tim1_run_flag == 0) {

                    tim1_run_flag = 1;
                    startTimer(touchBTN0pin);
                }
            }
        }
        else {

          if (tim1_run_flag == 1) {

              touch_flag = 0;
              tim1_run_flag = 0;
              stopTimer(touchBTN0pin);
//              MsTimer2::stop();
              btn_tim = 0;
          }
        }
    } 

    if (btn_tim == 0 || btn_tim == touchBTN1pin) {

        //터치버튼2 터치됨
        if (touchValue2 == HIGH) {

            // 터치가 되었을 때 엣지체크
            if (touch_flag2 == 0) {

                touch_flag2 = 1;
                
                //타이머가 실행되고 있는지 체크
                if (tim1_run_flag == 0) {

                    tim1_run_flag = 1;
                    startTimer(touchBTN1pin);
                    fCylinderDOWN(tCylinder);
                }
            }
        }
        else {

          if (tim1_run_flag == 1) {

              touch_flag2 = 0;
              tim1_run_flag = 0;
              stopTimer(touchBTN1pin);
//              MsTimer2::stop();
              fCylinderSTOP(tCylinder);
              btn_tim = 0;
          }
        }
    }
    
    if (btn_tim == 0 || btn_tim == touchBTN2pin) {

        //터치버튼2 터치됨
        if (touchValue3 == HIGH) {

            // 터치가 되었을 때 엣지체크
            if (touch_flag3 == 0) {

                touch_flag3 = 1;

                //타이머가 실행되고 있는지 체크
                if (tim1_run_flag == 0) {

                    tim1_run_flag = 1;
                    startTimer(touchBTN2pin);
                    fCylinderUP(tCylinder);
                }
            }
        }
        else {

          if (tim1_run_flag == 1) {

              touch_flag3 = 0;
              tim1_run_flag = 0;
              stopTimer(touchBTN2pin);
//              MsTimer2::stop();
              fCylinderSTOP(tCylinder);
              btn_tim = 0;
          }
        }
    }
}
