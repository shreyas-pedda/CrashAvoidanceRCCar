#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <Adafruit_NeoPixel.h>

#define LEFT_WIRE 13 // Dark Blue - 1
#define RIGHT_WIRE 12 // Turquoise - 2
#define UP_WIRE 14 // Red - 3, yellow  25
#define DOWN_WIRE 27 // Black - 4, black

#define TRIG1 15
#define ECHO1 2
#define TRIG2 16
#define ECHO2 4
#define TRIG3 17
#define ECHO3 5

#define DI 32
#define ringshow_noglitch() {delay(1);ring.show();delay(1);ring.show();}
Adafruit_NeoPixel ring = Adafruit_NeoPixel(16, DI, NEO_GRB + NEO_KHZ800);


typedef struct joystick_input {
  int x; // from -1800 to 1800
  int y; // from -1800 to 1800
  // deadzone from -110 to 100
  bool control; // initial value 1
} joystick_input;

joystick_input joystick_data;
const int pwmFrequency = 200; // PWM frequency in Hz
const int pwmResolution = 8;   // PWM resolution in bits (8-bit resolution for ESP32)

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&joystick_data, incomingData, sizeof(joystick_data));
  Serial.print("Bytes received: ");
  Serial.println(len);
  Serial.print("x: ");
  Serial.println(joystick_data.x);
  Serial.print("y: ");
  Serial.println(joystick_data.y);
  Serial.print("button: ");
  Serial.println(joystick_data.control);
  Serial.println();

}

void setup(){
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);

  pinMode(LEFT_WIRE, OUTPUT);
  pinMode(RIGHT_WIRE, OUTPUT);
  pinMode(UP_WIRE, OUTPUT);
  pinMode(DOWN_WIRE, OUTPUT);
  Serial.println(WiFi.macAddress());
  
  pinMode(TRIG1, OUTPUT); // set up distance1 sensor pins
  pinMode(ECHO1, INPUT);
  digitalWrite(TRIG1, LOW);
  pinMode(TRIG2, OUTPUT); // set up distance2 sensor pins
  pinMode(ECHO2, INPUT);
  digitalWrite(TRIG2, LOW);
  pinMode(TRIG3, OUTPUT); // set up distance3 sensor pins
  pinMode(ECHO3, INPUT);
  digitalWrite(TRIG3, LOW);

  ledcSetup(1, pwmFrequency, pwmResolution);
  ledcSetup(2, pwmFrequency, pwmResolution);
  ledcSetup(3, pwmFrequency, pwmResolution);
  ledcSetup(4, pwmFrequency, pwmResolution);      
  ledcAttachPin(UP_WIRE, 1);
  ledcAttachPin(DOWN_WIRE, 2);
  ledcAttachPin(RIGHT_WIRE, 3);
  ledcAttachPin(LEFT_WIRE, 4);    
  ledcWrite(1, 0); 
  ledcWrite(2, 0); 
  ledcWrite(3, 0); 
  ledcWrite(4, 0); 

  ring.begin();
  ring.setBrightness(32);
  ring.clear(); // clear all pixels
  ring.setPixelColor(0,0);
  ringshow_noglitch(); 
}

// read distance sensor, return centimeters
float readDistance(int TRIG, int ECHO) {
  digitalWrite(TRIG, LOW); delayMicroseconds(3);
  digitalWrite(TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  unsigned long timeout = micros() + 26233L;
  while((digitalRead(ECHO) == LOW) && (micros() < timeout));
  unsigned long start_time = micros();
  timeout = start_time + 26233L;
  while((digitalRead(ECHO) == HIGH) && (micros() < timeout));
  unsigned long lapse = micros() - start_time;
  return lapse * 0.01716f;
}

// calls readDistance a certain amount of time 
float averageDistance(int TRIG, int ECHO){
  unsigned long totalLapse = 0;
  for (int i = 0; i < 3; i++) {
    totalLapse += readDistance(TRIG, ECHO);
  }
  return totalLapse / 4;
}

void updateMotors(joystick_input joystick_data, float dist1, float dist2, float dist3){
  bool obstacleDetected = (dist1 < 4) || (dist2 < 30) || (dist3 < 4);

   // If obstacle detected, stop all motor movements
   if (obstacleDetected && joystick_data.control) {
     ledcWrite(1, 0); 
     ledcWrite(2, 0); 
     ledcWrite(3, 0); 
     ledcWrite(4, 0); 
     return; 
   }

  // Map joystick input to PWM duty cycle
  int dutyX = map(abs(joystick_data.x), 0, 1800, 0, 255);
  int dutyY = map(abs(joystick_data.y), 0, 1800, 0, 150);
  
  if (joystick_data.x < 0){
    ledcWrite(3, 0);
    ledcWrite(4, dutyX);
  } 
  else if (joystick_data.x == 0) {
    ledcWrite(4, 0);
    ledcWrite(3, 0);
  }
  else{
    ledcWrite(4, 0);
    ledcWrite(3, dutyX);
  }
  
  if (joystick_data.y < 0){
    ledcWrite(1, 0);
    ledcWrite(2, dutyY);
  } else if (joystick_data.y == 0){
      ledcWrite(1, 0);
      ledcWrite(2, 0);
  }
  else{
    ledcWrite(2, 0);
    ledcWrite(1, dutyY);
  }
}

void loop(){
  float dist1 = averageDistance(TRIG1, ECHO1);
  float dist2 = averageDistance(TRIG2, ECHO2);
  float dist3 = averageDistance(TRIG3, ECHO3);
  updateMotors(joystick_data, dist1, dist2, dist3);

  for(int i=0; i < ring.numPixels(); i++) {
    if (joystick_data.control){
      ring.setPixelColor(i, ring.Color(0, 255, 0));  
    } else {
      ring.setPixelColor(i, ring.Color(255, 0, 0));  
    }
  }
  ringshow_noglitch();

  
}
