/*******************************
   KATAKATA MEIRO !!
 ******************************/

#include <LiquidCrystal_I2C.h>   //LCD
#include <Wire.h>                   //LCD
#include <Servo.h>                 //Servo motor
#include <SNESpaduino.h>          //Super Famicon controller
#include <SPI.h>
//#include <SD.h>
#include "Ws2822s.h"
#define NUM_PIXELS 20
#define MAX_VAL 8  // 0 to 255 for brightness
uint16_t colval = 0; 
#define LED_PIN 1 // WS2822SのDAIピンにつなげるArduinoのピン番号

Ws2822s LED(LED_PIN, NUM_PIXELS);

#define SENSOR 2                    //goal sensor
#define PIEZO 3                    //beep
//#define chipSelect  4              //for SD
#define SERVO_L 6
#define SERVO_R 5
#define LATCH 7                     //orange line 
#define DAT 8                      //red line 
#define CLOCK 9                     //yellow line 

//#define MOSI 11
//#define MISO 12
//#define CLK 13
#define READY A0
#define START A1
#define FIN A2
#define SELECT A3

int MOTOR_DELAY = 15;       //must over 8[msec] (= MaxVelocity * MotorSpeed )
/******                         MaxVelocity = 3deg/Loop (up+left)
*******                         MotorSpeed = 160msec/60deg               */

const int CTR = 90;                         //center angle
uint16_t btns = 0b11111111111;        //button input
uint16_t pre_btns = 0b11111111111;    //button input(連続押し判定防止のため２度読み)
bool Sound = HIGH;                    //If HIGH ,sound on ,else off


//File dataFile;

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);
Servo servo_l;
Servo servo_r;
SNESpaduino pad(LATCH, CLOCK, DAT);  //controller

/*----------------------------setup 初期設定------------------------------*/

void setup()
{
  pinMode(READY,  OUTPUT);
  pinMode(START,  OUTPUT);
  pinMode(SELECT,  OUTPUT);
  pinMode(FIN,  OUTPUT);
  pinMode(SERVO_L,  OUTPUT);
  pinMode(SERVO_R,  OUTPUT);
  pinMode(PIEZO,  OUTPUT);
  pinMode(SENSOR,  INPUT);
  servo_l.attach(SERVO_L);
  servo_r.attach(SERVO_R);
  lcd.begin(16, 2);
  lcd.clear();
  lcd.backlight();
  Serial.begin(9600);
  /*  Serial.print("Initializing SD card...");
    if (!SD.begin(chipSelect)) {
      Serial.println("Card failed, or not present");
      return;
    }
    Serial.println("card initialized.");
  */
  Serial.print(0);                         //subBDに指令
}

/*------------------------------main status ここから開始------------------------------*/
void loop() {
  /* ---液晶表示---*/
  unsigned long previousMillis = 0;
  const long interval = 500;
  bool flip = LOW;
  int i = 0;

  while (1) {
    unsigned long currentMillis = millis();              //currentMillisに現在の時間を記憶
    digitalWrite(READY, HIGH);
    if (currentMillis - previousMillis > 25) {
      previousMillis = currentMillis;
      colval++;
    }
    for (int i = 0; i < NUM_PIXELS; i++) {
      int j = ((i * 256 / NUM_PIXELS) + colval) & 255;
      if (j < 85) {
        LED.setColor(i, (j * 3) * MAX_VAL / 255, (255 - j * 3) * MAX_VAL / 255, 0);
      }
      else if (j < 170) {
        j -= 85;
        LED.setColor(i,(255 - j * 3) * MAX_VAL / 255, 0, (j * 3) * MAX_VAL / 255);
      } 
      else {
        j -= 170;
        LED.setColor(i,0, (j * 3) * MAX_VAL / 255, (255 - j * 3) * MAX_VAL / 255);
      }
      LED.send();
    }

    /*
        if (currentMillis - previousMillis > interval) {     //"press START"と"KataKta Meiro !!"をinterval間隔で交互表示
          if (i < 10) {
            lcd.setCursor(0, 0);
            if (flip == LOW)      lcd.print("press START     ");
            else   lcd.print("                ");
            flip =  !flip;
          }
          else  if (i < 20) {
            lcd.setCursor(0, 0);
            lcd.print("KataKta Meiro !!");
          }
          previousMillis = currentMillis;
          i++;
          if (i == 30)i = 0;
        }
    */
    //---ボタン読み取り---//

    pre_btns = pad.getButtons(false);                         // 連押し判定防止のため50msecあけて再判定
    delay(50);                                                //
    btns = pad.getButtons(false);                             //
    /*--------------------*/
    if (!(btns & BTN_START) & (!(pre_btns) == 0)) {          // STARTでゲーム開始
      game_mode();
      break;
    }
    if (!(btns & BTN_SELECT) & (!(pre_btns) == 0)) {         // SELECTでリセット
      status_reset();
      break;
    }
    /*
          if (!(btns & (BTN_B)) & (!(pre_btns) == 0)) {    //Bでリプレイモード
          replay_mode();
          break;
        }
    */
    if (!(btns & (BTN_L | BTN_R)) & (!(pre_btns) == 0)) {    // L+Rでテストモード
      test_mode();
      break;
    }
  }
}

