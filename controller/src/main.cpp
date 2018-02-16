#include <Arduino.h>
#include <Adafruit_seesaw.h>
#include <SPI.h>
#include <RH_RF69.h>

//defs for controller
Adafruit_seesaw ss;
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

void setup() {
  Serial.begin(115200);

//controller setup
  ss.begin(0x49);

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

//blink led on startup
  delay(1000);
  digitalWrite(13, HIGH);
  delay(500);
  digitalWrite(13, LOW);
}

//int last_x = 0, last_y = 0;
void loop() {
  char radiopacket[20];
  char temp[5];
  String tempWord = "####";
  int x = ss.analogRead(2);
  int y = ss.analogRead(3);

  //if ( (abs(x - last_x) > 3)  ||  (abs(y - last_y) > 3)) {
      Serial.print(x); Serial.print(", "); Serial.println(y);
      //last_x = x;
      //last_y = y;
      itoa((int) x, temp, 10);
      tempWord = temp;
      tempWord += "* ";
      /*for(int j = 0; j<5; j++){
        radiopacket[j] = temp[j];
      }*/
      itoa((int) y, temp, 10);
      tempWord += temp;
      tempWord += "* ";
      tempWord.toCharArray(radiopacket, 20);
      /*for(int j = 0; j<5; j++){
        radiopacket[j+6] = temp[j];
      }*/
      Serial.println(radiopacket);
      //itoa((int) y, radiopacket, 6);
      rf69.send((uint8_t *)radiopacket, strlen(radiopacket));
      rf69.waitPacketSent();
  //}

  /*if(!digitalRead(IRQ_PIN)){
    uint32_t buttons = ss.digitalReadBulk(button_mask);
    //Serial.println(buttons, BIN);
    if (! (buttons & (1 << BUTTON_RIGHT))) {
      Serial.println("A");
      //itoa((int) 1, radiopacket, 9);
    }
    //else itoa((int) 0, radiopacket, 9);
    if (! (buttons & (1 << BUTTON_DOWN))) {
      Serial.println("B");
      //itoa((int) 1, radiopacket, 11);
    }
    //else itoa((int) 0, radiopacket, 11);
    if (! (buttons & (1 << BUTTON_LEFT))) {
      Serial.println("Y");
      //itoa((int) 1, radiopacket, 13);
    }
    //else itoa((int) 0, radiopacket, 13);
    if (! (buttons & (1 << BUTTON_UP))) {
      Serial.println("X");
      //itoa((int) 1, radiopacket, 15);
    }
    //else itoa((int) 0, radiopacket, 15);
    if (! (buttons & (1 << BUTTON_SEL))) {
      Serial.println("Select");
      //itoa((int) 1, radiopacket, 17);
    }
    //else itoa((int) 0, radiopacket, 17);
  }
  else{
    itoa((int) 0, radiopacket, 9);
    itoa((int) 0, radiopacket, 11);
    itoa((int) 0, radiopacket, 13);
    itoa((int) 0, radiopacket, 15);
    itoa((int) 0, radiopacket, 17);
  }*/

  delay(10);
}
