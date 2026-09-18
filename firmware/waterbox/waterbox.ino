//Contador de Agua con apertura/cierre remoto. GSM.
#define GSMAPN "m2mgt.tigo.com"
#define GSMUSER ""
#define GSMPASSWORD ""

#include <Time.h>
#include <sim800Client.h>
#include <GSMPubSubClient.h>
#include <TimeAlarms.h>
#include "settings.h"
#include <ArduinoJson.h> // https://github.com/bblanchon/ArduinoJson/releases/tag/v5.0.7

//Pines usados por default (2,3)Serial1, (5,7) powerpin, reset sim800 Interrupt 2 is PIN D6 for Sensor(reserved)
const int BIA = 9; // (pwm) pin 9 connected to pin B-IA 
const int BIB = 10;  // (pwm) pin 10 connected to pin B-IB

byte speed = 255;  // change this (0-255) to control the speed of the motors

// --- variables de verificacion de fallas de capa de conexion con servicio
int failed, sent, published; //variables de conteo de envios 
       
sim800Client s800;
char imeicode[16];
char msg[300];
boolean ValveState;
String myplString = "";
// Update these with values suitable for your network.
//byte server[] = { 192, 168, 1, 199 };
char server[] = "iotarduinodaygt.flatbox.io";

//------------------------General Flow senseor variables
byte statusLed    = 13;

byte sensorInterrupt = 2;  // 0 = digital pin 2
byte sensorPin       = 6;  //solo para 644P ATMEL verificar el pin interruptor libre para otro MCU

// The hall-effect flow sensor outputs approximately 4.5 pulses per second per
// litre/minute of flow.
float calibrationFactor = 4.5;

volatile byte pulseCount;  

float flowRate;
unsigned int flowLitres;
unsigned long totalLitres;

unsigned long oldTime;
//---------------finish variables de sensor flow------------------

void callback(char* topic, byte* payload, unsigned int length) {
  // handle message arrived
  char mypl[48];
  Serial.println(length);
  memcpy(mypl,payload,length);
  mypl[length]=char(0);
  Serial.print(F("receive: "));
  Serial.print(topic);
  Serial.print(F("->"));
  Serial.println(mypl);
  String myplString(mypl);
  Serial.println(myplString);
  if (myplString == "on"){
    ValveOn();
    ValveState = true;
    Serial.print("valve is true:");
    Serial.println(myplString);
  }
  if (myplString == "off"){
    ValveOff();
    ValveState = false;
    Serial.print("valve is false:");
    Serial.println(myplString);
  }
  
}

PubSubClient client(server, 1883, callback, s800);


void pub()
{
  buildJson ();
  Serial.print(F("send: "));
  Serial.print(F("waterbox/in"));
  Serial.print(F("->"));
  Serial.println(msg);
  sent ++;
  if (client.publish(publishTopic, msg)){
    Serial.println(F("Publish OK"));
    published ++;
    failed = 0; 
  }else {
    Serial.println(F("Publish FAILED"));
    failed ++;    
  }
  //client.publish(publishTopic, msg);
}

void buildJson() {
  String ISO8601 = "27/07/2016:13:19";
  int flow = random(1, 33);
  StaticJsonBuffer<300> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  JsonObject& d = root.createNestedObject("d");
  JsonObject& data = d.createNestedObject("data");
  data["imei"] = imeicode;
  data["flujo"] = flow;
  data["Lm"] = flowRate;
  data["Ls"] = flowLitres;
  data["TL"] = totalLitres;  
  data["timestamp"] = ISO8601;
  root.printTo(msg, sizeof(msg));
  Serial.println(F("publishing device metadata:")); 
  Serial.println(msg);
}

void ManageData() {
  String Lat = "14.606228";
  String Long = " -90.612996";
  String ClienteID = "Abastesa";
  int flow = random(1, 33);
  StaticJsonBuffer<300> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  JsonObject& d = root.createNestedObject("d");
  JsonObject& data = d.createNestedObject("data");
  data["Lat"] = Lat;
  data["Long"] = Long;
  data["ClienteID"] = ClienteID;
  char ManageMSG[300];
  root.printTo(ManageMSG, sizeof(ManageMSG));
  Serial.println(F("publishing device metadata:")); 
  Serial.println(ManageMSG);
  sent ++;
  if (client.publish(manageTopic, ManageMSG)){
    Serial.println(F("Publish OK"));
    published ++;
    failed = 0; 
  }else {
    Serial.println(F("Publish FAILED"));
    failed ++;    
  }
}