/*------------------------------game mode------------------------------*/
void game_mode() {
  int pos_l = CTR , pos_r = CTR ;

  unsigned long    startMillis = 0, timecounter = 0;
  bool sensor = HIGH, pres;

  status_reset();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Ready");
  Serial.print(3);                         //subBDに指令
  for (int i = 3; i > 0; i--) {
    lcd.setCursor(0, 1);
    lcd.print(i);
    digitalWrite(READY, HIGH);
    beep(1000, 500);
    delay(500);
    digitalWrite(READY, LOW);
    delay(500);
  }
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Go !!");
  beep(2000, 1000);
  digitalWrite(START, HIGH);
  Serial.print(1);                         //subBDに指令
  //  SD.remove("datalog.txt");
  //  dataFile = SD.open("datalog.txt", FILE_WRITE);
  //  Serial.println("save start");

  startMillis = millis();
  while (1) {
    btns = pad.getButtons(false);

    if (!(btns & BTN_UP)) {
      pos_l += 2;
      pos_r -= 2;
    }
    if (!(btns & BTN_DOWN)) {
      pos_l -= 2;
      pos_r += 2;
    }
    if (!(btns & BTN_LEFT)) {
      pos_l -= 2;
      pos_r -= 2;
    }
    if (!(btns & BTN_RIGHT)) {
      pos_l += 2;
      pos_r += 2;
    }
    if (!(btns & (BTN_UP | BTN_LEFT)))  pos_r -= 3 ;   // = MaxVelocity
    if (!(btns & (BTN_UP | BTN_RIGHT)))  pos_l += 3 ;
    if (!(btns & (BTN_DOWN | BTN_LEFT))) pos_l -= 3 ;
    if (!(btns & (BTN_DOWN | BTN_RIGHT))) pos_r += 3 ;

    if (pos_l < 30)pos_l = 30;
    if (pos_l > 150)pos_l = 150;
    if (pos_r < 30)pos_r = 30;
    if (pos_r > 150)pos_r = 150;

    servo_l.write(pos_l);                            //move motors
    servo_r.write(pos_r);

    //    dataFile.write(pos_l);
    //    dataFile.write(pos_r);
    timecounter = millis() - startMillis;
    sensor = digitalRead(SENSOR);

    if ((sensor == LOW) & (pres == HIGH)) {
      unsigned long finishMillis = timecounter;    //finishMillisにラップタイムを代入
      //      Serial.print("save closed");
      //      dataFile.close();
      Serial.print(2);                         //subBDに指令
      lcd.setCursor(0, 1);
      lcd.print(add_point(finishMillis));
      digitalWrite(FIN,  HIGH);
      beep(2000, 100);
      lcd.setCursor(0, 0);
      for (int i = 0; i < 6 ; i++) {
        lcd.setCursor(0, 0);
        if (i % 2 == 0) lcd.print("Finish !!       ");
        else  lcd.print("                ");
        delay(500);
      }
      return;
    }
    else {                                          //not yet finish
      lcd.setCursor(0, 1);
      lcd.print(add_point(timecounter));
      if (!(btns & BTN_SELECT)) {
        //        Serial.print("save closed");
        //        dataFile.close();
        lcd.setCursor(0, 0);
        lcd.print("Suspended");
        for (int i = 0; i < 3; i++) {
          beep(1000, 100);
          delay(200);
        }
        status_reset();
        delay(2000);
        return;
      }
      pres = sensor;
    }
    delay(MOTOR_DELAY);
  }
}

/*------------------------------status reset------------------------------*/
void status_reset() {
  Serial.print(0);                         //subBDに指令
  servo_l.write(CTR);
  servo_r.write(CTR);
  digitalWrite(SELECT, HIGH);
  digitalWrite(READY, HIGH);
  digitalWrite(START, LOW);
  digitalWrite(FIN, LOW);
  delay(100);
  digitalWrite(SELECT, LOW);
}

