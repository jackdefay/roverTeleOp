#include <Arduino.h>
#include <Adafruit_seesaw.h>
#include <SPI.h>
#include <RH_RF69.h>
#include "Adafruit_DRV2605.h"

#define JOYSTICK_RANGE 1023

//defs for controller
Adafruit_seesaw ss;
Adafruit_DRV2605 drv;
#define BUTTON_RIGHT 6
#define BUTTON_DOWN  7
#define BUTTON_LEFT  9
#define BUTTON_UP    10
#define BUTTON_SEL   14
uint32_t button_mask = (1 << BUTTON_RIGHT) | (1 << BUTTON_DOWN) | (1 << BUTTON_LEFT) | (1 << BUTTON_UP) | (1 << BUTTON_SEL);
#define IRQ_PIN   5

//defs for radio
#define RF69_FREQ 900.0

#define RFM69_CS      8
#define RFM69_INT     3
#define RFM69_RST     4
#define LED           13

RH_RF69 rf69(RFM69_CS, RFM69_INT);

void sendJoystickData(int x, int y);
void convertJoystickData(int coords[]);
void setHapticPower(int level);

void setup() {
  Serial.begin(115200);

  //controller setup
  ss.begin(0x4A);

  ss.pinModeBulk(button_mask, INPUT_PULLUP);
  ss.setGPIOInterrupts(button_mask, 1);
  pinMode(IRQ_PIN, INPUT);

  //radio setup
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

  //haptic motor driver setup
  drv.begin();
  drv.selectLibrary(1);

  drv.setMode(DRV2605_MODE_INTTRIG);

  //blink led on startup
  delay(1000);
  digitalWrite(13, HIGH);
  delay(500);
  digitalWrite(13, LOW);
}

void loop() {
  static int coords[2];
  coords[0] = ss.analogRead(2);
  coords[1] = ss.analogRead(3);
  int i, level;
  String word, stringLevel;
  char temp[20];
  // static unsigned long currentMillis, previousMillis;

  convertJoystickData(coords);
  //Serial.print(coords[0]); Serial.print(", "); Serial.println(coords[1]);
  sendJoystickData(coords[0], coords[1]);

  //listens for haptic data
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
              if((word.charAt(i) >= '0' && word.charAt(i) <= '9') || word.charAt(i) == '-') stringLevel += word.charAt(i);
              i++;
          }

          level = (int) stringLevel.toInt();
          setHapticPower(level);  //sets the haptic motor to the received power level
      }
  }
}

void sendJoystickData(int x, int y){
  char radiopacket[20];
  char temp[5];
  String tempWord = "####";

  //Serial.print(x); Serial.print(", "); Serial.println(y);
  itoa((int) x, temp, 10);
  tempWord = temp;
  tempWord += "* ";
  itoa((int) y, temp, 10);
  tempWord += temp;
  tempWord += "* ";
  tempWord.toCharArray(radiopacket, 20);
  //Serial.println(radiopacket);
  rf69.send((uint8_t *)radiopacket, strlen(radiopacket));
  rf69.waitPacketSent();
}

void convertJoystickData(int coords[]){
  int newx, newy;

  newx = coords[1] - (JOYSTICK_RANGE/2);  //coords[1] = old y
  newy = (JOYSTICK_RANGE/2) - coords[0];  //coords[0] = old x

  coords[0] = (int) 2*(newy * 255 /JOYSTICK_RANGE);
  coords[1] = coords[0];

  // if(newx>-20 && newx<20 && newy>20 && newy<20){
  //   coords[0] = 0;
  //   coords[1] = 0;
  // }
  if(newx > (JOYSTICK_RANGE/2)){
    coords[0] -= (int) (2*(newx * 255)/JOYSTICK_RANGE);
    coords[1] += (int) (2*(newx * 255)/JOYSTICK_RANGE);
  }
  else if(newx < (JOYSTICK_RANGE/2)){
    coords[1] -= (int) (-2*(newx * 255)/JOYSTICK_RANGE);
    coords[0] += (int) (-2*(newx * 255)/JOYSTICK_RANGE);
  }

  //coords[0] = pwmr, coords[1] = pwml
}

void setHapticPower(int level){
  int effect = 0;

  if(level == 0)
   effect = 0;
 else if(level == 1)
   effect = 51;
 else if (level == 2)
   effect = 50;
 else if (level == 3)
   effect = 49;
 else if (level == 4)
   effect = 48;
 else if (level == 5)
   effect = 47;

 drv.setWaveform(0, effect);  // play effect
 drv.setWaveform(1, 0);       // end waveform
   // plays
 drv.go();
}
