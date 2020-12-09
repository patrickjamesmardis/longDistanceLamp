#include <SoftwareSerial.h>

int txPin = 2;
int rxPin = 3;
int echoPin = 5;
int trigPin = 6;
int rPin = 9;
int gPin = 10;
int bPin = 11;

SoftwareSerial espArduino(rxPin, txPin);

void setup() {
  pinMode(rxPin, INPUT);
  pinMode(echoPin, INPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(txPin, OUTPUT);
  pinMode(rPin, OUTPUT);
  pinMode(gPin, OUTPUT);
  pinMode(bPin, OUTPUT);
  Serial.begin(115200);
  espArduino.begin(115200);
  delay(1500);
}

int r = 255;
int g = 0;
int b = 0;
unsigned long lastUpdate = 0L;
bool lightOn = true;

void loop() {
  int dur, dist;
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(500);
  digitalWrite(trigPin, LOW);
  dur = pulseIn(echoPin, HIGH);
  dist = (dur / 2) / 29.1;

  
  if (dist < 30) {
    analogWrite(rPin, r);
    analogWrite(gPin, g);
    analogWrite(bPin, b);
      lastUpdate = millis();
      lightOn = true;
  } else if (lightOn && ((millis() - lastUpdate) >= 3000)) {
    analogWrite(rPin, 0);
    analogWrite(gPin, 0);
    analogWrite(bPin, 0);
    lightOn = false;
  }
  if (espArduino.available()) {
    String newColor = espArduino.readString();
    Serial.println(newColor);

    if (newColor.startsWith("1") || newColor.startsWith("2") || newColor.startsWith("3") || newColor.startsWith("4") || newColor.startsWith("5") || newColor.startsWith("6") || newColor.startsWith("7") || newColor.startsWith("8") || newColor.startsWith("9") || newColor.startsWith("0")) {
      int firstComma = newColor.indexOf(',');
      int secondComma = newColor.indexOf(',', firstComma + 1);
      r = newColor.substring(0, firstComma).toInt();
      g = newColor.substring(firstComma + 1, secondComma).toInt();
      b = newColor.substring(secondComma + 1).toInt();
      analogWrite(rPin, r);
      analogWrite(gPin, g);
      analogWrite(bPin, b);
      lightOn = true;
      lastUpdate = millis();
      
    }
  }
}
