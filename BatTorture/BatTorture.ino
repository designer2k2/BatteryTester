//Battery Tester by Stephan Martin
//http://www.designer2k2.at
// 03.07.2012 Initial Version
// 31.08.2012 Start to add Ri Measurement (1sec free, 1sec 1A, 1sec free, 1sec 2A, 1sec free, 1sec 3A, 1sec free) NO Protection
// 09.09.2014 Extend Ri Measurement (take last sample and calc Ri)
// 17.09.2014 Start to add Capacity measurement, per chanel we have a 3.9Ohms R
// 10.10.2014 Capacity works, now add Wh, and make Ri automatically, maybe use timer to get 100ms intervall?

#include<stdlib.h> //for dtostrf(FLOAT,WIDTH,PRECSISION,BUFFER);
static char dtostrfbuffer1[20];

const int analogInPin = 0;  // Analog input pin that the Battery is attached to

int sensorValue = 0;        // value read from the Battery
float SensorVoltage = 0.0;
int ChargeInput = 0;
unsigned long time;
unsigned long Start = 0;
int Mosfets = 0;
boolean RunIt = false;
boolean ChargeIt = false;

float FreeVoltage, Volt1A, Volt2A, Volt3A, Ri1A, Ri2A, Ri3A;

float fCapacity = 0.0;
unsigned long lasttime = 0;

void setup() {

  //Define Mosfet Pins as Outputs
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);

  //And the Charge IC Pins:
  pinMode(4, OUTPUT); //Current select (High 500ma, Low 100ma)
  pinMode(2, OUTPUT); //Enable (High On, Low Off)

  //and give them a clear off:
  digitalWrite(5, LOW);
  digitalWrite(6, LOW);
  digitalWrite(7, LOW);
  digitalWrite(4, LOW);
  digitalWrite(2, LOW);

  // initialize serial communications at 9600 bps:
  Serial.begin(57600);

  pinMode(13, OUTPUT);

  // Ask for the Mosfets
  IntroTextSend();

}

