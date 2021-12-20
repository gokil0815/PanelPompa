#include <FirebaseESP8266.h>
#include <common.h>
#include <Firebase.h>
#include <FirebaseFS.h>
#include <Utils.h>
#include <ESP8266WiFi.h>
#include "SimpleModbusMaster.h"
#include <SoftwareSerial.h>

#if (defined(__AVR__) || defined(ESP8266)) && !defined(__AVR_ATmega2560__)
// For UNO and others without hardware serial, we must use software serial...
// pin #2 is IN from sensor (GREEN wire)
// pin #3 is OUT from arduino  (WHITE wire)
// Set up the serial port to use softwareserial..

#else
// On Leonardo/M0/etc, others with hardware serial, use hardware serial!
// #0 is green wire, #1 is white
#define mySerial Serial1

#endif

#define baud 9600
#define timeout 1000
#define polling 200 // the scan rate

#define retry_count 10 

// used to toggle the receive/transmit pin on the driver
#define TxEnablePin 14

//PORT OPTOCOUPLER
int L1 = 5; //Run
int L2 = 4; //Stop
int L3 = 16; //Trip
int L4 = 12; //Press
int L5 = 0; //PLN
int L6 = 13; //WLC

//PORT SOLID STATE RELAY
int CH1 = 15; //Pin Relay 1
int CH2 = 2; //Pin Relay 2

//KONFIGURASI WIFI
String ssid = "Nihdia";
String pass = "1231231231";

//SET FIREBASE
FirebaseJson json;
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

//SET BOOL RELAY
int relay1;
int relay2;

enum
{
  PACKET1,
  PACKET2,
  PACKET3,
  PACKET4,
  PACKET5,
  PACKET6,
  PACKET7,
  PACKET8,
  PACKET9,
  // leave this last entry
  TOTAL_NO_OF_PACKETS
};

// Create an array of Packets for modbus_update()
Packet packets[TOTAL_NO_OF_PACKETS];

packetPointer packet1 = &packets[PACKET1];
packetPointer packet2 = &packets[PACKET2];
packetPointer packet3 = &packets[PACKET3];
packetPointer packet4 = &packets[PACKET4];
packetPointer packet5 = &packets[PACKET5];
packetPointer packet6 = &packets[PACKET6];
packetPointer packet7 = &packets[PACKET7];
packetPointer packet8 = &packets[PACKET8];
packetPointer packet9 = &packets[PACKET9];

unsigned int volta[2];
unsigned int voltb[2];
unsigned int voltc[2];
unsigned int arusa[2];
unsigned int arusb[2];
unsigned int arusc[2];
unsigned int fps[2];
unsigned int frek[2];
unsigned int wpp[2];

unsigned long timer;

float f_2uint_float(unsigned int uint1, unsigned int uint2) {    // reconstruct the float from 2 unsigned integers

  union f_2uint {
    float f;
    uint16_t i[2];
  };

  union f_2uint f_number;
  f_number.i[0] = uint1;
  f_number.i[1] = uint2;
  

  return f_number.f;

}