/*------------------------------replay mode-----------------------------*/
/*
  void replay_mode() {
  int pos;
  Serial.println("load start");
  dataFile = SD.open("datalog.txt");
  if (dataFile) {
    while (dataFile.available()) {
      Serial.println("aberable");
      pos = dataFile.read();
      servo_l.write(pos);
      Serial.print(pos);
      Serial.print(",");
      pos = dataFile.read();
      servo_r.write(pos);
      Serial.println(pos);
      delay(MOTOR_DELAY);
    }
    dataFile.close();
    Serial.println("load done");
  }
  }
*/
/*------------------------------test mode-なくてもよい-----------------------------*/
void test_mode() {
  char message[][16] = {
    "1.Beep on/off",
    "2.START",
    "3.LED_ARRAY1",
    "4.LED_ARRAY2",
    "5.FIN",
    "6.SELECT",
    "7.Sensor adjust",
    "8.Mario Theme",
  };
  digitalWrite(READY, HIGH);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Test mode");
  lcd.setCursor(0, 1);
  lcd.print("[L+R] to return");
  delay(1000);
  lcd.clear();

  int i = 0 , cursor = 0, label;
  int m_num = sizeof(message) / sizeof(message[16]);

  while (1) {
    if (i < 0)i += m_num;
    if (i >= m_num ) i -= m_num;
    lcd.setCursor(0, cursor);
    lcd.print(">");
    lcd.setCursor(0, 1 - cursor);
    lcd.print(" ");
    lcd.setCursor(1, 0);
    lcd.print(message[i]);
    lcd.setCursor(1, 1);
    lcd.print(message[(i + 1) % m_num]);
    label = i + cursor + 1;
    if (label > m_num) label = label % m_num;
    pre_btns = pad.getButtons(false);
    delay(50);
    btns = pad.getButtons(false);

    if (!(btns & BTN_UP) & (!(pre_btns) == 0) ) {
      if (cursor == 1) cursor = 0;
      else {
        lcd.clear();
        i--;
      }
    }
    else if (!(btns & BTN_DOWN) & (!(pre_btns) == 0)) {
      if (cursor == 0)  cursor = 1;
      else {
        lcd.clear();
        i++;
      }
    }
    else if (!(btns &  BTN_A) & (!(pre_btns) == 0)) {
      switch (label) {
        case 1:
          sound_switch();
          break;
        case 2:
          sound_test(START);
          break;
        case 3:

          break;
        case 4:

          break;
        case 5:
          sound_test(FIN);
          break;
        case 6:
          sound_test(SELECT);
          break;
        case 7:
          digitalWrite(READY, HIGH);
          break;
        case 8:


          break;
      }
    }
    else if (!(btns & (BTN_L | BTN_R)) & (!(pre_btns) == 0)) {
      lcd.clear();
      delay(100);
      return;
    }
    pre_btns == pad.getButtons(false);
    delay(100);
  }
}

/*-------------------------------------------------------------*/
void beep(int frequency, int duration) {
  if (Sound == HIGH) tone(PIEZO, frequency, duration);
  else {}
}

void  sound_switch() {
  int cursor = 0;
  lcd.clear();
  delay(200);
  while (1) {
    lcd.setCursor(0, cursor);
    lcd.print(">");
    lcd.setCursor(0, 1 - cursor);
    lcd.print(" ");
    lcd.setCursor(1, 0);
    lcd.print("Sound on");
    lcd.setCursor(1, 1);
    lcd.print("Sound off");
    while (1) {
      if (!btns == 0)  break;
    }
    pre_btns = pad.getButtons(false);
    delay(50);
    btns = pad.getButtons(false);
    if (!(btns & BTN_UP) & (!(pre_btns) == 0)) {
      cursor = 0;
    }
    else if (!(btns & BTN_DOWN) & (!(pre_btns) == 0)) {
      cursor = 1;
    }
    else if (!(btns & BTN_A) & (!(pre_btns) == 0)) {
      if (cursor == 0)  Sound = HIGH;
      else  Sound = LOW;
      lcd.setCursor(0, 1 - cursor);
      lcd.print("                ");
      beep(2000, 500);
      delay(500);
      return;
    }
  }
}

String add_point(unsigned long n) {     // millisec  to  "sec + "." + millisec"
  String tmp;
  tmp = n / 1000;
  tmp += ".";
  tmp += n % 1000;
  return tmp;
}

void sound_test(int sw) {
  digitalWrite(sw, HIGH);
  delay(50);
  digitalWrite(sw, LOW);
  delay(50);
  return;
}
