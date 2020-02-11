#include <EEPROM.h>
#include <MsTimer2.h>
#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>
#include "U8glib.h"

U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE | U8G_I2C_OPT_DEV_0); // I2C / TWI

#define touchBTN0pin A0 //interruptPin
#define touchBTN1pin A1
#define touchBTN2pin A2
#define touchBTN3pin A3
#define touchBTN4pin A4


#define angleCylinderR 4
#define angleCylinderL 5

#define moveCylinderR 6
#define moveCylinderL 7

#define deskCylinderR 8 //독서대 원래는 6
#define deskCylinderL 9 //독서대 원래는 7

#define TX 12
#define RX 13

#define PHOTOSENSOR1 2
#define PHOTOSENSOR2 3
#define PHOTOSENSOR3 10


//메모리에 지문에 대한 각도값을 저장하기 위해 정의하는 이름
//한 id에 대한 메모리 크기는 5바이트로 설정
//저장되는 메모리 위치는 id * 5 + addr
//ex) id가 5인 지문의 id가 저장되는 주소 : 5 * 5 + 0 = 25
//ex2) id가 5인 지문의 모니터 높이가 저장되는 주소 : 5 * 5 + 1 = 26
#define ADDR_FING_KEY 0       //지문 ID
#define ADDR_MONI_HEIGHT 1    //모니터 높이
#define ADDR_MONI_ANGLE 2     //모니터 각도
#define ADDR_BOOK_HEIGHT 3    //독서대 높이

bool mode_flag = false;

bool desk_flag = false;
bool angle_flag = false;
bool move_flag = false;

int pre_photo_move = 0;
int photo_cnt_move = 0;
//int curr_photo_angle = 0;

int pre_photo_angle = 0;
int photo_cnt_angle = 0;
//int curr_photo_move = 0;

int pre_photo_desk = 0;
int photo_cnt_desk = 0;
//int curr_photo_desk = 0;



uint8_t tim1_run_flag = 0;  //타이머가 실행되고 있는지 여부를 알리는 flag
uint8_t tim_cnt = 0;        //타이머가 실행되는 시간을 누적하는 변수
uint8_t tim2_cnt = 0;       //터치2가 눌리는 동안의 시간이 누적되는 변수
uint8_t tim3_cnt = 0;       //터치3가 눌리는 동안의 시간이 누적되는 변수
uint8_t tim4_cnt = 0;       //터치3가 눌리는 동안의 시간이 누적되는 변수
uint8_t finger_save_cnt = 0;
uint8_t btn_tim = 0;        //현재 타이머를 실행시킨 버튼(현재 누른 버튼)
volatile int mode = 0;

SoftwareSerial mySerial(RX, TX);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);
uint16_t id;


struct Cylinder {
  uint8_t pinR;
  uint8_t pinL;
};
struct Cylinder tCylinder = { angleCylinderR, angleCylinderL};

struct Cylinder deskCylinder = { deskCylinderR, deskCylinderL};
struct Cylinder moniterMoveCylinder = { moveCylinderR, moveCylinderL};
struct Cylinder moniterAngleCylinder = { angleCylinderR, angleCylinderL};

void fCylinderSTOP(struct Cylinder mCylinder) {
  //  Serial.println("Cylinder stop");

  digitalWrite(mCylinder.pinR, LOW);
  digitalWrite(mCylinder.pinL, LOW);
}

void fCylinderUP (struct Cylinder mCylinder) {
  //  Serial.print("Cylinder up  ");

  //  Serial.print("pin : ");
  //  Serial.print(mCylinder.pinR);
  //  Serial.print(" pin : ");
  //  Serial.println(mCylinder.pinL);

  digitalWrite(mCylinder.pinR, HIGH);
  digitalWrite(mCylinder.pinL, LOW);
}

void fCylinderDOWN (struct Cylinder mCylinder) {
  //  Serial.print("Cylinder down  ");

  //  Serial.print("pin : ");
  //  Serial.print(mCylinder.pinR);
  //  Serial.print(" pin : ");
  //  Serial.println(mCylinder.pinL);

  digitalWrite(mCylinder.pinR, LOW);
  digitalWrite(mCylinder.pinL, HIGH);
}

