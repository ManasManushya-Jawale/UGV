#include "BluetoothSerial.h"
#include <Adafruit_BME280.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <ESP32Servo.h>
#include <Wire.h>

BluetoothSerial serialBLE;
Servo myservo;
int distance = 0;

#define RF 12
#define RB 13

#define LF 18
#define LB 19

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

#define MPU_ADDRESS 0x68 // Use 0x69 if the AD0 pin is wired to 3.3V
#define SEND_INTERVAL_MS 50
#define COMMAND_TIMEOUT_MS 500 // stop if the dashboard goes quiet

Adafruit_BME280 bme;
Adafruit_MPU6050 mpu;

float temp = 22;
float humidity = 10;
float pressure = 0;

// MPU-6050 readings
float accelX = 0, accelY = 0, accelZ = 0; // m/s^2
float gyroX = 0, gyroY = 0, gyroZ = 0;    // rad/s
bool mpuFound = false;                    // sensor initialised successfully
bool mpuOK = false;                       // latest reading is valid

uint8_t previousAngle = 16;
uint8_t scanAngle = 15;

bool direction = 1;
bool buzz = false;
bool LED = false;

unsigned long lastSendTime = 0;
unsigned long lastCommandTime = 0;

int calculateDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);

  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 25000);

  return duration * 0.034 / 2;
}

void setupMPU() {
  mpuFound = mpu.begin(MPU_ADDRESS, &Wire);

  if (!mpuFound) {
    Serial.println("MPU6050 not found!");
    return; // The rest of the car keeps working
  }

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
}

void readMPU() {
  mpuOK = false;

  if (!mpuFound) {
    return;
  }

  // Check that the sensor still answers on the I2C bus
  Wire.beginTransmission(MPU_ADDRESS);
  if (Wire.endTransmission() != 0) {
    return;
  }

  sensors_event_t accelEvent, gyroEvent, mpuTempEvent;
  if (!mpu.getEvent(&accelEvent, &gyroEvent, &mpuTempEvent)) {
    return;
  }

  accelX = accelEvent.acceleration.x;
  accelY = accelEvent.acceleration.y;
  accelZ = accelEvent.acceleration.z;

  gyroX = gyroEvent.gyro.x;
  gyroY = gyroEvent.gyro.y;
  gyroZ = gyroEvent.gyro.z;

  mpuOK = true;
}

// JSON has no "nan" or "inf", so send null instead
String jsonNumber(float value) {
  return isfinite(value) ? String(value) : String("null");
}

void sendInfo() {
  if (!serialBLE.hasClient()) {
    return;
  }

  String json = "{";
  json += "\"t\":" + String(millis()) + ",";
  json += "\"angle\":" + String(scanAngle) + ",";
  json += "\"distance\":" + String(distance) + ",";
  json += "\"temperature\":" + jsonNumber(temp) + ",";
  json += "\"humidity\":" + jsonNumber(humidity) + ",";
  json += "\"pressure\":" + jsonNumber(pressure / 100.0) + ",";
  json += "\"mpu_ok\":" + String(mpuOK ? "true" : "false") + ",";
  json += "\"ax\":" + (mpuOK ? jsonNumber(accelX) : String("null")) + ",";
  json += "\"ay\":" + (mpuOK ? jsonNumber(accelY) : String("null")) + ",";
  json += "\"az\":" + (mpuOK ? jsonNumber(accelZ) : String("null")) + ",";
  json += "\"gx\":" + (mpuOK ? jsonNumber(gyroX) : String("null")) + ",";
  json += "\"gy\":" + (mpuOK ? jsonNumber(gyroY) : String("null")) + ",";
  json += "\"gz\":" + (mpuOK ? jsonNumber(gyroZ) : String("null"));
  json += "}";

  serialBLE.println(json);
}

void controlMicellenaous() {
  digitalWrite(buzzerPin, buzz ? HIGH : LOW);
  digitalWrite(LEDPin, LED ? HIGH : LOW);
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

void moveCar(bool l, bool r) {
  toggleMotor(LF, LB, l);
  toggleMotor(RF, RB, r);
}

void stopCar() {
  digitalWrite(LF, LOW);
  digitalWrite(LB, LOW);
  digitalWrite(RF, LOW);
  digitalWrite(RB, LOW);
}

// Commands from the dashboard: F, B, L, R, S (stop)
void readCommands() {
  while (serialBLE.available()) {
    char command = serialBLE.read();

    switch (command) {
    case 'F':
      moveCar(1, 1);
      break;
    case 'B':
      moveCar(0, 0);
      break;
    case 'L':
      moveCar(0, 1);
      break;
    case 'R':
      moveCar(1, 0);
      break;
    case 'H':
      digitalWrite(BUZZER_PIN, HIGH);
      break;
    case 'h':
      digitalWrite(BUZZER_PIN, LOW);
      break;
    case 'O':
      digitalWrite(LED_PIN, HIGH);
      break;
    case 'o':
      digitalWrite(LED_PIN, LOW);
      break;
    case 'S':
      stopCar();
      break;
    default:
      continue; // ignore anything else
    }

    lastCommandTime = millis();
  }

  // Safety stop if the connection drops or commands stop arriving
  if (!serialBLE.hasClient() ||
      millis() - lastCommandTime > COMMAND_TIMEOUT_MS) {
    stopCar();
  }
}

void setup() {
  Serial.begin(115200);
  serialBLE.begin("Sentinal Car");

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  pinMode(RB, OUTPUT);
  pinMode(RF, OUTPUT);
  pinMode(LB, OUTPUT);
  pinMode(LF, OUTPUT);
  stopCar();

  pinMode(buzzerPin, OUTPUT);
  pinMode(LEDPin, OUTPUT);

  myservo.setPeriodHertz(50);
  myservo.attach(servoPin);

  Wire.begin(SDAPin, SCLPin); // SDA, SCL

  setupMPU();

  // Initialize sensor
  if (!bme.begin(0x76, &Wire)) {
    if (!bme.begin(0x77, &Wire)) {
      Serial.println("BME280 not found!");
      while (1)
        delay(10);
    }
  }
}

void loop() {
  // if (IrReceiver.decode()) {
  //   IrReceiver.printIRResultShort(&Serial);
  //   switch (IrReceiver.decodedIRData.command) {
  //     case REMOTE_F:
  //       moveCar(1, 1, 1, 1);
  //       break;
  //     case REMOTE_B:
  //       moveCar(0, 0, 0, 0);
  //       break;
  //     case REMOTE_L:
  //       moveCar(0, 1, 0, 1);
  //       break;
  //     case REMOTE_R:
  //       moveCar(1, 0, 1, 0);
  //       break;
  //     case REMOTE_HORN:
  //       buzz = !buzz;
  //       break;
  //     case REMOTE_LED:
  //       LED = !LED;
  //       break;
  //   }
  //   IrReceiver.resume();
  // }

  readCommands();

  myservo.write(scanAngle);
  delay(15);
  distance = calculateDistance();

  temp = bme.readTemperature();
  humidity = bme.readHumidity();
  pressure = bme.readPressure();

  readMPU();

  // Send before changing scanAngle, so angle and distance match
  if (millis() - lastSendTime >= SEND_INTERVAL_MS) {
    lastSendTime = millis();
    sendInfo();
  }

  scanAngle += direction ? 1 : -1;

  if (scanAngle >= 165 || scanAngle <= 15) {
    direction = !direction;
  }

  controlMicellenaous();
}
