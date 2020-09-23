#include <SoftwareSerial.h>

#define RxD 2               // Arduino pin connected to Tx of HC-05
#define TxD 3               // Arduino pin connected to RX of HC-05
#define cmd 4

// AT+ROLE=1 : set HC-05 as a master (defautl=slave)
// AT+PSWD:"1234"
// AT+CMODE=0 : use a fixed connection only
// AT+BIND=001d,a5,694400 : 00:1D:A5:69:44:00
// AT+RESET : reset, exist at mode
// AT+ORGL : restore initial configuration

SoftwareSerial blueToothSerial(RxD, TxD);

//-----------------------------------------------------------------------------------//
void setup() {

  pinMode(RxD, INPUT);
  pinMode(TxD, OUTPUT);
  pinMode(cmd, OUTPUT);
  digitalWrite(cmd, HIGH);

  Serial.begin(38400);
  Serial.println("HC-05 is now connected");
  Serial.println("Enter AT commands:");
  blueToothSerial.begin(38400);
}

void loop() {
  
  if (blueToothSerial.available()) {
    char inByte = blueToothSerial.read();
    Serial.write(inByte);
  }
  
  if (Serial.available()) {
    char inByte = Serial.read();
    blueToothSerial.write(inByte);
  }
}
