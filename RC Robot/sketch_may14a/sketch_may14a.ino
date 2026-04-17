#define CUSTOM_SETTINGS
#define INCLUDE_GAMEPAD_MODULE
#include <DabbleESP32.h>
#include <ESP32Servo.h>

Servo myservo1, myservo2;

int PIN_IN1 = 23;
int PIN_IN2 = 5;
int PIN_IN3 = 13;
int PIN_IN4 = 12;
int servo1pos = 0;

void setup() {
  Serial.begin(115200);      
  Dabble.begin("SABERTOOTH");    
  pinMode(PIN_IN1, OUTPUT);
  pinMode(PIN_IN2, OUTPUT);
  pinMode(PIN_IN3, OUTPUT);
  pinMode(PIN_IN4, OUTPUT);
  myservo1.attach(25);
  myservo2.attach(26);
}

void loop() {
  Dabble.processInput();

  if (GamePad.isUpPressed()) {
    digitalWrite(PIN_IN1, LOW);
    digitalWrite(PIN_IN2, HIGH);
    digitalWrite(PIN_IN3, LOW);
    digitalWrite(PIN_IN4, HIGH);
  }

  if (GamePad.isDownPressed())
  {
    digitalWrite (PIN_IN1, HIGH);
    digitalWrite (PIN_IN2, LOW);
    digitalWrite (PIN_IN3, HIGH);
    digitalWrite (PIN_IN4, LOW);
  }

  if (GamePad.isLeftPressed())
  {
    digitalWrite (PIN_IN1, HIGH);
    digitalWrite (PIN_IN2, LOW);
    digitalWrite (PIN_IN3, LOW);
    digitalWrite (PIN_IN4, HIGH);
  }
  if (GamePad.isRightPressed())
  {
    digitalWrite (PIN_IN1, LOW);
    digitalWrite (PIN_IN2, HIGH);
    digitalWrite (PIN_IN3, HIGH);
    digitalWrite (PIN_IN4, LOW);
  }

  if (!GamePad.isUpPressed() && !GamePad.isDownPressed() && !GamePad.isLeftPressed() && !GamePad.isRightPressed() && !GamePad.isSquarePressed() && !GamePad.isCirclePressed() && !GamePad.isCrossPressed() && !GamePad.isTrianglePressed() && !GamePad.isStartPressed() && !GamePad.isSelectPressed()) {
    digitalWrite(PIN_IN1, LOW);
    digitalWrite(PIN_IN2, LOW);
    digitalWrite(PIN_IN3, LOW);
    digitalWrite(PIN_IN4, LOW);
  }

  if (GamePad.isSquarePressed()){
    for (int pos = servo1pos ; pos <= 70; pos += 1) {  
      myservo1.write(pos);
    }
    servo1pos = 70 ;
  }

  if (GamePad.isCirclePressed()) {
    for (int pos = servo1pos ; pos >= 10; pos -= 1) {  
      myservo1.write(pos);
    }
    servo1pos = 0;
  }

  if (GamePad.isTrianglePressed()) {
    Serial.println("BUKA");
    myservo2.write(60);
  }

  if (GamePad.isCrossPressed()) {
    Serial.println("TUTUP");
    myservo2.write(-5);
  }
}