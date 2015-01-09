/*
                 Arduino Nano V3
                     _______
  RX             D0 -|  N  |- 
  TX             D1 -|  A  |-             GND
  RESET             -|  N  |-             RESET
  GND               -|  O  |-   	  5V
  receive        D2 -|     |- A7
  IR-LED         D3 -|  V  |- A6      
  transmit       D4 -|  3  |- A5          SCL
  DS18B20        D5 -|     |- A4          SDA
                 D6 -|     |- A3      
                 D7 -|     |- A2      
  LED            D8 -|     |- A1      
  DHT11          D9 -|     |- A0      			 
  SS            D10 -|     |-             AREF
  MOSI          D11 -|     |-             3.3V			
  MISO          D12 -|_____|- D13         LED/SCK
  
  
v 0.1    Read temperature from DS18B20 and broadcast with KaKu_dim protocol
v 0.2    Changed protocol KaKu_dim to Probe protocol
v 0.3    Added DHT11 humidity transmit
v 0.4    Changed to pimatic_generic protocol
v 0.4.1  Changed timings: https://github.com/pimatic/pimatic/issues/74#issuecomment-60989449 
v 0.4.2  Changed transmit() function with argument number of repeats
v 0.5    Add NewRemoteSwitch (KaKu) libraries for retransmitting
v 0.6    Changed Attiny drawing to Nano and changed pin's
v 0.7    Added KaKu retransmit as proxy
v 0.8    Added IR retransmit as proxy


 * Generic Sender code : Send a value (counter) over RF 433.92 mhz
 * Fréquence : 433.92 mhz
 * Protocole : homepi 
 * Licence : CC -by -sa
 * Auteur : Yves Grange
 * Version : 0.1
 * Lase update : 10/10/2014
 * 
 * Based on: Valentin CARRUESCO aka idleman and Manuel Esteban aka Yaug (http://manuel-esteban.com) work  
 */

// Includes
#include <OneWire.h> // http://www.pjrc.com/teensy/arduino_libraries/OneWire.zip
#include <DallasTemperature.h> // http://download.milesburton.com/Arduino/MaximTemperature/DallasTemperature_LATEST.zip
#include <dht.h> // http://playground.arduino.cc/Main/DHTLib#.UyMXevldWCQ
#include <NewRemoteReceiver.h>
#include <NewRemoteTransmitter.h>
#include <IRremote.h>

// Define vars
#define DHT11_PIN 9
#define senderPin 4
const int ledPin = 8; // LED PIN
#define ONE_WIRE_BUS 5 // DS18B20 PIN
#define rxPin 2   // RF RECEIVER PIN

long codeKit = 1000;  // Your unique ID for your Arduino node
int Bytes[30]; 
int BytesData[30]; 

// Set to 1 for serial output
boolean debug = 1;

/*
Config sensors
0=disable
1=enable / every loop
n=run everyth n loop
*/
int DS18B20 = 1;
int DHT11 = 1;
int kaku_proxy = 1;
int ir_proxy = 1;
int LoopDelay = 6000;
int SensorDelay = 5000;

int DS18B20_i = DS18B20;
int DHT11_i = DHT11;