void loop() {



  // check if data has been sent from the computer:
  if (Serial.available()) {
    Mosfets = Serial.parseInt();
    //Serial.print(Mosfets);

    switch (Mosfets) {
      case 1:
        //1 Mosfet (Center)
        Serial.println("Activate the center, 1A!");
        digitalWrite(2, LOW);
        digitalWrite(4, LOW);
        ChargeIt = false;
        digitalWrite(5, LOW);
        digitalWrite(6, HIGH);
        digitalWrite(7, LOW);
        RunIt = true;
        break;
      case 2:
        //2 Mosfet (edges)
        Serial.println("Activate the edges, 2A!");
        digitalWrite(2, LOW);
        digitalWrite(4, LOW);
        ChargeIt = false;
        digitalWrite(5, HIGH);
        digitalWrite(6, LOW);
        digitalWrite(7, HIGH);
        RunIt = true;
        break;
      case 3:
        //3 Mosfet (all)
        Serial.println("Activate ALL, 3A!");
        digitalWrite(2, LOW);
        digitalWrite(4, LOW);
        ChargeIt = false;
        digitalWrite(5, HIGH);
        digitalWrite(6, HIGH);
        digitalWrite(7, HIGH);
        RunIt = true;
        break;
      case 4:
        //Charge with 100mA
        Serial.println("Charge with 100mA");
        digitalWrite(5, LOW);
        digitalWrite(6, LOW);
        digitalWrite(7, LOW);
        digitalWrite(2, HIGH);
        digitalWrite(4, LOW);
        RunIt = false;
        ChargeIt = true;
        fCapacity = 0.0;
        break;
      case 5:
        //Charge with 500mA
        Serial.println("Charge with 500mA");
        digitalWrite(5, LOW);
        digitalWrite(6, LOW);
        digitalWrite(7, LOW);
        digitalWrite(2, HIGH);
        digitalWrite(4, HIGH);
        RunIt = false;
        ChargeIt = true;
        fCapacity = 0.0;
        break;
      case 6:
        //Just print current Voltage
        // read the analog in value:
        sensorValue = analogRead(analogInPin);
        SensorVoltage = sensorValue * (5.0 / 1023.0);
        Serial.print("Current Voltage: ");
        Serial.println(SensorVoltage);
        break;
      case 7:
        //Hardcoded Ri Measurement
        Serial.println("Starting Ri Measurement, no Undervolt Protection!");
        Serial.println("Best to use 50% Charged LiPos (3,8V)");
        delay(2000);
        RunIt = false;
        ChargeIt = false;
        fCapacity = 0.0;
        RiMeasurement();
        break;
      default:
        Serial.println("");
        Serial.print("NO Idea what: ");
        Serial.print(Mosfets);
        Serial.println(" should be.");
        // read the analog in value:
        sensorValue = analogRead(analogInPin);
        SensorVoltage = sensorValue * (5.0 / 1023.0);
        Serial.print("Current Voltage: ");
        Serial.println(SensorVoltage);
        RunIt = false;
        ChargeIt = false;
        digitalWrite(5, LOW);
        digitalWrite(6, LOW);
        digitalWrite(7, LOW);
        digitalWrite(4, LOW);
        digitalWrite(2, LOW);
        fCapacity = 0.0;
        IntroTextSend();

    }

    Start = millis();
    lasttime = Start;
  }


  // read the analog in value:
  sensorValue = analogRead(analogInPin);
  SensorVoltage = sensorValue * (5.0 / 1023.0);

  ChargeInput = ChargeInput * 0.5 + analogRead(1) * 0.5 ; //Reads the CHG Input from the Charger IC

  //if the Voltage is below 3.2V Turn on the LED, and OFF Them Mosfets!
  if (SensorVoltage < 3.2) {
    digitalWrite(13, HIGH);
    digitalWrite(5, LOW);
    digitalWrite(6, LOW);
    digitalWrite(7, LOW);
    if (RunIt == true) {
      Serial.println("Shutoff!!!!! UNDERVOLTAGE!!!!");
      RunIt = false;
      fCapacity = 0.0;
      IntroTextSend();
    }
  } else
  {
    digitalWrite(13, LOW);
  }

  //fetch the time:
  time = millis();

  // print the results to the serial monitor (only if needed)
  if (RunIt == true) {
    PrintLogLine(Mosfets);
  }

  // Watches over the Charging Process:

  if (ChargeIt == true) {

    if (ChargeInput <= 400) {
      Serial.print(time - Start);
      Serial.print(";");
      Serial.print(SensorVoltage);
      Serial.print(";");
      Serial.println(ChargeInput);
    } else
    {
      //Charge Finished
      ChargeIt = false;
      digitalWrite(4, LOW);
      digitalWrite(2, LOW);
      Serial.println("Shutoff!!!!! LiPo Full!!!");
      IntroTextSend();
    }

  }

  // wait 100 milliseconds before the next loop
  delay(98);
  delayMicroseconds(600);
}


//---------------------------------------------------------------------------------