//홀센서 사용 예정+타이머
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
  else if (btn_tim == touchBTN3pin) {
    tim4_cnt++;
  }
  else if (btn_tim == touchBTN4pin) {
    finger_save_cnt++;
  }
}


void startTimer(int btn) {
  btn_tim = btn;
  MsTimer2::start();
}

void stopTimer(int btn) {
  btn_tim = btn;
  MsTimer2::stop();

  if (btn == touchBTN0pin) {
    tim3_cnt = 0;
  }
  if (btn == touchBTN3pin) {
    tim4_cnt = 0;
  }
  if (btn == touchBTN4pin) {
    finger_save_cnt = 0;
  }
  btn_tim = 0;
}

void saveFingerPrint(int btn)                     // 지문을 읽어서 저장하는 함수
{
  finger.getTemplateCount();

  id = finger.templateCount + 1;
  if (id == 0) {
    return;
  }

  //  Serial.print("ID ");
  //  Serial.print(id);
  //  Serial.print("(으)로 지문을 저장합니다.");
  startTimer(btn);
  while (!getFingerprintEnroll())
  {
  }

  stopTimer(btn);
  return;
}

uint8_t getFingerprintEnroll() {

  int p = -1;
  //  Serial.print("Waiting for valid finger to enroll as #"); Serial.println(id);
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
      case FINGERPRINT_OK:
        //        Serial.println("Image taken");
        break;
      case FINGERPRINT_NOFINGER:
        //        Serial.print(".");
        u8g.firstPage();
        do {
          u8g.drawStr(0, 22, "Waiting for");
          u8g.drawStr(0, 44, "valid finger.");
        } while (u8g.nextPage());
        if (finger_save_cnt > 30 ) {
          //          Serial.println("시간초과");
          u8g.firstPage();
          do {
            u8g.drawStr(0, 22, "Time out");
          } while (u8g.nextPage());
          return 1;
        }
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        //        Serial.println("통신 에러");
        u8g.firstPage();
        do {
          u8g.drawStr(0, 22, "communication");
          u8g.drawStr(0, 44, "error");
        } while (u8g.nextPage());
        break;
      case FINGERPRINT_IMAGEFAIL:
        //        Serial.println("이미지 변환 에러");
        u8g.firstPage();
        do {
          u8g.drawStr(0, 22, "image convert");
          u8g.drawStr(0, 44, "error");
        } while (u8g.nextPage());
        break;
      default:
        //        Serial.println("알 수 없는 에러");
        u8g.firstPage();
        do {
          u8g.drawStr(0, 22, "unknown error");
        } while (u8g.nextPage());
        break;
    }
  }

  // OK success!

  p = finger.image2Tz(1);
  switch (p) {
    case FINGERPRINT_OK:
      //      Serial.println("이미지 변환 완료");
      u8g.firstPage();
      do {
        u8g.drawStr(0, 22, "image convert");
        u8g.drawStr(0, 44, "complete");
      } while (u8g.nextPage());
      break;
    case FINGERPRINT_IMAGEMESS:
      //      Serial.println("이미지가 너무 큽니다.");
      u8g.firstPage();
      do {
        u8g.drawStr(0, 22, "to big image");
      } while (u8g.nextPage());
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      //      Serial.println("통신 에러");
      u8g.firstPage();
      do {
        u8g.drawStr(0, 22, "communication");
        u8g.drawStr(0, 44, "error");
      } while (u8g.nextPage());
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      //      Serial.println("알 수 없는 에러");
      u8g.firstPage();
      do {
        u8g.drawStr(0, 22, "unknown error");
      } while (u8g.nextPage());
      return p;
  }

  p = finger.fingerFastSearch();
  if (p == FINGERPRINT_OK) {
    //    Serial.println("이미 존재하는 지문입니다.");
    u8g.firstPage();
    do {
      u8g.drawStr(0, 22, "fingerprint");
      u8g.drawStr(0, 44, "already exist");
    } while (u8g.nextPage());
    return 1;
  }

  //  Serial.println("손가락을 떼세요");
  u8g.firstPage();
  do {
    u8g.drawStr(0, 22, "put off finger");
  } while (u8g.nextPage());
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }
  //  Serial.print("ID "); Serial.println(id);

  char fingerID[10];
  sprintf(fingerID, "%d", id);
  u8g.firstPage();
  do {
    u8g.drawStr(0, 22, "ID : ");
    u8g.drawStr(50, 22, fingerID);
  } while (u8g.nextPage());
  p = -1;

  //  Serial.println("같은 손가락을 다시 올려주세요");
  u8g.firstPage();
  do {
    u8g.drawStr(0, 22, "put on same");
    u8g.drawStr(0, 44, "finger");
  } while (u8g.nextPage());

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
        //        Serial.println("통신 에러");
        u8g.firstPage();
        do {
          u8g.drawStr(0, 22, "communication");
          u8g.drawStr(0, 44, "error");
        } while (u8g.nextPage());
        break;
      case FINGERPRINT_IMAGEFAIL:
        //        Serial.println("이미지 변환 에러");
        u8g.firstPage();
        do {
          u8g.drawStr(0, 22, "image convert");
          u8g.drawStr(0, 44, "error");
        } while (u8g.nextPage());
        break;
      default:
        //        Serial.println("알 수 없는 에러");
        u8g.firstPage();
        do {
          u8g.drawStr(0, 22, "unknown error");
        } while (u8g.nextPage());
        break;
    }
  }

  // OK success!

  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      //      Serial.println("이미지 변환 완료");
      u8g.firstPage();
      do {
        u8g.drawStr(0, 22, "image convert");
        u8g.drawStr(0, 44, "complete");
      } while (u8g.nextPage());
      break;
    case FINGERPRINT_IMAGEMESS:
      //      Serial.println("이미지가 너무 큽니다.");
      u8g.firstPage();
      do {
        u8g.drawStr(0, 22, "image to big");
      } while (u8g.nextPage());
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      //      Serial.println("Communication error");
      u8g.firstPage();
      do {
        u8g.drawStr(0, 22, "communication");
        u8g.drawStr(0, 44, "error");
      } while (u8g.nextPage());
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      //      Serial.println("알 수 없는 에러");
      u8g.firstPage();
      do {
        u8g.drawStr(0, 22, "unknown error");
      } while (u8g.nextPage());
      return p;
  }

  // OK converted!

  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    Serial.println("데이터 일치!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    //    Serial.println("통신 에러");
    u8g.firstPage();
    do {
      u8g.drawStr(0, 22, "communication");
      u8g.drawStr(0, 44, "error");
    } while (u8g.nextPage());
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    //    Serial.println("지문이 일치하지 않습니다.");
    u8g.firstPage();
    do {
      u8g.drawStr(0, 22, "fingerprint");
      u8g.drawStr(0, 44, "not same");
    } while (u8g.nextPage());
    return p;
  } else {
    //    Serial.println("알 수 없는 에러");
    u8g.firstPage();
    do {
      u8g.drawStr(0, 22, "unknown error");
    } while (u8g.nextPage());
    return p;
  }

  //  Serial.print("다음 ID로 저장합니다. ");  Serial.println(id);
  //  Serial.print("ID "); Serial.println(id);
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {

    //    Serial.println("저장완료!");
    u8g.firstPage();
    do {
      u8g.drawStr(0, 22, "save complete");
      u8g.drawStr(0, 44, "with ID : ");
      u8g.drawStr(100, 44, fingerID);
    } while (u8g.nextPage());

    //eeprom에 지문에 대한 각도를 저장하는 부분
    EEPROM.write(id * 5 + ADDR_FING_KEY, id);
    EEPROM.write(id * 5 + ADDR_MONI_HEIGHT, photo_cnt_move);
    EEPROM.write(id * 5 + ADDR_MONI_ANGLE, photo_cnt_angle);//모니터 각도
    EEPROM.write(id * 5 + ADDR_BOOK_HEIGHT, photo_cnt_desk);// 독서대 로 변경할것

    return 1;

  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {

    //    Serial.println("통신 에러");
    u8g.firstPage();
    do {
      u8g.drawStr(0, 22, "communication");
      u8g.drawStr(0, 44, "error");
    } while (u8g.nextPage());

    return p;

  } else if (p == FINGERPRINT_BADLOCATION) {

    //    Serial.println("해당 ID로 저장할 수 없습니다.");
    u8g.firstPage();
    do {
      u8g.drawStr(0, 22, "cannot save id");
      u8g.drawStr(0, 44, fingerID);
    } while (u8g.nextPage());

    return p;

  } else if (p == FINGERPRINT_FLASHERR) {

    //    Serial.println("플래시 메모리에 저장중에 오류가 발생하였습니다.");
    u8g.firstPage();
    do {
      u8g.drawStr(0, 22, "error while");
      u8g.drawStr(0, 44, "saving storage");
    } while (u8g.nextPage());

    return p;

  } else {

    //    Serial.println("알 수 없는 에러");
    u8g.firstPage();
    do {
      u8g.drawStr(0, 22, "unknown error");
    } while (u8g.nextPage());

    return p;

  }
}

