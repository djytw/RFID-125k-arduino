/*


              DJYTW arduino RFID reader
       copyright (c) 2017 djytw. All rights reserved.



*/
#include <LiquidCrystal.h>

void init_125k() {
  // OC0A -> 6
  // OC0B -> 5
  // OC1A -> 9
  // OC1B -> 10
  // OC2A -> 11
  // OC2B -> 3
  pinMode(9, OUTPUT);
  //CTC mode, prescaler = 1
  TCCR1A = _BV(COM1A0);
  TCCR1B = _BV(WGM12) | _BV(CS10);
  OCR1A = 127;

  // f/2/Need_HZ-1 f=16M
}

unsigned char bytes;
unsigned char read_A3() {
  ADMUX = _BV(REFS0) | _BV(REFS1) | _BV(ADLAR) | 3;
  ADCSRA = _BV(ADEN) | _BV(ADSC) | _BV(ADPS2);
  ADCSRB = 0;

  while (bytes) {
    bytes = ADSC;
  }
  return ADCH;
}


unsigned char rawp;
bool raw[128];
bool pro[64];
bool lastc;
unsigned char t, cur;
void read_raw() {
  rawp = lastc = cur = 0;
  while (rawp < 128) {
    read_A3();
    read_A3();
    delayMicroseconds(12);
    t = read_A3();
    cur++;
    if (lastc && t < 5) {
      //negedge
      lastc = 0;
      if (cur > 25 || cur <= 8) {
        /*Serial.print("neg c:");
        Serial.print(cur);
        Serial.print(", ");*/
       // Serial.println(rawp);
        rawp = 0; //too long/short, restart
      }
      else if (cur > 15) {
        raw[rawp++] = 0;
        raw[rawp++] = 0;
      } else {
        raw[rawp++] = 0;
      }
      cur = 0;
    } else if (lastc == 0 && t >= 5) {
      //posedge
      lastc = 1;
      if (cur > 40 || cur <= 8){
       /* Serial.print("pos c:");
        Serial.print(cur);
        Serial.print(", ");*/
        //Serial.println(rawp);
        rawp = 0; //too long/short, restart
      }
      else if (cur > 23) {
        raw[rawp++] = 1;
        raw[rawp++] = 1;
      } else {
        raw[rawp++] = 1;
      }
      cur = 0;
    }
  }
}
void proc() {
  unsigned char i, st, count, j;
  bool start;
  for (i = 1; i < 128; i++) {
    if (raw[i] == raw[i - 1]) {
      start = i % 2;
      break;
    }
  }
  count = 0;
  for (i = start; i < 128; i += 2) {
    if (raw[i]) {
      count++;
      if (count == 9) {
        st = i + 2;
        break;
      }
    }
    else count = 0;
  }
  j = 0;
  for (i = st; i < 128; i += 2)
    pro[j++] = raw[i];
  for (i = start; i < st; i += 2)
    pro[j++] = raw[i];
}
bool check() {
  unsigned char i;
  for (i = 0; i < 50; i += 5) {
    if (pro[i + 4] != pro[i]^pro[i + 1]^pro[i + 2]^pro[i + 3])return 1;
  }
  for (i = 0; i < 4; i++) {
    if (pro[50 + i] != pro[i]^pro[5 + i]^pro[10 + i]^pro[15 + i]^pro[20 + i]^pro[25 + i]^pro[30 + i]^pro[35 + i]^pro[40 + i]^pro[45 + i])return 1;
  }
  if (pro[54])return 1;
  //check ok
  return 0;
}
unsigned long id;
unsigned char vendor;
void gen() {
  unsigned char i, j;
  vendor = id = 0;
  for (j = 0; j < 10; j += 5) {
    for (i = 0; i < 4; i++) {
      vendor *= 2;
      vendor += pro[i + j];
    }
  }
  for (j = 10; j < 50; j += 5) {
    for (i = 0; i < 4; i++) {
      id *= 2;
      id += pro[i + j];
    }
  }
}
LiquidCrystal lcd(8, 7, 6, 5, 4, 3, 2);
void setup() {

  init_125k();
  Serial.begin(250000);
  analogReference(INTERNAL);

  lcd.begin(16, 2);
  lcd.print("  RFID Reader");
  lcd.setCursor(0, 1);
  char str[]= "----------------";
  lcd.print(str);
  unsigned char i;
  for (i = 0; i < 16; i++) {
    delay(100);
    str[i] = '>';
    lcd.setCursor(0, 1);
    lcd.print(str);
  }
}
int k = 0;
char str1[17], str2[17];
void loop() {
  Serial.println(k++);
  read_raw();
  int i;
  for (i = 0; i < 128; i++)Serial.print(raw[i]);
  proc();
  Serial.print("\n");
  for (i = 0; i < 64; i++)Serial.print(pro[i]);
  if (check())Serial.println("\nCheck error");
  else {
    gen();
    Serial.print("\nID number:");
    Serial.println(id);
    Serial.print("Version:");
    Serial.println(vendor);
    sprintf(str1, "ID:%010ld %02X", id, vendor);
    sprintf(str2, "hex:%08lX  %02d", id, k % 100);
    lcd.setCursor(0, 0);
    lcd.print(str1);
    lcd.setCursor(0, 1);
    lcd.print(str2);
  }

  Serial.println("\n------------------------------------------");
}