unsigned int aan[] = {3420, 1592, 500, 1128, 464, 1160, 460, 372, 464, 372, 412, 424, 456, 1160, 460, 376, 460, 376, 500, 1132, 412, 1212, 456, 376, 456, 1160, 464, 372, 460, 376, 460, 1160, 412, 1208, 504, 348, 456, 1160, 460, 1160, 460, 372, 464, 372, 460, 1160, 464, 372, 460, 376, 496, 1136, 460, 372, 464, 372, 460, 376, 460, 376, 412, 424, 408, 424, 456, 376, 500, 348, 412, 424, 412, 424, 456, 376, 460, 372, 464, 372, 412, 424, 460, 376, 500, 344, 460, 376, 412, 1208, 460, 376, 460, 376, 408, 1212, 408, 424, 412, 424, 496, 1136, 460, 1160, 460, 380, 408, 420, 464, 372, 460, 376, 412, 424, 412, 424, 500, 344, 408, 428, 412, 1208, 412, 1208, 412, 424, 460, 376, 408, 424, 412, 424, 496, 348, 460, 1160, 412, 424, 412, 1208, 412, 1212, 408, 1212, 412, 420, 460, 1160, 500, 348, 408, 428, 408, 424, 412, 424, 456, 376, 460, 376, 408, 428, 408, 424, 504, 340, 412, 424, 412, 424, 460, 376, 412, 424, 408, 428, 408, 424, 456, 376, 500, 348, 412, 424, 456, 376, 408, 428, 408, 424, 412, 424, 412, 424, 412, 424, 496, 348, 412, 424, 412, 424, 408, 428, 456, 372, 460, 376, 412, 424, 412, 424, 500, 344, 408, 1212, 412, 424, 408, 428, 408, 424, 412, 424, 412, 424, 408, 424, 492};
unsigned int uit[] = {3424, 1592, 504, 1128, 460, 1160, 460, 380, 452, 380, 456, 376, 460, 1160, 460, 372, 464, 372, 500, 1132, 464, 1160, 460, 372, 460, 1160, 460, 376, 460, 376, 460, 1160, 460, 1160, 500, 348, 460, 1160, 460, 1160, 460, 376, 484, 348, 460, 1160, 460, 376, 460, 376, 500, 1128, 460, 376, 460, 372, 460, 376, 460, 376, 460, 372, 464, 372, 460, 376, 500, 348, 456, 380, 456, 376, 456, 380, 456, 376, 460, 372, 464, 372, 460, 376, 500, 348, 456, 380, 456, 376, 456, 380, 456, 380, 456, 1164, 408, 424, 460, 376, 496, 1136, 456, 1160, 460, 376, 460, 376, 460, 372, 464, 372, 460, 376, 460, 376, 500, 348, 456, 376, 460, 1156, 464, 1160, 460, 376, 456, 380, 456, 380, 456, 376, 496, 348, 460, 1160, 460, 376, 460, 1160, 460, 1160, 460, 1160, 460, 376, 460, 1160, 500, 348, 408, 428, 408, 424, 456, 380, 456, 380, 456, 376, 456, 380, 456, 380, 496, 348, 460, 372, 464, 372, 412, 424, 412, 424, 408, 424, 456, 380, 456, 380, 496, 348, 460, 372, 464, 372, 460, 376, 460, 376, 408, 424, 412, 424, 412, 424, 496, 348, 412, 424, 460, 372, 460, 376, 460, 376, 460, 372, 412, 424, 412, 424, 500, 344, 412, 1208, 460, 1160, 460, 1160, 460, 1160, 460, 1164, 412, 1208, 412, 1208, 492};

// Start includes
//OneWire oneWire(ONE_WIRE_BUS); // Setup a oneWire instance
//DallasTemperature sensors(&oneWire); // Pass our oneWire reference to Dallas Temperature  
//dht DHT;    
IRsend irsend;