//지문을 인식하여 id를 출력
int getFingerprintIDez() {
  uint8_t p = finger.getImage();
  int moni_height = 0;
  int moni_angle = 0;
  int book_height = 0;

  if (p != FINGERPRINT_OK)  return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK)  return -1;

  // found a match!
  //  Serial.print("Found ID #"); Serial.print(finger.fingerID);
  //  Serial.print(" 신뢰도 "); Serial.println(finger.confidence);

  char fingerID[10];
  sprintf(fingerID, "%d", finger.fingerID);
  u8g.firstPage();
  do {
    u8g.drawStr(0, 22, "Found ID : ");
    u8g.drawStr(110, 22, fingerID);
  } while (u8g.nextPage());
  delay(200);

  moni_height = EEPROM.read(finger.fingerID * 5 + ADDR_MONI_HEIGHT);
  moni_angle = EEPROM.read(finger.fingerID * 5 + ADDR_MONI_ANGLE);
  book_height = EEPROM.read(finger.fingerID * 5 + ADDR_BOOK_HEIGHT);

  //  Serial.print("모니터 높이 : ");
  //  Serial.println(moni_height);
  //  Serial.print("모니터 각도 : ");
  //  Serial.println(moni_angle);
  //  Serial.print("책상높이 : ");
  //  Serial.println(book_height);

  char str_height[10];
  char str_desk[10];
  char str_angle[10];
  sprintf(str_height, "%d", moni_height);
  sprintf(str_desk, "%d", book_height);
  sprintf(str_angle, "%d", moni_angle);
  u8g.firstPage();
  do {
    u8g.drawStr(0, 11, "height : ");
    u8g.drawStr(90, 11, str_height);

    u8g.drawStr(0, 33, "desk : ");
    u8g.drawStr(90, 33, str_desk);

    u8g.drawStr(0, 55, "angle : ");
    u8g.drawStr(90, 55, str_angle);
  } while (u8g.nextPage());

  //독서대 실린더를 확인해서 증감시키는 부분
  if (photo_cnt_desk < book_height) {

    //독서대 실린더 늘임
    fCylinderUP(deskCylinder);
  }
  else if (photo_cnt_desk > book_height) {

    //독서대 실린더 줄임
    fCylinderDOWN(deskCylinder);
  }

  //모니터 높이 실린더를 확인해서 증감시키는 부분
  if (photo_cnt_move < moni_height) {

    //모니터 높이 실린더 늘임
    fCylinderUP(moniterMoveCylinder);
  }
  else if (photo_cnt_move > moni_height) {

    //모니터 높이 실린더 줄임
    fCylinderDOWN(moniterMoveCylinder);
  }

  //현재 포토센서 값과 지문인식으로 불러온 값이 같아질 때 까지 이동
  while ((photo_cnt_desk != book_height) or (photo_cnt_move != moni_height)) {
    int curr_photo_desk = digitalRead(PHOTOSENSOR1);
    int curr_photo_move = digitalRead(PHOTOSENSOR3);

    if (photo_cnt_desk != book_height) {
      fPhoto_test(pre_photo_desk , curr_photo_desk, &photo_cnt_desk, (photo_cnt_desk < book_height ? 1 : -1));
    }
    if (photo_cnt_move != moni_height) {
      fPhoto_test(pre_photo_move , curr_photo_move, &photo_cnt_move, (photo_cnt_move < moni_height ? 1 : -1));
    }

    pre_photo_desk = curr_photo_desk;
    pre_photo_move = curr_photo_move;
  }

  //독서대 멈춤
  fCylinderSTOP(deskCylinder);
  //독서대 멈춤

  //모니터 높이 멈춤
  fCylinderSTOP(moniterMoveCylinder);
  //모니터 높이 멈춤

  //모니터 높이조절이 멈춘 후에 모니터 각도를 확인하여 움직임
  //모니터 각도 실린더를 확인해서 증감시키는 부분
  if (photo_cnt_angle < moni_angle) {

    //모니터 각도 실린더 늘임
    fCylinderUP(moniterAngleCylinder);
  }
  else if (photo_cnt_angle > moni_angle) {

    //모니터 각도 실린더 줄임
    fCylinderDOWN(moniterAngleCylinder);
  }

  while (photo_cnt_angle != moni_angle) {
    int curr_photo_angle = digitalRead(PHOTOSENSOR2);

    if (photo_cnt_angle != moni_angle) {
      fPhoto_test(pre_photo_angle , curr_photo_angle, &photo_cnt_angle, (photo_cnt_angle < moni_angle ? 1 : -1));
    }

    pre_photo_angle = curr_photo_angle;
  }

  //모니터 각도 멈춤
  fCylinderSTOP(moniterAngleCylinder);
  //모니터 각도 멈춤

  return finger.fingerID;
}

