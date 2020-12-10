// software serial to communicate between arduino and esp + keep arduino's serial open for computer
#include <SoftwareSerial.h>
// header file for LED
#include <Adafruit_NeoPixel.h>

int txPin = 2;
int rxPin = 3;
int echoPin = 5;
int trigPin = 6;
//int rPin = 9;
//int gPin = 10;
//int bPin = 11;

//new led uses 1 pin for data
int ledPin = 9;
//ring of 12 LEDs
int ledCount = 12;

SoftwareSerial espArduino(rxPin, txPin);
Adafruit_NeoPixel ring(ledCount, ledPin, NEO_GRB + NEO_KHZ800);

void setup() {
  pinMode(rxPin, INPUT);
  pinMode(echoPin, INPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(txPin, OUTPUT);
  //  pinMode(rPin, OUTPUT);
  //  pinMode(gPin, OUTPUT);
  //  pinMode(bPin, OUTPUT);
  pinMode(ledPin, OUTPUT);

  Serial.begin(115200);
  espArduino.begin(115200);
  delay(1500);

//  start LED ring, but show no color until we get IoT cloud's color
  ring.begin();
  ring.show();
}

// set variables to r, g, b color values, time of last update, and light status
int r = 255;
int g = 0;
int b = 0;
unsigned long lastUpdate = 0L; // millis() returns unsigned long (https://www.arduino.cc/reference/en/language/functions/time/millis/)
bool lightOn = true;

void loop() {
// HC-SR04: arudino example at https://create.arduino.cc/projecthub/abdularbi17/ultrasonic-sensor-hc-sr04-with-arduino-tutorial-327ff6
  int dur, dist;
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  dur = pulseIn(echoPin, HIGH);
  dist = (dur * .0343) / 2;
  if (dist < 30) {
//    less than 30 cm away, show the current color
    showColor(r, g, b);
//    tell the esp to turn on the second lamp
    espArduino.println("MOTION");
//  keep track of the latest movement
    lastUpdate = millis();
    lightOn = true;
  } else if (lightOn && ((millis() - lastUpdate) >= 3000)) {
//    after 3 seconds of no motion, turn off light
    showColor(0, 0, 0);
    lightOn = false;
  }
  if (espArduino.available()) {
//    read esp serial, only lines beginning with a number are valid colors (ignores the few wifi connection messages at startup)
    String newColor = espArduino.readString();
    Serial.println(newColor);
    if (newColor.startsWith("1") || newColor.startsWith("2") || newColor.startsWith("3") || newColor.startsWith("4") || newColor.startsWith("5") || newColor.startsWith("6") || newColor.startsWith("7") || newColor.startsWith("8") || newColor.startsWith("9") || newColor.startsWith("0")) {
//      color is r,g,b so find the index of both commas
      int firstComma = newColor.indexOf(',');
      int secondComma = newColor.indexOf(',', firstComma + 1);
//      then split the string to 3 integers
      r = newColor.substring(0, firstComma).toInt();
      g = newColor.substring(firstComma + 1, secondComma).toInt();
      b = newColor.substring(secondComma + 1).toInt();
//      and finally show the color, flag light on, and update time
      showColor(r, g, b);
      lightOn = true;
      lastUpdate = millis();
    }
  }
}

void showColor(int thisR, int thisG, int thisB) {
  //    analogWrite(rPin, r);
  //    analogWrite(gPin, g);
  //    analogWrite(bPin, b);

//  set each LED in the ring to the color params
  for (int i = 0; i < ledCount; i++) {
    ring.setPixelColor(i, thisR, thisG, thisB);
  }
  ring.show();
}
