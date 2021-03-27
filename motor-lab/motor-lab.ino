#include <WiFi.h>
#include <HTTPClient.h>
#include <ESP32Servo.h>

#define USE_SERIAL Serial

int in1Pin = 12;      // Define L293D channel 1 pin
int in2Pin = 14;      // Define L293D channel 2 pin
int enable1Pin = 13;  // Define L293D enable 1 pin
int channel = 0;
boolean rotationDir;  // Define a variable to save the motor's rotation direction
int rotationSpeed;    // Define a variable to save the motor rotation speed
//change speed in interval of 5 sec
long previousMillis = 0;
long interval = 1000;

int outPorts[] = {33, 27, 26, 25};

int constSpeed;
int directionValue;
int photoValue = 0;

String address= "http://165.227.76.232:3000/jm4607/running";
const char *ssid_Router     = "Columbia University"; //Enter the router name
const char *password_Router = ""; //Enter the router password


void setup() {
  USE_SERIAL.begin(112500);

  WiFi.begin(ssid_Router, password_Router);
  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    USE_SERIAL.print(".");
  }

  // set pins to output
  for (int i = 0; i < 4; i++) {
    pinMode(outPorts[i], OUTPUT);
  }
  
  // Initialize the pin into an output mode:
  pinMode(in1Pin, OUTPUT);
  pinMode(in2Pin, OUTPUT);
  pinMode(enable1Pin, OUTPUT);
  
  ledcSetup(channel,1000,11);         //Set PWM to 11 bits, range is 0-2047
  ledcAttachPin(enable1Pin,channel);

  randomSeed(analogRead(0));
}

void loop() {

  if((WiFi.status() == WL_CONNECTED)) {
    HTTPClient http;
    http.begin(address);
 
    int httpCode = http.GET(); // start connection and send HTTP header
    if (httpCode == HTTP_CODE_OK) { 
        String response = http.getString();
        if (response.equals("false")) {
            // Do not run sculpture, perhaps sleep for a couple seconds
        }
        else if(response.equals("true")) {
            photoValue = analogRead(34);
            //Serial.println(photoValue);
            delay(100);
            if (photoValue > 3000) {
              //sun
              unsigned long currentMillis = millis();
              if(currentMillis - previousMillis > interval) {
                previousMillis = currentMillis;
                long randDirection = random(2);
                Serial.println(randDirection);
                directionValue = int(randDirection);
              }
              
              constSpeed = 1400;
              int speedVal = int(constSpeed);
              //Compare the number with value 2048, 
              //if more than 2048, clockwise rotates, otherwise, counter clockwise rotates
              rotationSpeed = speedVal - 2048;
          
              if (constSpeed > 2048) {
                rotationDir = true;
              } else {
                rotationDir = false;
              }
              
              // Calculate the motor speed
              rotationSpeed = abs(speedVal - 2048);
              //Control the steering and speed of the motor
              driveMotor(rotationDir, constrain(rotationSpeed,0,2048));
            } else if (photoValue < 1000) {
              //moon
              if (digitalRead(in1Pin) == HIGH || digitalRead(in2Pin) == HIGH) {
                digitalWrite(in1Pin, LOW);
                digitalWrite(in2Pin, LOW);      
              }
              moveSteps(true, 32 * 64, 3);
              delay(100);
              moveSteps(false, 32 * 64, 3);
              delay(100);
            } else {
              //thunder
              if (digitalRead(in1Pin) == HIGH || digitalRead(in2Pin) == HIGH) {
                digitalWrite(in1Pin, LOW);
                digitalWrite(in2Pin, LOW);      
              }
              //try to get servo to work?
              
            }
        }
        USE_SERIAL.println("Response was: " + response);
    } else {
        USE_SERIAL.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
    delay(500); // sleep for half of a second
  }
}

void driveMotor(boolean dir, int spd) {
  // Control motor rotation direction
  if (dir) {
    digitalWrite(in1Pin, HIGH);
    digitalWrite(in2Pin, LOW);
  } else {
    digitalWrite(in1Pin, LOW);
    digitalWrite(in2Pin, HIGH);
  }
  // Control motor rotation speed
  ledcWrite(channel, spd);
}

//Suggestion: the motor turns precisely when the ms range is between 3 and 20
void moveSteps(bool dir, int steps, byte ms) {
  for (unsigned long i = 0; i < steps; i++) {
    moveOneStep(dir); // Rotate a step
    delay(constrain(ms,3,20));        // Control the speed
  }
}

void moveOneStep(bool dir) {
  // Define a variable, use four low bit to indicate the state of port
  static byte out = 0x01;
  // Decide the shift direction according to the rotation direction
  if (dir) {  // ring shift left
    out != 0x08 ? out = out << 1 : out = 0x01;
  }
  else {      // ring shift right
    out != 0x01 ? out = out >> 1 : out = 0x08;
  }
  // Output singal to each port
  for (int i = 0; i < 4; i++) {
    digitalWrite(outPorts[i], (out & (0x01 << i)) ? HIGH : LOW);
  }
}

void moveAround(bool dir, int turns, byte ms){
  for(int i=0;i<turns;i++)
    moveSteps(dir,32*64,ms);
}
void moveAngle(bool dir, int angle, byte ms){
  moveSteps(dir,(angle*32*64/360),ms);
}