/*
  AnalogReadSerial
  Reads an analog input on pin 0, prints the result to the serial monitor.
  Attach the center pin of a potentiometer to pin A0, and the outside pins to +5V and ground.

  This example code is in the public domain.
*/

void modeSet() {
  mode = (++mode) % 2;
  //  Serial.print("BTN0  "); Serial.print("mode : "); Serial.println(mode);

  u8g.firstPage();
  do {
    u8g.drawStr(0, 22, "mode : ");
    u8g.drawStr(70, 22, mode == 1 ? "1" : "0");
  } while (u8g.nextPage());
}

// the setup routine runs once when you press reset:
void setup() {
  Serial.begin(9600);
  pinMode(deskCylinderR, OUTPUT);
  pinMode(deskCylinderL, OUTPUT);
  pinMode(moveCylinderR, OUTPUT);
  pinMode(moveCylinderL, OUTPUT);
  pinMode(angleCylinderR, OUTPUT);
  pinMode(angleCylinderL, OUTPUT);
  //pinMode(touchBTN0pin, INPUT);
  //attachInterrupt(digitalPinToInterrupt(touchBTN0pin), modeSet, FALLING);

  u8g.setFont(u8g_font_unifont);
  MsTimer2::set(100, count);
  u8g.firstPage();

  do {
    u8g.drawStr(0, 22, "initialize");
    u8g.drawStr(0, 44, "fingerprint");
  } while (u8g.nextPage());

  finger.begin(57600);
  if (finger.verifyPassword()) {

    finger.getTemplateCount();
    char fingCnt[10];
    sprintf(fingCnt, "%d", finger.templateCount);
    u8g.firstPage();

    do {
      u8g.drawStr(0, 22, "initialize fin");
      u8g.drawStr(0, 44, fingCnt);
      u8g.drawStr(10, 44, " saved");
    } while (u8g.nextPage());

  } else {
    u8g.firstPage();
    do {
      u8g.drawStr(0, 22, "sensor");
      u8g.drawStr(0, 44, "not found");
    } while (u8g.nextPage());
  }
  //    //지문을 전부 초기화
  //    finger.emptyDatabase();
}

