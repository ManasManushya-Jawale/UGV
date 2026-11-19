#include <IRremote.hpp>
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <ESP32Servo.h>
#include "BluetoothSerial.h"

BluetoothSerial serialBLE;
Servo myservo;
int distance = 0;

// calibrate all this when remote comes
#define REMOTE_F 0x11
#define REMOTE_B 0x12
#define REMOTE_L 0x13
#define REMOTE_R 0x14
#define REMOTE_HORN 0x15
#define REMOTE_LED 0x16

#define BLF 13
#define BLB 14
#define BRF 16
#define BRB 17

#define FLF 18
#define FLB 19
#define FRF 23
#define FRB 25

#define REMOTE_PIN 35

#define LDRPin 34

#define trigPin 27
#define echoPin 33

#define SDAPin 21
#define SCLPin 22

#define servoPin 26

#define buzzerPin 32
#define LEDPin 4

#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME280 bme;

float temp = 22;
float humidity = 10;
float pressure = 0;

uint8_t previousAngle = 16;
uint8_t scanAngle = 15;

bool direction = 1;
bool buzz = false;
bool LED = false;

int calculateDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);

  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 25000);

  return duration * 0.034 / 2;
}

void sendInfo() {

  String json = "{";
  json += "\"angle\":" + String(scanAngle) + ",";
  json += "\"distance\":" + String(distance) + ",";
  json += "\"temperature\":" + String(temp) + ",";
  json += "\"humidity\":" + String(humidity) + ",";
  json += "\"pressure\":" + String(pressure / 100.0);
  json += "}";

  if (serialBLE.available()) {
    serialBLE.println(json);
  }
}

void controlMicellenaous() {
  digitalWrite(buzzerPin, buzz ? HIGH:LOW);
  digitalWrite(LEDPin, LED ? HIGH:LOW);
}

void toggleMotor(uint8_t forward, uint8_t backward, bool v) {
  if (v) {
    digitalWrite(forward, HIGH);
    digitalWrite(backward, LOW);
  } else {
    digitalWrite(forward, LOW);
    digitalWrite(backward, HIGH);
  }
}

void moveCar(bool fl, bool fr, bool bl, bool br) {
  toggleMotor(FLF, FLB, fl);
  toggleMotor(FRF, FRB, fr);
  toggleMotor(BLF, BLB, bl);
  toggleMotor(BRF, BRB, br);
}

void setup() {
  serialBLE.begin("Sentinal Car");
  Serial.begin(115200);
  IrReceiver.begin(REMOTE_PIN, ENABLE_LED_FEEDBACK);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  pinMode(REMOTE_PIN, INPUT);

  for (uint8_t i = 15; i <= 22; i++) {
    pinMode(i, OUTPUT);
  }

  myservo.setPeriodHertz(50);
  myservo.attach(servoPin);

  Serial.begin(115200);

  Wire.begin(SDAPin, SCLPin);  // SDA, SCL

  // Initialize sensor
  if (!bme.begin(0x76, &Wire)) {
    if (!bme.begin(0x77, &Wire)) {
      Serial.println("BME280 not found!");
      while (1) delay(10);
    }
  }
}

void loop() {
  
  if (IrReceiver.decode()) {
    IrReceiver.printIRResultShort(&Serial);
    switch (IrReceiver.decodedIRData.command) {
      case REMOTE_F:
        moveCar(1, 1, 1, 1);
        break;
      case REMOTE_B:
        moveCar(0, 0, 0, 0);
        break;
      case REMOTE_L:
        moveCar(0, 1, 0, 1);
        break;
      case REMOTE_R:
        moveCar(1, 0, 1, 0);
        break;
      case REMOTE_HORN:
        buzz = !buzz;
        break;
      case REMOTE_LED:
        LED = !LED;
        break;
    }
    IrReceiver.resume();
  }

  myservo.write(scanAngle);
  delay(15);
  distance = calculateDistance();

  scanAngle += direction ? 1 : -1;

  if (scanAngle >= 165 || scanAngle <= 15) {
    direction = !direction;
  }

  temp = bme.readTemperature();
  humidity = bme.readHumidity();
  pressure = bme.readPressure();

  controlMicellenaous();

}