void setup(){
  //-----------------------pines de valvula-----------
  pinMode(BIA, OUTPUT);
  pinMode(BIB, OUTPUT);
  //----------------------inicio de conexion----------  
  Serial.begin(19200);
  Serial.println(F("SIM800 Shield testing."));
  //Ethernet.begin(mac, ip);
  for (int i=0; i<10; i++){
    delay(5000);
    Serial.println(F("try to init sim800"));
    #ifdef HARDWARESERIAL
    if (s800.init( 5, 7)) break;
    #else
    if (s800.init(&Serial1 , 5, 7)) break;
    #endif
  }
  
  Serial.println(F("try to setup sim800"));
  s800.setup();
  s800.stop();
  s800.TCPstop();
  s800.getIMEI(imeicode);
  Serial.print(F("IMEI: "));
  Serial.println(imeicode);

  while (!s800.TCPstart(GSMAPN,GSMUSER,GSMPASSWORD)) {
    Serial.println(F("TCPstart failed"));
    s800.TCPstop();
    delay(1000);
  }
  
  Serial.println(F("TCPstart started"));
  while (!client.connect(imeicode)) {
    Serial.println(F("connect failed"));
    delay(1000);
  }
  //---------------------------envio de datos de administracion.
  Serial.println(F("connected"));
  ManageData();
  client.subscribe(subscribeTopic);
  Alarm.timerRepeat(60, pub);             // timer
  //--------------------------------inicio de sensor de flujo--------
  // Set up the status LED line as an output
  pinMode(statusLed, OUTPUT);
  digitalWrite(statusLed, HIGH);  // We have an active-low LED attached
  
  pinMode(sensorPin, INPUT);
  digitalWrite(sensorPin, HIGH);

  pulseCount        = 0;
  flowRate          = 0.0;
  flowLitres   = 0;
  totalLitres  = 0;
  oldTime           = 0;

  // The Hall-effect sensor is connected to pin 2 which uses interrupt 0.
  // Configured to trigger on a FALLING state change (transition from HIGH
  // state to LOW state)
  attachInterrupt(sensorInterrupt, pulseCounter, FALLING);
  
}


void loop(){
  if((millis() - oldTime) > 1000){ // Only process counters once per second
    // Disable the interrupt while calculating flow rate and sending the value to
    // the host
    detachInterrupt(sensorInterrupt);
    
    // Because this loop may not complete in exactly 1 second intervals we calculate
    // the number of milliseconds that have passed since the last execution and use
    // that to scale the output. We also apply the calibrationFactor to scale the output
    // based on the number of pulses per second per units of measure (litres/minute in
    // this case) coming from the sensor.
    flowRate = ((1000.0 / (millis() - oldTime)) * pulseCount) / calibrationFactor;
    
    // Note the time this processing pass was executed. Note that because we've
    // disabled interrupts the millis() function won't actually be incrementing right
    // at this point, but it will still return the value it was set to just before
    // interrupts went away.
    
    oldTime = millis();
    
    // Divide the flow rate in litres/minute by 60 to determine how many litres have
    // passed through the sensor in this 1 second interval, then multiply by 1000 to
    // convert to millilitres.
    
    flowLitres = (flowRate / 60)*1000;
    
    // Add the millilitres passed in this second to the cumulative total
    
    totalLitres += flowLitres;
      
    unsigned int frac;
    
    // Print the flow rate for this second in litres / minute
    Serial.print("Flow rate: ");
    Serial.print(int(flowRate));  // Print the integer part of the variable
    Serial.print(".");             // Print the decimal point
    // Determine the fractional part. The 10 multiplier gives us 1 decimal place.
    frac = (flowRate - int(flowRate)) * 10;
    Serial.print(frac, DEC) ;      // Print the fractional part of the variable
    Serial.print("L/min");
    // Print the number of litres flowed in this second
    Serial.print("  Current Liquid Flowing: ");             // Output separator
    Serial.print(flowLitres);
    Serial.print("L/Sec");

    // Print the cumulative total of litres flowed since starting
    Serial.print("  Output Liquid Quantity: ");             // Output separator
    Serial.print(totalLitres);
    Serial.println("L"); 

    // Reset the pulse counter so we can start incrementing again
    pulseCount = 0;
    
    // Enable the interrupt again now that we've finished sending output
    attachInterrupt(sensorInterrupt, pulseCounter, FALLING);
  }  
  client.loop();
  Alarm.delay(100); 
}

/*
Insterrupt Service Routine
 */
void pulseCounter()
{
  // Increment the pulse counter
  pulseCount++;
}

void ValveOff(){
  analogWrite(BIA, 0);
  analogWrite(BIB, speed);
  delay(25);
  analogWrite(BIA, 0);
  analogWrite(BIB, 0);
}

void ValveOn(){
  analogWrite(BIA, speed);
  analogWrite(BIB, 0);
  delay (25);
  analogWrite(BIA, 0);
  analogWrite(BIB, 0);
}


