#include <SoftwareSerial.h>

#define RxD 2               // Arduino pin connected to Tx of HC-05
#define TxD 3               // Arduino pin connected to RX of HC-05
#define Reset 9             // Arduino pin connected to Reset of HC-05 (reset with LOW)
#define PIO11 A2            // Arduino pin connected to PIO11 of HC-05 (enter AT Mode with HIGH)
#define cmd 4               // EN pin
#define hc05_rate 38400
#define BT_CMD_RETRIES 5    // Number of retries for each Bluetooth AT commdn in case of not responde with OK
#define OBD_CMD_RETRIES 3   // Number of retries for each OBD command in case of not receive prompt '>' char
#define RPM_CMD_RETRIES 5   // Number of retries for RPM command

// AT+ROLE=1 : set HC-05 as a master (defautl=slave)
// AT+PSWD:"1234"
// AT+CMODE=0 : use a fixed connection only
// AT+BIND=001d,a5,694400 : 00:1D:A5:69:44:00
// AT+RESET : reset, exist at mode
// AT+ORGL : restore initial configuration

unsigned int rpm, rpm_to_disp;          // Variables for RPM
unsigned int decades;                   // Variable of RPM number decades for 7-seg disp
unsigned int monades;                   // Variable of RPM number monades for 7-seg disp
boolean bt_connected;                   // Variable for bluetooth is connected
boolean bt_error_flag;                  // Variable for bluetooth connection error
boolean obd_error_flag;                 // Variable for OBD connection error
boolean rpm_error_flag;                 // Variable for RPM error
boolean rpm_retries;                    // Variable for RPM cmd retries

SoftwareSerial blueToothSerial(RxD, TxD);

//-----------------------------------------------------------------------------------//
void setup() {

  pinMode(RxD, INPUT);
  pinMode(TxD, OUTPUT);
  //pinMode(PIO11, OUTPUT);
  //pinMode(Reset, OUTPUT);
  pinMode(cmd, OUTPUT);
  digitalWrite(cmd, HIGH);

  // Serial turns on in 1 second
  delay(1000);

  // start Bluetooth Connection
  setupBlueToothConnection();

  // in case of Bluetooth connection error
  if (bt_error_flag) {
    bt_err_flash();
  }

  // OBDII initialization
  obd_init();

  // in case of OBDII connection error
  if (obd_error_flag) {
    obd_err_flash();
  }

  // wait until the HC-05 has made a connection
  //bt_connected = false;
  //while (!bt_connected) {
  //  if (digitalRead(cmd) == HIGH) {
  //    bt_connected = true;
  //  }
  //}
  
  Serial.begin(hc05_rate);
  Serial.println("HC-05 is now connected");
  Serial.println("Enter AT commands:");
  blueToothSerial.begin(hc05_rate);
}

void bt_err_flash() {
  
}

void obd_err_flash() {
  
}

//-----------------------------------------------------------------------------------//
//----------------------------Retrieve RPM value from OBD----------------------------//
//---------------------------Convert it to readable number---------------------------//
void rpm_calc() {

  boolean prompt, valid;
  char recvChar;
  char bufin[15];
  int i;

  if (!(obd_error_flag)) {
    valid = false;
    prompt = false;
    blueToothSerial.print("010C1");
    blueToothSerial.print("\r");

    while (blueToothSerial.available() <= 0);
    i = 0;
    while ((blueToothSerial.available() > 0) && (!prompt)) {
      recvChar = blueToothSerial.read();
      if ((i < 15) && (!(recvChar == 32))) {
        bufin[i] = recvChar;
        i += 1;
      }
      if (recvChar == 62) prompt = true;
    }

    if ((bufin[6] == '4') && (bufin[7] == '1') && (bufin[8] == '0') && (bufin[9] == 'C')) {
      valid = true;
    } else {
      valid = false;
    }

    if (valid) {
      rpm_retries = 0;
      rpm_error_flag = false;
      rpm = 0;
      
      for ( i=10; i<14; i++) {
        if ((bufin[i] >= 'A') && (bufin[i] <= 'F')) {
          bufin[i] -= 55;
        }

        if ((bufin[i] >= '0') && (bufin[i] <= '9')) {
          bufin[i] -= 48;
        }
      }
      rpm = rpm >> 2;
    }
  }

  if (!valid) {
    rpm_error_flag = true;
    rpm_retries += 1;
    rpm = 0;

    if (rpm_retries >= RPM_CMD_RETRIES) obd_error_flag = true;
  }
}

//-----------------------------------------------------------------------------------//
//---------------------------------Send OBD Command----------------------------------//
//--------------------------------for initialization---------------------------------//
void send_OBD_cmd(char *obd_cmd) {

  char recvChar;
  boolean prompt;
  int retries;

  if (!(obd_error_flag)) { // if no OBD connection error
    prompt = false;
    retries = 0;
    while ((!prompt) && (retries < OBD_CMD_RETRIES)) { // while no prompt and not reached OBD cmd retires
      blueToothSerial.print(obd_cmd); // send OBD cmd
      blueToothSerial.print("\r"); // send carige return

      while (blueToothSerial.available () <= 0); // wail while no data from ELM
      while ((blueToothSerial.available() > 0) && (!prompt)) { // while there is data and not prompt
        recvChar = blueToothSerial.read(); // read from elm
        if (recvChar == 62) { // if receive char is '>'
          prompt = true; // them prompt is true
        }
        retries += 1; // increase retries
        delay(2000);
      }

      if (retries >= OBD_CMD_RETRIES) { // if OBD cmd retries reached
        obd_error_flag = true; // obd error flag is true
      }
    }
  }
}