//포토다이오드 센서 값에 따라 포토다이오드 카운트를 증감시키는 함수
//이전 포토다이오드값, 현재 포토다이오드값, 증감시킬 카운트 변수의 포인터, 증감값
void fPhoto_test(int pre_photo, int curr_photo, int *cnt, int x) {
  if (curr_photo == 0 and pre_photo == 1) {
    if ( x == 1) {
      (*cnt)++;

    } else if ( x == -1) {
      if ((*cnt) < 2) {
        (*cnt) = 1;
      }
      else {
        (*cnt)--;
      }
    }

    Serial.print(*cnt);
    Serial.print("  ");
  }
}


// the loop routine runs over and over again forever:
void loop() {
  // read the input on analog pin 0:

  // print out the value you read:
  //    switch (){
  //      case :
  //
  //      break;

  //default:
  //}

  /*-- -*/
  int curr_photo_desk = digitalRead(PHOTOSENSOR1);
  int curr_photo_angle = digitalRead(PHOTOSENSOR2);
  int curr_photo_move = digitalRead(PHOTOSENSOR3);


  if (btn_tim == 0 || btn_tim == touchBTN0pin) { //모드0번 0버튼 독서대 위로  : 누르는 동안 작동
    if (analogRead(touchBTN0pin) >= 900) {
      if (mode_flag == false) { // 터치가 되었을 때 엣지체크
        mode_flag = true;
        if (tim1_run_flag == 0) { //타이머가 실행되고 있는지 체크
          tim1_run_flag = 1;
          startTimer(touchBTN0pin);
          modeSet();
        }
      }
    }
    else {
      if (tim1_run_flag == 1) {
        mode_flag = false;
        tim1_run_flag = 0;
        stopTimer(touchBTN0pin);
      }
    }
  }


  if ( (btn_tim == 0 || btn_tim == touchBTN1pin) and mode == 0) { //모드0번 1버튼 독서대 위로  : 누르는 동안 작동
    if (analogRead(touchBTN1pin) >= 900) {

      if (desk_flag == false) { // 터치가 되었을 때 엣지체크
        Serial.println("desk up BTN1");//TEST
        desk_flag = true;
        if (tim1_run_flag == 0) { //타이머가 실행되고 있는지 체크
          tim1_run_flag = 1;
          startTimer(touchBTN1pin);
          fCylinderUP(deskCylinder);
        }
      }

      fPhoto_test(pre_photo_desk , curr_photo_desk, &photo_cnt_desk, 1);


    }
    else {
      if (tim1_run_flag == 1) {
        desk_flag = false;
        tim1_run_flag = 0;
        stopTimer(touchBTN1pin);
        fCylinderSTOP(deskCylinder);
      }
    }
  }

  if ( (btn_tim == 0 || btn_tim == touchBTN2pin) and mode == 0) { //모드0번 2버튼 독서대 아래로  : 누르는 동안 작동
    if (analogRead(touchBTN2pin) >= 900) {
      if (desk_flag == false) { // 터치가 되었을 때 엣지체크
        Serial.println("desk down BTN2");//TEST
        desk_flag = true;
        if (tim1_run_flag == 0) { //타이머가 실행되고 있는지 체크
          tim1_run_flag = 1;
          startTimer(touchBTN2pin);
          fCylinderDOWN(deskCylinder);
        }
      }

      fPhoto_test(pre_photo_desk, curr_photo_desk, &photo_cnt_desk, -1);
    }
    else {
      if (tim1_run_flag == 1) {
        desk_flag = false;
        tim1_run_flag = 0;
        stopTimer(touchBTN2pin);
        fCylinderSTOP(deskCylinder);
      }
    }
  }

  if ((btn_tim == 0 || btn_tim == touchBTN3pin) and mode == 0) { //모드0번 3버튼 : 지문 인식
    if (analogRead(touchBTN3pin) >= 900) {
      Serial.println("손가락을 올려주세요");
      int fingerId = -1;
      startTimer(touchBTN3pin);
      while (fingerId == -1) {
        fingerId = getFingerprintIDez();
        delay(50);
        if (fingerId == -1) {
          u8g.firstPage();
          do {
            u8g.drawStr(0, 22, "put on finger");
          } while (u8g.nextPage());
        }
        else {
          u8g.firstPage();
          do {
            u8g.drawStr(0, 22, "found");
          } while (u8g.nextPage());
        }
        if (tim4_cnt > 30) {
          stopTimer(touchBTN3pin);
          u8g.firstPage();
          do {
            u8g.drawStr(0, 22, "Time out");
          } while (u8g.nextPage());
          break;
        }

      }
    }
    else {
    }
  }

  if ( (btn_tim == 0 || btn_tim == touchBTN4pin) and mode == 0) { //모드0번 4버튼 : 지문 저장
    if (analogRead(touchBTN4pin) >= 900) {
      saveFingerPrint(touchBTN4pin);
    } else {
    }
  }


  if ((btn_tim == 0 || btn_tim == touchBTN1pin) and mode == 1) { //모드1번 1버튼 모니터 위로  : 누르는 동안 작동
    if (analogRead(touchBTN1pin) >= 900) {
      if (angle_flag == false) {// 터치가 되었을 때 엣지체크
        Serial.println("Moniter up BTN1");//TEST
        angle_flag = true;
        //타이머가 실행되고 있는지 체크
        if (tim1_run_flag == 0) {
          tim1_run_flag = 1;
          startTimer(touchBTN1pin);
          fCylinderUP(moniterMoveCylinder);
        }
      }

      fPhoto_test(pre_photo_move , curr_photo_move, &photo_cnt_move, 1);
    }
    else {
      if (tim1_run_flag == 1) {
        angle_flag = false;
        tim1_run_flag = 0;
        stopTimer(touchBTN1pin);
        fCylinderSTOP(moniterMoveCylinder);
      }
    }
  }
  if ( (btn_tim == 0 || btn_tim == touchBTN2pin) and mode == 1) { //모드1번 2버튼 모니터 아래로  : 누르는 동안 작동
    if (analogRead(touchBTN2pin) >= 900) {
      if (angle_flag == false) {// 터치가 되었을 때 엣지체크
        Serial.println("Moniter down BTN2");//TEST
        angle_flag = true;
        //타이머가 실행되고 있는지 체크
        if (tim1_run_flag == 0) {
          tim1_run_flag = 1;
          startTimer(touchBTN2pin);
          fCylinderDOWN(moniterMoveCylinder);
        }
      }

      fPhoto_test(pre_photo_move , curr_photo_move, &photo_cnt_move, -1);
    }
    else {
      if (tim1_run_flag == 1) {
        angle_flag = false;
        tim1_run_flag = 0;
        stopTimer(touchBTN2pin);
        fCylinderSTOP(moniterMoveCylinder);
      }
    }
  }

  if ( (btn_tim == 0 || btn_tim == touchBTN3pin) and mode == 1) { //모드1번 3버튼 모니터 각도 위로  : 누르는 동안 작동
    if (analogRead(touchBTN3pin) >= 900) {
      if (move_flag == false) { // 터치가 되었을 때 엣지체크
        Serial.println("Moniter angle up BTN3");//TEST
        move_flag = true;
        //타이머가 실행되고 있는지 체크
        if (tim1_run_flag == 0) {
          tim1_run_flag = 1;
          startTimer(touchBTN3pin);
          fCylinderUP(moniterAngleCylinder);
        }
      }
      fPhoto_test(pre_photo_angle , curr_photo_angle, &photo_cnt_angle, 1);
    }
    else {
      if (tim1_run_flag == 1) {
        move_flag = false;
        tim1_run_flag = 0;
        stopTimer(touchBTN3pin);
        fCylinderSTOP(moniterAngleCylinder);
      }
    }
  }

  if (( btn_tim == 0 || btn_tim == touchBTN4pin) and mode == 1) { //모드1번 4버튼 모니터 각도 아래로  : 누르는 동안 작동
    if (analogRead(touchBTN4pin) >= 900) {
      // 터치가 되었을 때 엣지체크
      if (move_flag == false) {
        Serial.println("Moniter angle down BTN4");//TEST
        move_flag = true;
        //타이머가 실행되고 있는지 체크
        if (tim1_run_flag == 0) {
          tim1_run_flag = 1;
          startTimer(touchBTN4pin);
          fCylinderDOWN(moniterAngleCylinder);
        }
      }
      fPhoto_test(pre_photo_angle , curr_photo_angle, &photo_cnt_angle, -1);
    }
    else {
      if (tim1_run_flag == 1) {
        move_flag = false;
        tim1_run_flag = 0;
        stopTimer(touchBTN4pin);
        fCylinderSTOP(moniterAngleCylinder);
      }
    }
  }

  //if(analogRead(touchBTN3pin) >= 900 and mode == 1){Serial.println("BTN3");}
  //if(analogRead(touchBTN4pin) >= 900 and mode == 1){Serial.println("BTN4");}

  pre_photo_move = curr_photo_move;
  pre_photo_desk = curr_photo_desk;
  pre_photo_angle = curr_photo_angle;
  delay(1);        // delay in between reads for stability
}
