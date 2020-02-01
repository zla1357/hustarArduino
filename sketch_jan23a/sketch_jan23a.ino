#include <MsTimer2.h>
#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>
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

#define ADDR_CYL_MAIN 0 //메인실린더 값을 저장하는 주소

int touch_flag = 0;     //터치버튼1의 엣지체크를 위한 플래그
int touch_flag2 = 0;    //터치버튼2의 엣지체크를 위한 플래그
int touch_flag3 = 0;    //터치버튼3의 엣지체크를 위한 플래그
int tim1_run_flag = 0;  //타이머가 실행되고 있는지 여부를 알리는 flag
int tim_cnt = 0;        //타이머가 실행되는 시간을 누적하는 변수
int tim2_cnt = 0;       //터치2가 눌리는 동안의 시간이 누적되는 변수
int tim3_cnt = 0;       //터치3가 눌리는 동안의 시간이 누적되는 변수
int btn_tim = 0;        //현재 타이머를 실행시킨 버튼(현재 누른 버튼)

SoftwareSerial mySerial(12, 13);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);
uint16_t id;

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

    Serial.println("핀 초기화중...");
    pinMode(touchBTN0pin, INPUT);
    pinMode(touchBTN1pin, INPUT);
    pinMode(touchBTN2pin, INPUT);
    pinMode(tCylinder.pinR, OUTPUT);
    pinMode(tCylinder.pinL, OUTPUT);

    MsTimer2::set(100, count);

    Serial.println("지문인식 센서 초기화중...");
    finger.begin(57600);
    if (finger.verifyPassword()) {
        Serial.println("지문인식 센서 설정 완료");
    } else {
        Serial.println("지문인식 센서를 찾지 못했습니다.");
        while (1) { delay(1); }
    }

    finger.getTemplateCount();
    Serial.print("센서에 저장된 지문은 "); Serial.print(finger.templateCount); Serial.println("개 입니다.");
}

uint8_t readnumber(void) {
    uint8_t num = 0;
    
    while (num == 0) {
        while (! Serial.available());
        num = Serial.parseInt();
    }
    return num;
}

//실리더를 움직인 시간 카운터

void count() {

    if (btn_tim == touchBTN0pin) {

        tim_cnt++;
    }
    else if (btn_tim == touchBTN1pin) {

        //tim2_cnt++;
    }
    else if (btn_tim == touchBTN2pin) {
        //tim3_cnt++;
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

void saveFingerPrint()                     // 지문을 읽어서 저장하는 함수
{    
    finger.getTemplateCount();
    Serial.println(finger.templateCount);
    Serial.println("지문 저장 준비");
    id = finger.templateCount + 1;
    if (id == 0) {// ID #0 not allowed, try again!
        return;
    }
    Serial.print("ID ");
    Serial.print(id);
    Serial.print("(으)로 지문을 저장합니다.");
    
    while (!  getFingerprintEnroll() );
}

uint8_t getFingerprintEnroll() {

    int p = -1;
    Serial.print("Waiting for valid finger to enroll as #"); Serial.println(id);
    while (p != FINGERPRINT_OK) {
      p = finger.getImage();
      switch (p) {
          case FINGERPRINT_OK:
              Serial.println("Image taken");
              break;
          case FINGERPRINT_NOFINGER:
              Serial.print(".");
              break;
          case FINGERPRINT_PACKETRECIEVEERR:
              Serial.println("통신 에러");
              break;
          case FINGERPRINT_IMAGEFAIL:
              Serial.println("이미지 변환 에러");
              break;
          default:
              Serial.println("알 수 없는 에러");
              break;
        }
    }
  
    // OK success!
  
    p = finger.image2Tz(1);
    switch (p) {
        case FINGERPRINT_OK:
            Serial.println("이미지 변환 완료");
            break;
        case FINGERPRINT_IMAGEMESS:
            Serial.println("이미지가 너무 큽니다.");
            return p;
        case FINGERPRINT_PACKETRECIEVEERR:
            Serial.println("통신 에러");
            return p;
        case FINGERPRINT_FEATUREFAIL:
            Serial.println("Could not find fingerprint features");
            return p;
        case FINGERPRINT_INVALIDIMAGE:
            Serial.println("Could not find fingerprint features");
            return p;
        default:
            Serial.println("알 수 없는 에러");
            return p;
    }

    p = finger.fingerFastSearch();
    if (p == FINGERPRINT_OK){
        Serial.println("이미 존재하는 지문입니다.");
        return 1;  
    }
    
    Serial.println("손가락을 떼세요");
    delay(2000);
    p = 0;
    while (p != FINGERPRINT_NOFINGER) {
        p = finger.getImage();
    }
    Serial.print("ID "); Serial.println(id);
    p = -1;
    Serial.println("같은 손가락을 다시 올려주세요");
    while (p != FINGERPRINT_OK) {
        p = finger.getImage();
        switch (p) {
            case FINGERPRINT_OK:
                Serial.println("Image taken");
                break;
            case FINGERPRINT_NOFINGER:
                Serial.print(".");
                break;
            case FINGERPRINT_PACKETRECIEVEERR:
                Serial.println("통신 에러");
                break;
            case FINGERPRINT_IMAGEFAIL:
                Serial.println("이미지 변환 에러");
                break;
            default:
                Serial.println("알 수 없는 에러");
                break;
        }
    }
  
    // OK success!
  
    p = finger.image2Tz(2);
    switch (p) {
        case FINGERPRINT_OK:
            Serial.println("이미지 변환 완료");
            break;
        case FINGERPRINT_IMAGEMESS:
            Serial.println("이미지가 너무 큽니다.");
            return p;
        case FINGERPRINT_PACKETRECIEVEERR:
            Serial.println("Communication error");
            return p;
        case FINGERPRINT_FEATUREFAIL:
            Serial.println("Could not find fingerprint features");
            return p;
        case FINGERPRINT_INVALIDIMAGE:
            Serial.println("Could not find fingerprint features");
            return p;
        default:
            Serial.println("알 수 없는 에러");
            return p;
    }
    
    // OK converted!
    Serial.print("다음 ID로 저장합니다. ");  Serial.println(id);
    
    p = finger.createModel();
    if (p == FINGERPRINT_OK) {
        Serial.println("데이터 일치!");
    } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
        Serial.println("통신 에러");
        return p;
    } else if (p == FINGERPRINT_ENROLLMISMATCH) {
        Serial.println("지문이 일치하지 않습니다.");
        return p;
    } else {
        Serial.println("알 수 없는 에러");
        return p;
    }   
    
    Serial.print("ID "); Serial.println(id);
    p = finger.storeModel(id);
    if (p == FINGERPRINT_OK) {
        Serial.println("저장완료!");
        return 1;
    } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
        Serial.println("통신 에러");
        return p;
    } else if (p == FINGERPRINT_BADLOCATION) {
        Serial.println("해당 ID로 저장할 수 없습니다.");
        return p;
    } else if (p == FINGERPRINT_FLASHERR) {
        Serial.println("플래시 메모리에 저장중에 오류가 발생하였습니다.");
        return p;
    } else {
        Serial.println("알 수 없는 에러");
        return p;
    }   
}