void setup() {
  Serial.begin(9600);
  
  //deklarasi port OPTOCOUPLER
  pinMode(L1, INPUT);
  pinMode(L2, INPUT);
  pinMode(L3, INPUT);
  pinMode(L4, INPUT);
  pinMode(L5, INPUT);
  pinMode(L6, INPUT);
  
  //deklarasi port SSR
  pinMode(CH1, OUTPUT);//deklarasi port SSR
  pinMode(CH2, OUTPUT);//deklarasi port SSR
  digitalWrite(CH1, HIGH);

  //deklarasi power meter
  packet1->id = 1;
  packet1->function = READ_HOLDING_REGISTERS;
  packet1->address = 9;
  packet1->no_of_registers = 2;
  packet1->register_array = volta;

  packet2->id = 1;
  packet2->function = READ_HOLDING_REGISTERS;
  packet2->address = 11;
  packet2->no_of_registers = 2;
  packet2->register_array = voltb;

  packet3->id = 1;
  packet3->function = READ_HOLDING_REGISTERS;
  packet3->address = 13;
  packet3->no_of_registers = 2;
  packet3->register_array = voltc;

  packet4->id = 1;
  packet4->function = READ_HOLDING_REGISTERS;
  packet4->address = 15;
  packet4->no_of_registers = 2;
  packet4->register_array = arusa;

  packet5->id = 1;
  packet5->function = READ_HOLDING_REGISTERS;
  packet5->address = 17;
  packet5->no_of_registers = 2;
  packet5->register_array = arusb;

  packet6->id = 1;
  packet6->function = READ_HOLDING_REGISTERS;
  packet6->address = 19;
  packet6->no_of_registers = 2;
  packet6->register_array = arusc;

  packet7->id = 1;
  packet7->function = READ_HOLDING_REGISTERS;
  packet7->address = 27;
  packet7->no_of_registers = 2;
  packet7->register_array = fps;
  
  packet8->id = 1;
  packet8->function = READ_HOLDING_REGISTERS;
  packet8->address = 29;
  packet8->no_of_registers = 2;
  packet8->register_array = frek;

  packet9->id = 1;
  packet9->function = READ_HOLDING_REGISTERS;
  packet9->address = 71;
  packet9->no_of_registers = 2;
  packet9->register_array = wpp;
  
  modbus_configure(baud, timeout, polling, retry_count, TxEnablePin, packets, TOTAL_NO_OF_PACKETS);
  timer = millis();
  
  //CONNECT TO WIFI
  WiFi.begin(ssid, pass);
  if(WiFi.status() == WL_CONNECTED){
    Serial.println("Wifi Terkoneksi");
   }else{
    Serial.println("Wifi Tidak Terkoneksi");
   }

   //SETUP SSR
   SolidStateRelay();
   
   //SETUP FIREBASE
   config.database_url = "https://pdam-iot-65ebf-default-rtdb.firebaseio.com/";
   config.signer.tokens.legacy_token = "oIH91rpFqN47QJ4zjapYQlFLwZJOwgPkjAbGwQw8";
   Firebase.reconnectWiFi(true);
   Firebase.begin(&config, &auth);
}

//baca OPTOCOUPLER
void optocoupler(){
  int RUN = digitalRead(L1);
  int STOP = digitalRead(L2);
  int TRIP = digitalRead(L3);
  int PRESS = digitalRead(L4);
  int PLN = digitalRead(L5);
  int WLC = digitalRead(L6);

  if(RUN==0){
    Serial.println("Run");
    SetLED("led1", true, "Run");
    SetLED("led2", false, "Stop");
    SetLED("led3", false, "Trip");
    SetLED("led4", false, "Press");
    SetLED("led5", false, "PLN");
    SetLED("led6", false, "WLC");
  }else if(STOP==0){
    Serial.println("Stop");
    SetLED("led1", false, "Run");
    SetLED("led2", true, "Stop");
    SetLED("led3", false, "Trip");
    SetLED("led4", false, "Press");
    SetLED("led5", false, "PLN");
    SetLED("led6", false, "WLC");
  }else if(TRIP==0){
    Serial.println("Trip");
    SetLED("led1", false, "Run");
    SetLED("led2", false, "Stop");
    SetLED("led3", true, "Trip");
    SetLED("led4", false, "Press");
    SetLED("led5", false, "PLN");
    SetLED("led6", false, "WLC");
  }else if(PRESS==0){
    Serial.println("Press");
    SetLED("led1", false, "Run");
    SetLED("led2", false, "Stop");
    SetLED("led3", false, "Trip");
    SetLED("led4", true, "Press");
    SetLED("led5", false, "PLN");
    SetLED("led6", false, "WLC");
  }else if(PLN==0){
    Serial.println("PLN");
    SetLED("led1", false, "Run");
    SetLED("led2", false, "Stop");
    SetLED("led3", false, "Trip");
    SetLED("led4", false, "Press");
    SetLED("led5", true, "PLN");
    SetLED("led6", false, "WLC");
  }else if(WLC==0){
    Serial.println("WLC");
    SetLED("led1", false, "Run");
    SetLED("led2", false, "Stop");
    SetLED("led3", false, "Trip");
    SetLED("led4", false, "Press");
    SetLED("led5", false, "PLN");
    SetLED("led6", true, "WLC");
  }
}

