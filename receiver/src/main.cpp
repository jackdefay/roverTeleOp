#include <Arduino.h>
#include <SPI.h>
#include <RH_RF69.h>

#define JOYSTICK_RANGE 1023

#define RF69_FREQ 900.0


//radio pins
#define RFM69_CS      8
#define RFM69_INT     3
#define RFM69_RST     4
#define LED           13

RH_RF69 rf69(RFM69_CS, RFM69_INT);

//ultrasonic pins
#define trigPin SCL //21;  //output SCL
#define echoPin SDA //= 20;  //input SDA

//motor direction pins
#define rfpos A5
#define rfneg A2
#define rbpos A1
#define rbneg 5
#define lfpos 11
#define lfneg 10
#define lbpos 9
#define lbneg 6

//motor speed pins
#define pwmrf 13
#define pwmrb 12
#define pwmlf A3
#define pwmlb A4

void setDirection(char motor, bool direction);
void setSpeed(int pwmr, int pwml);
int clip(int num);
int getUltrasonicDistance();
void sendLevel(int level);

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

  //ultrasonic pins
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  //motor pins
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

  //set motors to forwards
  setDirection('r', true);
  setDirection('l', true);
}

void loop(){
  String word, xcoord, ycoord;
  char temp[20];
  int i = 0, xcoordint, ycoordint;//, pwmr = 0, pwml = 0;
  static unsigned long previousMillis = 0, currentMillis = 0;
  int level;

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
          if((word.charAt(i) >= '0' && word.charAt(i) <= '9') || word.charAt(i) == '-') xcoord += word.charAt(i);
          i++;
        }
        i++;
        while(word.charAt(i) != '*'){
          if((word.charAt(i) >= '0' && word.charAt(i) <= '9') || word.charAt(i) == '-') ycoord += word.charAt(i);
          i++;
        }

        xcoordint = (int) xcoord.toInt();
        ycoordint = (int) ycoord.toInt();

        //Serial.print(xcoordint); Serial.print(", "); Serial.println(ycoordint);

        setSpeed(xcoordint, ycoordint);

        //after setting the speed of the motors, sends ultrasonic data back for haptics
        level = getUltrasonicDistance();
        sendLevel(level);

        previousMillis = millis();  //reset timer

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
   delay(100);                                                  //change this after done testing
}

void setDirection(char motor, bool direction){  //1 = forwards, 0 = backwards
  Serial.print("setting direction to "); Serial.println(direction);
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

  Serial.print("the values being written to the motors are: "); Serial.print(pwmr); Serial.print(", "); Serial.println(pwml);

  analogWrite(pwmrf, pwmr);
  analogWrite(pwmrb, pwmr);
  analogWrite(pwmlf, pwml);
  analogWrite(pwmlb, pwml);
}

int clip(int num){
  if(num>=-100 && num<=100) return 0;
  if(num>=0 && num<=255) return num;
  else if(num>=-255 && num<0) return -num;
  else if(num>255 || num<-255) return 255;
  else return 0;
}

int getUltrasonicDistance(){
  long duration;
  double distanceCm;

  digitalWrite(trigPin, LOW);
  delayMicroseconds(5);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  pinMode(echoPin, INPUT);
  duration = pulseIn(echoPin, HIGH);

  distanceCm = (double) duration * 0.01715;

  if(distanceCm < 20) return 5;
  else if(distanceCm < 40) return 4;
  else if(distanceCm < 60) return 3;
  else if(distanceCm < 80) return 2;
  else if(distanceCm < 100) return 1;
  else return 0;
}

void sendLevel(int level){
  char radiopacket[20];
  char temp[5];
  String tempWord = "####";

  //Serial.print(x); Serial.print(", "); Serial.println(y);
  itoa((int) level, temp, 10);
  tempWord = temp;
  tempWord += "* ";
  tempWord.toCharArray(radiopacket, 20);
  //Serial.println(radiopacket);
  rf69.send((uint8_t *)radiopacket, strlen(radiopacket));
  rf69.waitPacketSent();
}
