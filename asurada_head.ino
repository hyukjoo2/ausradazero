#include <Servo.h>
int servoPin = 9;
Servo servo;

int led_eye = 13;

int angle_eye_spin_start = 0;
int angle_eye_spin_finish = 90;
int delay_eye_spin = 550;

void setup() {
  // put your setup code here, to run once:
  pinMode(led_eye, OUTPUT);
  
  servo.attach(servoPin);
  servo.write(angle_eye_spin_start);
  delay(1000);
}

void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(led_eye, HIGH);
  
  eyeSpin();
  delay(3000);
}

void eyeSpin()
{
  servo.write(angle_eye_spin_start);
  delay(delay_eye_spin);
  servo.write(angle_eye_spin_finish);
  delay(delay_eye_spin);
  servo.write(angle_eye_spin_start);
  delay(delay_eye_spin);
}