//지문을 인식하여 id를 출력
int getFingerprintIDez() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK)  return -1;
  
  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID); 
  Serial.print(" 신뢰도 "); Serial.println(finger.confidence);
  return finger.fingerID; 
}

void loop() {

//    터치값 읽음

    int touchValue1 = digitalRead(touchBTN0pin);
    int touchValue2 = digitalRead(touchBTN1pin);
    int touchValue3 = digitalRead(touchBTN2pin);

//    Serial.print(btn_tim);

    if ((btn_tim == 0) || (btn_tim == touchBTN0pin)) {

        // 터치버튼1 터치됨, 숏터치 체크
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
            if((btn_tim == touchBTN0pin) && (tim1_run_flag == 1)){
                touch_flag = 0; 
                stopTimer(touchBTN0pin);
                btn_tim = 0;
                tim1_run_flag = 0;

                if(tim_cnt > 20){
                    saveFingerPrint();
                }
                else{
                    Serial.println("손가락을 올려주세요");
                    int fingerId = -1;
                    while(fingerId == -1){
                        fingerId = getFingerprintIDez();
                        delay(50);
                        if(fingerId == -1){
                            Serial.println("지문을 찾을 수 없습니다.");
                        }
                    }  
                }
                tim_cnt = 0;
            }
        }
    }

//    if (btn_tim == 0 || btn_tim == touchBTN0pin) {
//
//        // 터치버튼1 터치됨, 숏터치 체크
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
//                    stopTimer(touchBTN0pin);
//                }
//            }
//        }
//        else if(btn_tim = btn_tim) {
//            touch_flag = 0;
//            btn_tim = 0; 
//        }
//    }

// 1번버튼을 롱터치로 바꿀 대를 대비해 남겨둔 구문
//    if (btn_tim == 0 || btn_tim == touchBTN0pin) {
//
//        //터치버튼1 터치됨, 롱터치 체크
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
//            }
//        }
//        else {
//
//          if (tim1_run_flag == 1) {
//
//              touch_flag = 0;
//              tim1_run_flag = 0;
//              stopTimer(touchBTN0pin);
//              btn_tim = 0;
//          }
//        }
//    }
// 1번버튼을 롱터치로 바꿀 대를 대비해 남겨둔 구문

    if ((btn_tim == 0) || (btn_tim == touchBTN1pin)) {

        //터치버튼2 터치됨, 롱터치 체크
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

          if ((tim1_run_flag == 1) && (btn_tim == touchBTN1pin)) {

              touch_flag2 = 0;
              tim1_run_flag = 0;
              stopTimer(touchBTN1pin);
              fCylinderSTOP(tCylinder);
              btn_tim = 0;
          }
        }
    }
    
    if ((btn_tim == 0) || (btn_tim == touchBTN2pin)) {

        //터치버튼3 터치됨, 롱터치 체크
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

          if ((tim1_run_flag == 1) && (btn_tim == touchBTN2pin)) {

              touch_flag3 = 0;
              tim1_run_flag = 0;
              stopTimer(touchBTN2pin);
              fCylinderSTOP(tCylinder);
              btn_tim = 0;
          }
        }
    }
}
