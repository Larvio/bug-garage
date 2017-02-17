/* RFM69 library and code by Felix Rusu - felix@lowpowerlab.com
// Get libraries at: https://github.com/LowPowerLab/
// Make sure you adjust the settings in the configuration section below !!!
// **********************************************************************************
// Copyright Felix Rusu, LowPowerLab.com
// Library and code by Felix Rusu - felix@lowpowerlab.com
// **********************************************************************************
// License
// **********************************************************************************
// This program is free software; you can redistribute it 
// and/or modify it under the terms of the GNU General    
// Public License as published by the Free Software       
// Foundation; either version 3 of the License, or        
// (at your option) any later version.                    
//                                                        
// This program is distributed in the hope that it will   
// be useful, but WITHOUT ANY WARRANTY; without even the  
// implied warranty of MERCHANTABILITY or FITNESS FOR A   
// PARTICULAR PURPOSE. See the GNU General Public        
// License for more details.                              
//                                                        
// You should have received a copy of the GNU General    
// Public License along with this program.
// If not, see <http://www.gnu.org/licenses></http:>.
//                                                        
// Licence can be viewed at                               
// http://www.gnu.org/licenses/gpl-3.0.txt
//
// Please maintain this license information along with authorship
// and copyright notices in any redistribution of this code
// **********************************************************************************/

#include <SPI.h> //general SPI (SD card, radio module)
#include <RFM69.h> //radio library
#include <OneWire.h> //temperature sensor
#include <DallasTemperature.h> //temperature sensor  
#include <DHT.h> //Needed for Sensors

#define AIRDHT 11     
#define RELAY1 A1
#define RELAY2 A2
#define RELAY3 A3
#define RELAY4 A4
#define ONE_WIRE_BUS 10

#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
#define heatingTempHighThreshold 87
#define heatingTempLowThreshold 82

//*********************************************************************************************
// *********** IMPORTANT SETTINGS - YOU MUST CHANGE/ONFIGURE TO FIT YOUR HARDWARE *************
//*********************************************************************************************
#define NETWORKID     100  // The same on all nodes that talk to each other
#define NODEID        2    // The unique identifier of this node
#define RECEIVER      1    // The recipient of packets

//Match frequency to the hardware version of the radio on your Feather
//#define FREQUENCY     RF69_433MHZ
//#define FREQUENCY     RF69_868MHZ
#define FREQUENCY     RF69_915MHZ
#define ENCRYPTKEY    "io8FRlnmV81RA0n2" //exactly the same 16 characters/bytes on all nodes!
#define IS_RFM69HCW   true // set to 'true' if you are using an RFM69HCW module

//*********************************************************************************************
#define SERIAL_BAUD   115200

// for Feather 32u4 Radio
#define RFM69_CS      8
#define RFM69_IRQ     7
#define RFM69_IRQN    4  // Pin 7 is IRQ 4!
#define RFM69_RST     4

#define LED           13  // onboard blinky

//********GLOBAL VARIABLES******************************************************************
int16_t packetnum = 0;  // packet counter, we increment per xmission

double airHumidity;
double airTemp;
double binTemp;
boolean heatOn = false;

//********GLOBAL OBJECTS*******************************************************************

RFM69 radio = RFM69(RFM69_CS, RFM69_IRQ, IS_RFM69HCW, RFM69_IRQN);
DHT airSensor(AIRDHT, DHTTYPE);
OneWire oneWire(ONE_WIRE_BUS); //setup a oneWire node to communicate with 1 or more devices
DallasTemperature multisensors(&oneWire); //setup temperature sensors (respond in celcius)

void setup() {
  while (!Serial); // wait until serial console is open, remove if not tethered to computer. Delete this line on ESP8266
  Serial.begin(SERIAL_BAUD);
  Serial.println("Feather RFM69HCW Transmitter");
  
  RFMhardreset();

  //DIGITAL PINS
  pinMode(RELAY1, OUTPUT);      // sets the digital pin as output
  pinMode(RELAY2, OUTPUT);      // sets the digital pin as output
  pinMode(RELAY3, OUTPUT);      // sets the digital pin as output
  pinMode(RELAY4, OUTPUT);      // sets the digital pin as output
  pinMode(LED, OUTPUT); //Initialize LED

  //INITIALIZERS
  multisensors.begin(); // Initialize temperature sensor
  airSensor.begin();
  radio.initialize(FREQUENCY,NODEID,NETWORKID); // Initialize radio

  //MORE RADIO INITIALIZING STUFF
  if (IS_RFM69HCW) {
    radio.setHighPower();    // Only for RFM69HCW & HW!
  }
  radio.setPowerLevel(31); // power output ranges from 0 (5dBm) to 31 (20dBm)
  radio.encrypt(ENCRYPTKEY);

  Serial.print("\nTransmitting at ");
  Serial.print(FREQUENCY==RF69_433MHZ ? 433 : FREQUENCY==RF69_868MHZ ? 868 : 915);
  Serial.println(" MHz");
}


void loop() {
  delay(1000);  // Wait 1 second between transmits, could also 'sleep' here!

  //read sensors
  binTemp = readtemp();
  airHumidity = airSensor.readHumidity();
  airTemp = airSensor.readTemperature(true);
  Serial.print("bin ");
  Serial.print(binTemp);
  Serial.print(" air ");
  Serial.print(airTemp);
  Serial.print("F ");
  Serial.println(airHumidity);
  
  //parse env data
  char radiopacket[10]; //radiopacket length should be 
  dtostrf(binTemp,6,2,radiopacket);
  Serial.println("bin " + binTemp + " air " + airTemp);
   
  //char radiopacket[20] = "Hello World #";
  //itoa(packetnum++, radiopacket+13, 10);
  Serial.print("Sending "); 
  Serial.println(radiopacket);
    
  if (radio.sendWithRetry(RECEIVER, radiopacket, strlen(radiopacket))) { 
    //target node Id, message as string or byte array, message length
    Serial.println("OK");
    Blink(LED, 50, 3); //blink LED 3 times, 50ms between blinks
  }

  radio.receiveDone(); //put radio in RX mode
  Serial.flush(); //make sure all serial data is clocked out before sleeping the MCU
}

void Blink(byte PIN, byte DELAY_MS, byte loops)
{
  for (byte i=0; i<loops; i++)
  {
    digitalWrite(PIN,HIGH);
    delay(DELAY_MS);
    digitalWrite(PIN,LOW);
    delay(DELAY_MS);
  }
}

/*  readtemp()
 *  read temperatures on the onewire bus and return the value of the sensor at index 0
 */
double readtemp(){
  multisensors.requestTemperatures(); // Send the command to get temperatures from all devices on the bus
  double t = multisensors.getTempFByIndex(0);
  return t;
  }
  
/*
 *
 */
void RFMhardreset(){
  // Hard Reset the RFM module
  pinMode(RFM69_RST, OUTPUT);
  digitalWrite(RFM69_RST, HIGH);
  delay(100);
  digitalWrite(RFM69_RST, LOW);
  delay(100);
  }

