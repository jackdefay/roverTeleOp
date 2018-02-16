#include <Arduino.h>
#include <SPI.h>
#include <RH_RF69.h>

#define JOYSTICK_RANGE 1020

#define RF69_FREQ 900.0

  #define RFM69_CS      8
  #define RFM69_INT     3
  #define RFM69_RST     4
  #define LED           13

RH_RF69 rf69(RFM69_CS, RFM69_INT);

#define rfpos A5
#define rfneg A2
#define rbpos A1
#define rbneg 5
#define lfpos 11
#define lfneg 10
#define lbpos 9
#define lbneg 6

#define pwmrf 13
#define pwmrb 12
#define pwmlf A3
#define pwmlb A4

void setDirection(char motor, bool direction);
void setSpeed(int pwmr, int pwml);
int clip(int num);

void setup() {
  Serial.begin(115200);

  Serial.println("starting");

  pinMode(LED, OUTPUT);
  pinMode(RFM69_RST, OUTPUT);
  digitalWrite(RFM69_RST, LOW);

    // manual reset
  digitalWrite(RFM69_RST, HIGH);
  delay(10);
  digitalWrite(RFM69_RST, LOW);
  delay(10);

  if (!rf69.init()) {
    //Serial.println("RFM69 radio init failed");
    while (1);
  }
  Serial.println("RFM69 radio init OK!");
  if (!rf69.setFrequency(RF69_FREQ)) {
    //Serial.println("setFrequency failed");
  }
  rf69.setTxPower(20, true);  // range from 14-20 for power, 2nd arg must be true for 69HCW
  uint8_t key[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
  rf69.setEncryptionKey(key);

  pinMode(LED, OUTPUT);

  pinMode(rfpos, OUTPUT);
  pinMode(rfneg, OUTPUT);
  pinMode(rbpos, OUTPUT);
  pinMode(rbneg, OUTPUT);
  pinMode(lfpos, OUTPUT);
  pinMode(lfneg, OUTPUT);
  pinMode(lbpos, OUTPUT);
  pinMode(lbneg, OUTPUT);
  pinMode(pwmrf, OUTPUT);
  pinMode(pwmrb, OUTPUT);
  pinMode(pwmlf, OUTPUT);
  pinMode(pwmlb, OUTPUT);

  setDirection('r', true);
  setDirection('l', true);

}

void loop(){
  String word, xcoord, ycoord;
  char temp[20];
  int i = 0, xcoordint, ycoordint, pwmr = 0, pwml = 0;
  static unsigned long previousMillis = 0, currentMillis = 0;

  if (rf69.available()) {
      uint8_t buf[RH_RF69_MAX_MESSAGE_LEN];
      uint8_t len = sizeof(buf);
      if (rf69.recv(buf, &len)) {
        if (!len) return;
        buf[len] = 0;
        Serial.println((char*)buf);
        for(int i = 0; i<20; i++){
          temp[i] = (char) buf[i];
        }
        word = temp;
        i = 0;
        while(word.charAt(i) != '*'){
          if(word.charAt(i) >= '0' && word.charAt(i) <= '9') xcoord += word.charAt(i);
          i++;
        }
        i++;
        while(word.charAt(i) != '*'){
          if(word.charAt(i) >= '0' && word.charAt(i) <= '9') ycoord += word.charAt(i);
          i++;
        }
        xcoordint = xcoord.toInt();
        ycoordint = ycoord.toInt();
        Serial.print(xcoordint); Serial.print(", "); Serial.println(ycoordint);

        //set pwm levels for writing to motors!
        pwmr = (int) ycoordint-(JOYSTICK_RANGE/2) * 255 /JOYSTICK_RANGE;  //***just going to code it to do turns for now, will add rotation functionality later***
        pwml = pwmr;
        if(xcoordint > JOYSTICK_RANGE/2) pwmr -= (int) ((xcoordint * 255)/JOYSTICK_RANGE - 125.5);
        else if(xcoordint < JOYSTICK_RANGE/2) pwml -= (int) (125.5 - (xcoordint * 255)/JOYSTICK_RANGE);

        setSpeed(pwmr, pwml);
        Serial.print(pwmr); Serial.print(", "); Serial.println(pwml);
        previousMillis = millis();

        digitalWrite(13, HIGH);
      }

      else {
        Serial.println("Receive failed");
      }
   }
   else {
     digitalWrite(13, LOW);
     Serial.println("no signal");
   }

   currentMillis = millis();
   if(currentMillis - previousMillis > 100) setSpeed(0, 0);
   delay(100);
}

void setDirection(char motor, bool direction){  //1 = forwards, 0 = backwards
  Serial.println("In function setDirection");
  if(motor == 'r'){
    digitalWrite(rfpos, (int) direction);
    digitalWrite(rfneg, (int) !direction);
    digitalWrite(rbpos, (int) direction);
    digitalWrite(rbneg, (int) !direction);
  }
  else if(motor == 'l'){                        //the left wheels are reversed
    digitalWrite(lfpos, (int) !direction);
    digitalWrite(lfneg, (int) direction);
    digitalWrite(lbpos, (int) !direction);
    digitalWrite(lbneg, (int) direction);
  }
}

void setSpeed(int pwmr, int pwml){
  if(pwmr>=0) setDirection('r', true);
  else setDirection('r', false);

  if(pwml>=0) setDirection('l', true);
  else setDirection('l', false);

  pwmr = clip(pwmr);
  pwml = clip(pwml);

  analogWrite(pwmrf, pwmr);
  analogWrite(pwmrb, pwmr);
  analogWrite(pwmlf, pwml);
  analogWrite(pwmlb, pwml);
}

int clip(int num){
  if(num>0 && num<255) return num;
  else if(num>255) return 255;
  else return 0;
}
