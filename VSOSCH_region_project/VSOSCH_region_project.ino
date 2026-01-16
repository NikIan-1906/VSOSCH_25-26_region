/*
!!!DO NOT DELETE!!!
Данная программа является базовым управляющим кодом для робота, собранного под ЕТЗ проектного тура регионального этапа ВСОШ 2025-2026.
Любое копирование, сборка, модификация и загрузка данной программы без разрешения разработчика будет считаться нарушением Авторских прав.
Разработчик: Яниогло Никита, nikian1906@gmail.com
*/
#include <EEPROM.h>
#include <LiquidCrystal.h>

#define SERV 31
#define MIN_I 550
#define MAX_I 2490
#define X A0
#define LINE A1
#define Y A2
#define STEP 0.04665
#define GREY 15
#define SU 72
#define SD 89
#define ST 150000

struct Eeprom {
  double marker_additional = 0;
};

Eeprom mem;

LiquidCrystal lcd(43, 41, 39, 37, 35, 33);

uint8_t m_dir[2] = {48, 42};
uint8_t m_sp[2] = {46, 44};
volatile double dist = 0;
char* programs[] = {"C", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10",};
uint8_t program = 255;

void isrA() { //encoder polling, giving relative distance (static), in [mm] | [INTERRUPT] [COMPLETE]
  dist += STEP*((digitalRead(20))?1:-1);
}

uint8_t moveDist(int16_t distance){ //moves the robot for given distance, in [mm] | [ASYNC] [COMPLETE]
  bool direction = (distance-dist)>=0;
  uint8_t pwr = constrain(abs(distance-dist)*100, 0, 150);
  digitalWrite(m_dir[1], direction);
  analogWrite(m_sp[1], direction?255-pwr:pwr);
  return pwr;
}

void servoControl(uint8_t deg, uint32_t mcs){ //control servo for given degree | [ASYNC] [COMPLETE]
  uint32_t tmr = micros();
  while (micros()-tmr < mcs) {
    uint16_t impulse = map(deg, 0, 180, MIN_I, MAX_I);
    static uint32_t tmr1, tmr2;
    if (micros()-tmr1 > 20000) {
      tmr1 = tmr2 = micros();
      digitalWrite(SERV, 1);
    }
    if (micros()-tmr2 > impulse){
      digitalWrite(SERV, 0);
      tmr2 += 30000;
    }
  }
}

void dot(int16_t pos) {
  while (moveDist(pos) > 10) continue;
  servoControl(SD, ST);
  servoControl(SU, ST);
}

void line(int16_t start, int16_t end) {
  while(moveDist(start) > 10) continue;
  servoControl(SD, ST);
  while(moveDist(end) > 10) continue;
  servoControl(SU, ST);
}

void programChoose(){
  int8_t plast = -1;
  uint8_t len = (sizeof(programs)/sizeof(char*));
  int8_t pos = 0;
  uint32_t tmr4 = micros();
  while (program == 255) {
    if(pos != plast){
      lcd.clear();
      lcd.home();
      lcd.write(programs[0]);
      for(int i = 1; i < len; i++){
        lcd.write(' ');
        lcd.write(programs[i]);
      }
      lcd.setCursor(2*pos, 1);
      lcd.print("\x87");
      if(pos > 3){
        for(int j = 5; j < 2*min(pos, len-5); j++) lcd.scrollDisplayLeft();
      }
      plast = pos;
    }
    if(abs(512-analogRead(X))>200 && micros()-tmr4>250000){
      tmr4 = micros();
      pos += (analogRead(X)-512>0)?1:-1;
      pos = constrain(pos, 0, len-1);
    }
    if(analogRead(Y)>750){
      program = pos;
      lcd.clear();
      lcd.home();
      lcd.write("Program ");
      lcd.write(programs[program]);
      lcd.write(" start");
      lcd.setCursor(0, 1);
      lcd.print("Running...");
      digitalWrite(49, OUTPUT);
      break;
    }
  }
}

void calibrate(){
  if (program != 255) {
    while (analogRead(LINE) < GREY) {
      moveDist(dist+1.5);
    }
    dist = 0;
    while (moveDist(10) > 30) continue;
    while (analogRead(LINE) > GREY) moveDist(dist+1.5);
    dist = 0;
    float iadd = 0;
    while(analogRead(Y) > 200) {
      float add = (float)map(analogRead(X), 0, 1023, -1, 2)/100;
      iadd += add;
      moveDist(iadd);
    }
    mem.marker_additional = dist;
    EEPROM.put(0, mem);
    while (analogRead(LINE) < GREY) moveDist(-100);
    dist = 0;
    while (moveDist(-60) > 10) continue;
    digitalWrite(49, 0);
    program = 255;
  }
}

void p1() {
  if (program != 255) {
    while (analogRead(LINE) < GREY) {
      moveDist(dist+1.5);
    }
    dist = 0;
    while (moveDist(10) > 30) continue;
    while (analogRead(LINE) > GREY) {
      moveDist(dist+1.5);
    }
    dist = 0;
    while(moveDist(mem.marker_additional) > 10) continue;
    dist = 0;
    while (1) {
      //main prog code
      delay(10000);
      break;
    }
    while (moveDist(-60) > 10) continue;
    digitalWrite(49, 0);
    program = 255;
  }
}

void p2() {
  if (program != 255) {
    while (analogRead(LINE) < GREY) {
      moveDist(dist+1.5);
    }
    dist = 0;
    while (moveDist(10) > 30) continue;
    while (analogRead(LINE) > GREY) {
      moveDist(dist+1.5);
    }
    dist = 0;
    while(moveDist(mem.marker_additional) > 10) continue;
    dist = 0;
    while (1) {
      //main prog code
      dot(10);
      dot(20);
      dot(30);
      line(40, 50);
      line(60, 70);
      line(80, 90);
      dot(100);
      dot(110);
      dot(120);
      break;
    }
    while (moveDist(-60) > 10) continue;
    digitalWrite(49, 0);
    program = 255;
  }
}

void setup() {
  pinMode(SERV, OUTPUT);
  servoControl(SU, 150000);
  EEPROM.get(0, mem);

  lcd.begin(16, 2);
  Serial.begin(115200);

  attachInterrupt(4, isrA, RISING);

  for(int i = 0; i < 2; i++){
    pinMode(m_dir[i], OUTPUT);
    pinMode(m_sp[i], OUTPUT);
  }
  pinMode(49, OUTPUT);
}
//moveDist(300); // robot moving forward | [LOOPED] [REFERENCE]
void loop() {
  switch(program){
    case 0: calibrate();
    case 1: p1();
    case 2: p2();

    default:programChoose();
  }
}