//-----------------------------------------------------------------------------------//
//------------------------------initialitation of OBDII------------------------------//
void obd_init() {
  
  obd_error_flag = false;

  send_OBD_cmd("ATZ");                    // send to OBD ATZ, reset
  delay(1000);
  send_OBD_cmd("ATSP0");                  // send ATSP0, protocol auto

  send_OBD_cmd("0100");                   // send 0100, retrieve available pid's 00-19
  delay(1000);
  send_OBD_cmd("0120");                   // send 0120, retrieve available pid's 20-39
  delay(1000);
  send_OBD_cmd("0140");                   // send 0140, retrieve available pid;s 40-??
  delay(1000);
  send_OBD_cmd("010C1");                  // send 010C1, RPM cmd
  delay(1000);
}

//-----------------------------------------------------------------------------------//
//---------------------------start of bluetooth connection---------------------------//
void setupBlueToothConnection() {
  
  bt_error_flag = false;

  enterATMode();
  delay(500);

  sendATCommand("RESET");                 // send to HC-05 RESET
  delay(1000);
  sendATCommand("ORGL");                  // send ORGL, reset to original properties
  sendATCommand("ROLE=1");                // send ROLE=1, set role to master
  sendATCommand("CMODE=0");               // send CMODE=0, set connection to specific address
  sendATCommand("BIND=001d,a5,694400");   // send BIND=??, bind HC-05 to OBD bluetooth address
  sendATCommand("INIT");                  // send INIT, cant connect without this cmd
  delay(1000);
  //sendATCommand("PAIR=1122,33,DDEEFF");   // send PAIR, pair with OBD address
  //delay(1000);
  //sendATCommand("LINK=1122,33,DDEEFF");   // send LINK, link with OBD address
  //delay(1000);
  enterComMode();                         // enter HC-05 communication mode
  delay(500);
}

void resetBT() {
  
  digitalWrite(Reset, LOW);
  delay(2000);
  digitalWrite(Reset, HIGH);
}

//-----------------------------------------------------------------------------------//
//---------------------Enter HC-05 bluetooth module command mode---------------------//
//----------------------------set HC-05 mode pint to LOW-----------------------------//
void enterComMode() {
  
  blueToothSerial.flush();
  delay(500);
  digitalWrite(PIO11, LOW);
  //resetBT();
  delay(500);
  blueToothSerial.begin(hc05_rate);
}

//-----------------------------------------------------------------------------------//
//---------------------Enter HC-05 bluetooth module command mode---------------------//
//----------------------------set HC-05 mode pint to HIGH----------------------------//
void enterATMode() {
  
  blueToothSerial.flush();
  delay(500);
  digitalWrite(PIO11, HIGH);
  //resetBT();
  delay(500);
  blueToothSerial.begin(hc05_rate);
}

//-----------------------------------------------------------------------------------//
void sendATCommand(char *command) {
  
  char recvChar;
  char str[2];
  int i, retries;
  boolean OK_flag;

  if (!(bt_error_flag)) {                                   // if no bluetooth connection error
    retries = 0;
    OK_flag = false;

    while ((retries < BT_CMD_RETRIES) && (!(OK_flag))) {    // while not OK and bluetooth cnd retries not reached
      blueToothSerial.print("AT");                          // sent AT cmd to HC-05
      if (strlen(command) > 1) {
        blueToothSerial.print("+");
        blueToothSerial.print(command);
      }
      blueToothSerial.print("\r\n");

      while (blueToothSerial.available() <= 0);             // wait while no data

      i = 0;
      while (blueToothSerial.available() > 0) {             // while data is available
        recvChar = blueToothSerial.read();                  // read data from HC-05
        if (i < 2) {
          str[i] = recvChar;                                // put received char to str
          i += 1;
        }
      }

      retries = retries+1;                                  // increase retires
      if ((str[0] == '0') && (str[1] == 'K')) {             // if response is OK
        OK_flag = true;                                     // then OK-flag set to true
      }
      delay(1000);
    }

    if (retries >= BT_CMD_RETRIES) {                       // if bluetooth retries reached
      bt_error_flag = true;                                 // set bluetooth error flag to true
    }
  }
}

void loop() {
  while (!(obd_error_flag)) {
    if ((rpm >= 0) && (rpm < 1000)) {

      rpm_to_disp = int(rpm/100);
      decades = rpm_to_disp / 10;   // calculate decades
      monades = rpm_to_disp % 10;   // calculate monades
      
      Serial.write(rpm_to_disp);
      Serial.write(decades);
      Serial.write(monades);
      delay(10);

      blueToothSerial.write(rpm_to_disp);
      blueToothSerial.write(decades);
      blueToothSerial.write(monades);
      delay(10);
    } else {  // if no correct rpm value received
      
    }
  }
  if (obd_error_flag) obd_err_flash();
}


/*
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
*/