void RiMeasurement() {

  int iTimepassed;

  Start = millis();

  //1Sec Free
  digitalWrite(5, LOW);
  digitalWrite(6, LOW);
  digitalWrite(7, LOW);
  digitalWrite(13, HIGH);

  do {
    PrintLogLine(0);
    delay(10);
    iTimepassed = time - Start;
  } while (iTimepassed <= 100);

  FreeVoltage = SensorVoltage;

  //1Sec 1A
  digitalWrite(5, LOW);
  digitalWrite(6, HIGH);
  digitalWrite(7, LOW);
  digitalWrite(13, LOW);

  do {
    PrintLogLine(1);
    delay(10);
    iTimepassed = time - Start;
  } while (iTimepassed <= 200);

  Volt1A = SensorVoltage;
  Ri1A = (FreeVoltage - Volt1A) / 1.0;

  //1Sec Free
  digitalWrite(5, LOW);
  digitalWrite(6, LOW);
  digitalWrite(7, LOW);
  digitalWrite(13, HIGH);

  do {
    PrintLogLine(0);
    delay(10);
    iTimepassed = time - Start;
  } while (iTimepassed <= 300);

  FreeVoltage = SensorVoltage;

  //1Sec 2A
  digitalWrite(5, HIGH);
  digitalWrite(6, LOW);
  digitalWrite(7, HIGH);
  digitalWrite(13, LOW);

  do {
    PrintLogLine(2);
    delay(10);
    iTimepassed = time - Start;
  } while (iTimepassed <= 400);

  Volt2A = SensorVoltage;
  Ri2A = (FreeVoltage - Volt2A) / 2.0;

  //1Sec Free
  digitalWrite(5, LOW);
  digitalWrite(6, LOW);
  digitalWrite(7, LOW);
  digitalWrite(13, HIGH);

  do {
    PrintLogLine(0);
    delay(10);
    iTimepassed = time - Start;
  } while (iTimepassed <= 500);

  FreeVoltage = SensorVoltage;

  //1Sec 3A
  digitalWrite(5, HIGH);
  digitalWrite(6, HIGH);
  digitalWrite(7, HIGH);
  digitalWrite(13, LOW);

  do {
    PrintLogLine(3);
    delay(10);
    iTimepassed = time - Start;
  } while (iTimepassed <= 600);

  Volt3A = SensorVoltage;
  Ri3A = (FreeVoltage - Volt3A) / 3.0;

  //1Sec Free

  digitalWrite(5, LOW);
  digitalWrite(6, LOW);
  digitalWrite(7, LOW);
  digitalWrite(13, HIGH);

  do {
    PrintLogLine(0);
    delay(10);
    iTimepassed = time - Start;
  } while (iTimepassed <= 700);

  FreeVoltage = SensorVoltage;

  Serial.println("Ri Measurement done. Result:");

  Serial.print("Ri 1A: ");
  Serial.print(Ri1A * 1000.0);
  Serial.print("mOhm Ri 2A: ");
  Serial.print(Ri2A * 1000.0);
  Serial.print("mOhm Ri 3A: ");
  Serial.print(Ri3A * 1000.0);
  Serial.println("mOhm");

}

void IntroTextSend() {
  Serial.println("What to do??");
  Serial.println("Enter 1-3 to Discharge wit 1-3A");
  Serial.println("Enter 4-5 to charge with 100/500mA");
  Serial.println("Enter 6 to show current voltage");
  Serial.println("Enter 7 to make a Ri Cycle");

}

void PrintLogLine(int iC) {

  unsigned long deltatime;
  float deltacap;

  //fetch the time:
  time = millis();

  // read the analog in value:
  sensorValue = analogRead(analogInPin);
  SensorVoltage = sensorValue * (5.0 / 1023.0);

  // make the capacity measurement:
  deltatime = time - lasttime;
  deltacap = (SensorVoltage / (3.9 / iC)) * deltatime; //This should be Ams
  fCapacity = fCapacity + deltacap;

  float fCap2;
  fCap2 = fCapacity / 3600.0; // from Ams to mAh

  String sCapacity;
  dtostrf(fCap2, 6, 2, dtostrfbuffer1);
  sCapacity = dtostrfbuffer1;
  sCapacity.replace(".", ",");
  sCapacity.replace(" ", "");

  String sSensorVoltage;
  dtostrf(SensorVoltage, 6, 2, dtostrfbuffer1);
  sSensorVoltage = dtostrfbuffer1;
  sSensorVoltage.replace(".", ",");
  sSensorVoltage.replace(" ", "");


  Serial.print(time - Start);
  Serial.print(";");
  Serial.print(sSensorVoltage);
  Serial.print(";");
  Serial.print(sCapacity);
  Serial.print(";");
  Serial.println(iC);

  lasttime = time;

}
