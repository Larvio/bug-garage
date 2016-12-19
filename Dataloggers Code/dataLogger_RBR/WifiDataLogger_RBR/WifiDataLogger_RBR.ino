//#include <SPI.h> //general SPI (SD card, radio module)
//#include <RFM69.h> //radio library
#include <OneWire.h> //temperature sensor
#include <DallasTemperature.h> //temperature sensor  
#include <DHT.h> //Needed for Sensors
#include <WiFi101.h>
#include <UbidotsArduino.h>

#define RELAY1 A0
#define RELAY2 A1
#define RELAY3 A2
#define RELAY4 A3
#define AIRDHT 9     
#define ONE_WIRE_BUS 10
#define LED    13  // onboard blinky

#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
#define TEMPCEILING 80
#define TEMPFLOOR   70
//#define DELAY_MS 60000
#define SERIAL_BAUD 9600

//********DATA TRANSMISSION KEYS***********************************************************
#define NETWORK "theham"
#define PASSWORD "EGBDF901"
#define UBIDOTSURL "things.ubidots.com/api/v1.6/collections/values/?token=3hB2gtSrMKDaZKyPkRt6Egqh7Lft8c"
#define TOKEN "3hB2gtSrMKDaZKyPkRt6Egqh7Lft8c"
#define ID1 "58555b4f7625425970ee3a78" //bin temperature
#define ID2 "58555b577625425972dec78a" //ambient temperature
#define ID3 "58555b5e762542596efaa42d" //ambient humidity

//********GLOBAL VARIABLES******************************************************************
double airHumidity;
double airTemp;
double binTemp;
boolean heatOn = false;

//********GLOBAL OBJECTS*******************************************************************

DHT airSensor(AIRDHT, DHTTYPE);
OneWire oneWire(ONE_WIRE_BUS); //setup a oneWire node to communicate with 1 or more devices
DallasTemperature multisensors(&oneWire); //setup temperature sensors (respond in celcius)


char ssid[] = NETWORK; //  your network SSID (name)
char pass[] = PASSWORD;    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;            // your network key Index number (needed only for WEP)

int status = WL_IDLE_STATUS;
char server[] = UBIDOTSURL;
WiFiClient client; // initialize the Ethernet client library (port 80 is default for HTTP):


void setup() {
  Serial.begin(SERIAL_BAUD);
 
  //DIGITAL PINS
  pinMode(RELAY1, OUTPUT);      // sets the digital pin as output
  pinMode(RELAY2, OUTPUT);      // sets the digital pin as output
  pinMode(RELAY3, OUTPUT);      // sets the digital pin as output
  pinMode(RELAY4, OUTPUT);      // sets the digital pin as output
  pinMode(LED, OUTPUT); //Initialize LED

  //INITIALIZERS
  multisensors.begin(); // Initialize temperature sensor
  airSensor.begin();
  WiFi.setPins(8,7,4,2); //Configure pins for Adafruit ATWINC1500 Feather
  
}

void loop() {
  //read sensors
  binTemp = readtemp();
  airHumidity = airSensor.readHumidity();
  airTemp = airSensor.readTemperature(true);
  Serial.print("bin "); Serial.print(binTemp);
  Serial.print(" air ");  Serial.print(airTemp);  Serial.print("F ");  Serial.println(airHumidity);

  //send data
  wifiConnect();
  Serial.println("Sending using Ubidots");
  sendUbidots();

  //control
  if (airTemp<TEMPFLOOR)        relayControl_NC(1, true);
  else if (airTemp>TEMPCEILING) relayControl_NC(1, false);

  //delay interval
  delay(60000);  // Wait 1 second between transmits, could also 'sleep' here!
}

//------FUNCTION: READ TEMPERATURE--------------------
double readtemp(){
  multisensors.requestTemperatures(); // Send the command to get temperatures from all devices on the bus
  double t = multisensors.getTempFByIndex(0);
  return t;
  }

//------FUNCTION: SET RELAY STATE--------------------
void relayControl_NC(int relaynum, bool state){
  int relaypin;
  switch (relaynum) {
  case 1:
    relaypin=RELAY1;
    break;
  case 2:
    relaypin=RELAY2;
    break;
  case 3:
    relaypin=RELAY3;
    break;
  case 4:
    relaypin=RELAY4;
  default:
    break;
  }
  if (state) {
    digitalWrite(relaypin,LOW);
    digitalWrite(LED,HIGH);
    } else {
      digitalWrite(relaypin,HIGH);
      digitalWrite(LED,LOW);
    }
}

//------FUNCTION: CONNECT TO WIFI--------------------
void wifiConnect(){
  if (WiFi.status() == WL_NO_SHIELD) { // check for the presence of the module:
    Serial.println("WiFi module not detected");
  }
  
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: "); Serial.println(ssid);
      
    status = WiFi.begin(ssid, pass); // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    delay(10000); // wait 10 seconds for connection:
  }
  Serial.println("Connected to wifi");
}

//-------FUNCTION: SEND DATA USING UBIDOTS LIBRARY--------
void sendUbidots(){
  Serial.println("Building data structure");
  Ubidots client(TOKEN);
  client.add(ID1,binTemp);
  client.add(ID2,airTemp);
  client.add(ID3,airHumidity);
  client.sendAll();
}

void Blink(byte PIN, byte DELAY_MS, byte loops){
  for (byte i=0; i<loops; i++)  {
    digitalWrite(PIN,HIGH);
    delay(DELAY_MS);
    digitalWrite(PIN,LOW);
    delay(DELAY_MS);
  }
}

