// ================================================
//  Arduino Ultrasonic Radar System (Fixed Version)
//  Servo + HC-SR04 Ultrasonic Sensor
// ================================================

#include <Servo.h>

// Pin definitions
const int trigPin = 10;
const int echoPin = 11;

// Servo object
Servo myServo;

// Variables
long duration;
int distance;

void setup() 
{
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  
  Serial.begin(9600);           // Must match Processing baud rate
  myServo.attach(12);           // Servo on pin 12
  
  // Give servo time to initialize
  delay(1000);
}

void loop() 
{
  // Sweep from 15° to 190°
  for (int angle = 15; angle <= 190; angle += 1) 
  {
    myServo.write(angle);
    delay(15);                    // Increased for better servo settling
    
    distance = calculateDistance();

    // Clean and reliable output format: "angle,distance\n"
    Serial.print(angle);
    Serial.print(",");
    Serial.println(distance);     // println adds \n automatically (cleaner)
  }

  // Sweep back from 190° to 15°
  for (int angle = 190; angle >= 15; angle -= 1) 
  {
    myServo.write(angle);
    delay(15);

    distance = calculateDistance();

    Serial.print(angle);
    Serial.print(",");
    Serial.println(distance);
  }
}

// Ultrasonic distance calculation
int calculateDistance() 
{
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH, 30000);   // Timeout to prevent hanging
  
  // Convert to cm (speed of sound ≈ 0.034 cm/µs)
  distance = duration * 0.034 / 2;

  // Basic range limit (HC-SR04 is not reliable beyond ~400cm)
  if (distance > 400 || distance <= 0) {
    distance = 400;   // Treat as "out of range"
  }

  return distance;
}