void SetLED(String nama, bool value, String namaLed) {
  String pathLed = nama + "/value";
  String pathNamaLed = nama + "/nama";
  json.set(pathNamaLed, namaLed);
  json.set(pathLed, value);
}

void SolidStateRelay(){
  if(Firebase.getInt(fbdo, "ewsApp/panel-pompa/panelPompa1/relay1/trigger")){
    relay1 = fbdo.intData();
    Serial.println("Relay1:");
    Serial.println(relay1);
    if(relay1==1) {
      digitalWrite(CH1, LOW);//Lampu SSR mati
      Serial.println("Relay 1 ON");
      }else if(relay1==0){
        digitalWrite(CH1, HIGH);//Lampu SSR nyala
        Serial.println("Relay 1 OFF");
        }
    }else {
    Serial.println("Error relay2");
    Serial.println(fbdo.errorReason());
    }
  if(Firebase.getInt(fbdo, "ewsApp/panel-pompa/panelPompa1/relay2/trigger")){
    relay2 = fbdo.intData();
    Serial.println("Relay2:");
    Serial.println(relay2);
    if(relay2==1) {
      digitalWrite(CH2, LOW);//Lampu SSR mati
      Serial.println("Relay 2 ON");
      }else if(relay2==0){
        digitalWrite(CH2, HIGH);//Lampu SSR nyala
        Serial.println("Relay 2 OFF");
        }
    }else{
    Serial.println("Error relay2");
    Serial.println(fbdo.errorReason());
  }
}

void Powermeter(){
    unsigned int connection_status = modbus_update(packets);
    long newTimer = millis();
    if(newTimer - timer >= 5000){
      Serial.println();
      Serial.print("Volt A: ");   
      Serial.println(f_2uint_float(volta[1],volta[0]));
      json.set("voltR", f_2uint_float(volta[1],volta[0]));
      Serial.print("Volt B: ");
      Serial.println(f_2uint_float(voltb[1],voltb[0]));
      json.set("voltS", f_2uint_float(voltb[1],voltb[0]));
      Serial.print("Volt C: ");
      Serial.println(f_2uint_float(voltc[1],voltc[0]));
      json.set("voltT", f_2uint_float(voltc[1],voltc[0]));
      Serial.print("Arus A: ");
      Serial.println(f_2uint_float(arusa[1],arusa[0]));
      json.set("currentR", f_2uint_float(arusa[1],arusa[0]));
      Serial.print("Arus B: ");
      Serial.println(f_2uint_float(arusb[1],arusb[0]));
      json.set("currentS", f_2uint_float(arusb[1],arusb[0]));
      Serial.print("Arus C: ");
      Serial.println(f_2uint_float(arusc[1],arusc[0]));
      json.set("currentT", f_2uint_float(arusc[1],arusc[0]));
      Serial.print("Power Facktor : ");
      Serial.println(f_2uint_float(fps[1],fps[0]));
      json.set("powerFactor", f_2uint_float(fps[1],fps[0]));
      Serial.print("Frekuensi : ");
      Serial.println(f_2uint_float(frek[1],frek[0]));
      json.set("frequency", f_2uint_float(frek[1],frek[0]));
      Serial.print("WPP : ");
      Serial.println(f_2uint_float(wpp[1],wpp[0]));
      json.set("power", f_2uint_float(wpp[1],wpp[0]));
      timer = newTimer;
      }
}

void loop() {
  String Datastring;
  SolidStateRelay();
  optocoupler();
  Powermeter();
  // String jsonString = json.toString(Datastring,false);
  // json.toString(Serial,false);
  Serial.println(json.toString(Serial,true));
  if(Firebase.updateNodeAsync(fbdo, "ewsApp/panel-pompa/panelPompa1/", json)) {
    Serial.println("Berhasil upload data ke firebase");
  } else {
    Serial.println(fbdo.errorReason());
  }
}