void setup()
{
  if (debug) {
    Serial.begin(115200);
    Serial.print("Configured values:\n");
    Serial.print("DS18B20=");
    Serial.print(DS18B20);
    Serial.print("\n");
    Serial.print("DHT11=");
    Serial.print(DHT11);
    Serial.print("\n");    
    Serial.print("kaku_proxy=");
    Serial.print(kaku_proxy);
    Serial.print("\n");        
    Serial.print("ir_proxy=");
    Serial.print(ir_proxy);
    Serial.print("\n");        
    Serial.print("LoopDelay=");
    Serial.print(LoopDelay);
    Serial.print("\n");    
    Serial.print("SensorDelay=");
    Serial.print(SensorDelay);
    Serial.print("\n");        
    Serial.print("\n");        
  }
  pinMode(senderPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  // buildSignal();

  if (DS18B20 != 0) {
     //start up temp sensor
//    sensors.begin();
  }
    
  if (kaku_proxy != 0) {
    // Initialize receiver on interrupt 0 (= digital pin 2), calls the callback "retransmit"
    NewRemoteReceiver::init(0, 2, retransmit);
  } 
}

void loop()
{
    if (debug) {
      Serial.print("Start loop\n");
    }  
  
  if (DS18B20 == DS18B20_i) {
    if (debug) {
      Serial.print("Read & Send DS18B20\n");
      irsend.sendRaw(aan, 227, 38);         
   delay(1000); 
    }  
    // Read DS18B20 and transmit value as sensor 1
/*
    float temperature;
    sensors.requestTemperatures(); // Get the temperature
    temperature = sensors.getTempCByIndex(0); // Get temperature in Celcius
    unsigned long CounterValue = temperature * 10;
    Blink(ledPin,2);  
    int BytesType[] = {0,0,0,1};
    transmit(true, CounterValue, BytesType, 6);
    delay(SensorDelay);
*/
    DS18B20_i = 0;
  }

  if (DHT11 == DHT11_i) {
    if (debug) {
      Serial.print("Read & Send DHT11\n");
      irsend.sendRaw(uit, 227, 38);            
    }  
    
    // Read DHT11 and transmit value as sensor 2
    /*
    int chk = DHT.read11(DHT11_PIN);
    switch (chk)
    {
      case DHTLIB_OK:
      float humfloat = DHT.humidity;
      int CounterValue = humfloat * 10;
      Blink(ledPin,2); 
      int BytesType[] = {0,0,1,0};
      transmit(true, CounterValue, BytesType, 6);
      break;
    }
    */
    DHT11_i = 0;    
  }

  DS18B20_i ++; 
  DHT11_i ++;   

  if (debug) {
    Serial.print("End of loop, sleep . . .\n\n");
  }  
  delay(LoopDelay);
}
void itob(unsigned long integer, int length)
{  
 for (int i=0; i<length; i++){
   if ((integer / power2(length-1-i))==1){
     integer-=power2(length-1-i);
     Bytes[i]=1;
   }
   else Bytes[i]=0;
 }
}

// Callback function is called only when a valid code is received.
void retransmit(NewRemoteCode receivedCode) {

        Serial.print("ik ontvang iets! \n");

  /*
  receivedCode.address      -> ID
  receivedCode.unit         -> unit
  receivedCode.period       -> µs (microseconds)
  receivedCode.switchType   -> on/off/dimlevel
  receivedCode.groupBit     -> group command
  */

  // Disable the receiver; otherwise it might pick up the retransmit as well.
  NewRemoteReceiver::disable();

  // Need interrupts for delay()
//  interrupts();

  // Wait 1 second before sending.
  delay(1000);

  // retransmit KaKu doorcontact 13040182-9 (tuindeur) as 101010-0
  if (receivedCode.address == 13040182 && receivedCode.unit == 9) {  
      Serial.print("retransmit KaKu doorcontact 13040182-9 (tuindeur) as 101010-0");
      Serial.print(receivedCode.switchType);
      Serial.print("\n");
      NewRemoteTransmitter transmitter(101010, 4, receivedCode.period);    
      transmitter.sendUnit(0, receivedCode.switchType);
  }
  
  // retransmit KaKu sender 12691134-1 (bijkeuken verlichting sender) as 101010-1
  if (receivedCode.address == 12691134 && receivedCode.unit == 1) {  
      Serial.print("retransmit KaKu sender 12691134-1 (bijkeuken verlichting sender) as 101010-1 \n");
      Serial.print(receivedCode.switchType);
      Serial.print("\n");      
      NewRemoteTransmitter transmitter(101010, 4, receivedCode.period);    
      transmitter.sendUnit(1, receivedCode.switchType);
  }  

  // retransmit KaKu sender 11221182-9 (Elise verlichting) as 101010-2
  if (receivedCode.address == 11221182 && receivedCode.unit == 9) {  
      Serial.print("retransmit KaKu sender 11221182-9 (Elise verlichting) as 101010-2 \n");
      Serial.print(receivedCode.switchType);
      Serial.print("\n");      
      NewRemoteTransmitter transmitter(101010, 4, receivedCode.period);    
      transmitter.sendUnit(2, receivedCode.switchType);
  }  
  
  // retransmit KaKu receiver 8934706-6 (bijkeuken verlichting receiver) as 101010-3
  if (receivedCode.address == 8934706 && receivedCode.unit == 6) {  
      Serial.print("retransmit KaKu receiver 8934706-6 (bijkeuken verlichting receiver) as 101010-3 \n");
      Serial.print(receivedCode.switchType);
      Serial.print("\n");
      NewRemoteTransmitter transmitter(101010, 4, receivedCode.period);    
      transmitter.sendUnit(3, receivedCode.switchType);
  }  
  
  // retransmit KaKu doorcontact 12991098-9 (testdeur) as 101010-4
  if (receivedCode.address == 12991098 && receivedCode.unit == 9) {  
      Serial.print("retransmit KaKu doorcontact 12991098-9 (testdeur) as 101010-4");
      Serial.print(receivedCode.switchType);
      Serial.print("\n");
      NewRemoteTransmitter transmitter(101010, 4, receivedCode.period);    
      transmitter.sendUnit(4, receivedCode.switchType);
  }


  if (receivedCode.address == 8934706 && receivedCode.unit == 9 && receivedCode.switchType == 0) {  
      Serial.print("receive 8934706-9 OFF \n");
  }

  if (receivedCode.address == 8934706 && receivedCode.unit == 9 && receivedCode.switchType == 1) {  
      Serial.print("receive 8934706-9 ON \n");
  }


  // Enable the receiver.
  NewRemoteReceiver::enable();
}

void itobCounter(unsigned long integer, int length)
{  
 for (int i=0; i<length; i++){
   if ((integer / power2(length-1-i))==1){
     integer-=power2(length-1-i);
     BytesData[i]=1;
   }
   else BytesData[i]=0;
 }
}

unsigned long power2(int power){    //gives 2 to the (power)
 unsigned long integer=1;          
 for (int i=0; i<power; i++){      
   integer*=2;
 }
 return integer;
}

/**
 * Crée notre signal sous forme binaire
**/
/*
void buildSignal()
{
  Serial.print(codeKit);
  // Converti les codes respectifs pour le signal en binaire
  itob(codeKit, 14);
  for(int j=0;j < 14; j++){
   Serial.print(Bytes[j]);
  }
  Serial.print();
}

*/

// Convert 0 in 01 and 1 in 10 (Manchester conversion)
void sendPair(bool b) {
 if(b)
 {
   sendBit(true);
   sendBit(false);
 }
 else
 {
   sendBit(false);
   sendBit(true);
 }
}

//Envois d'une pulsation (passage de l'etat haut a l'etat bas)
//1 = 310µs haut puis 1340µs bas
//0 = 310µs haut puis 310µs bas
void sendBit(bool b) {
 if (b) {
   digitalWrite(senderPin, HIGH);
   delayMicroseconds(650);   //506 orinally, but tweaked.
   digitalWrite(senderPin, LOW);
   delayMicroseconds(2024);  //1225 orinally, but tweaked.
 }
 else {
   digitalWrite(senderPin, HIGH); 
   delayMicroseconds(650);   //506 orinally, but tweaked.
   digitalWrite(senderPin, LOW);
   delayMicroseconds(4301);   //305 orinally, but tweaked.
 }
}

/** 
 * Transmit data
 * @param boolean  positive : if the value you send is a positive or negative one
 * @param long Counter : the value you want to send
 **/
void transmit(boolean positive, unsigned long Counter, int BytesType[], int repeats)
{
 int ii;
 for(ii=0; ii<repeats;ii++)
 {
  int i;
  itobCounter(Counter, 30);

  // Send the unique ID of your Arduino node
  for(i=0; i<14;i++)
 {
  sendPair(Bytes[i]);
 }

  // Send protocol type
 for(int j = 0; j<4; j++)
 {
  sendPair(BytesType[j]);
 }

 // Send the flag to mark the value as positive or negative
 sendPair(positive);

 // Send value (ie your counter)
 for(int j = 0; j<30; j++)
 {
   sendPair(BytesData[j]);
 }

 // Send the flag "End of the transmission"
 digitalWrite(senderPin, HIGH);
 delayMicroseconds(650);     
 digitalWrite(senderPin, LOW);
 delayMicroseconds(8602);
 }
}

void Blink(int led, int times)
{
 for (int i=0; i< times; i++)
 {
  digitalWrite(ledPin,HIGH);
  delay (50);
  digitalWrite(ledPin,LOW);
  delay (50);
 }
}




