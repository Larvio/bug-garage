#include <Wire.h> //Needed for RTC
#include <RTClib.h> //Needed for RTC
#include <SD.h> //Needed for SD Card
#include <DHT.h> //Needed for Sensors

#define DHTPIN_4 4     // what digital pin the sensor is connected to (duplicate as necessary)
#define DHTPIN_5 5     // what digital pin the sensor is connected to (duplicate as necessary)
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
DHT dht_4(DHTPIN_4, DHTTYPE);
DHT dht_5(DHTPIN_5, DHTTYPE);

int relayPin1 = 6;                 // IN1 connected to digital pin 7
int relayPin2 = 7;                 // IN2 connected to digital pin 8
int relayPin3 = 8;
int relayPin4 = 9;
//int relayPin5 = 11;
//int relayPin6 = 12;
//int relayPin7 = 13;

int coolingTempHighThreshold = 95; //95
int coolingTempLowThreshold = 90; //85

int heatingTempHighThreshold = 87; //87.5
int heatingTempLowThreshold = 82;

int humidityHighThreshold = 80;
int humidityLowThreshold = 75;

boolean exhaustOn = false;
boolean humidifierOn=false;
boolean heatOn = false;

//#define LOG_INTERVAL 5000 //time between measurements in millis
#define LOG_INTERVAL 20000 //time between measurements in millis

// how many milliseconds before writing the logged data permanently to disk
// set it to the LOG_INTERVAL to write each time (safest)
// set it to 10*LOG_INTERVAL to write all data every 10 datareads, you could lose up to 
// the last 10 reads if power is lost but it uses less power and is much faster!
#define SYNC_INTERVAL 300000 // mills between calls to flush() - to write data to the card
uint32_t syncTime = 0; // time of last sync()

//not sure if i need these
#define ECHO_TO_SERIAL 0 // echo data to serial port
#define WAIT_TO_START 0 // Wait for serial input in setup()

// the digital pins that connect to the LEDs
#define greenLEDpin 2
#define redLEDpin 3

RTC_DS1307 RTC; // define the Real Time Clock object

// for the data logging shield, we use digital pin 10 for the SD cs line
const int chipSelect = 10;

// the logging file
File logfile;

void error(char *str)
{
  #if ECHO_TO_SERIAL
    Serial.print("error: ");
    Serial.println(str);
#endif

  while(1){
    // red LED indicates error
  digitalWrite(redLEDpin, HIGH);
  delay(100);
  digitalWrite(redLEDpin, LOW);
  delay(100);
  digitalWrite(redLEDpin, HIGH);
  delay(100);
  digitalWrite(redLEDpin, LOW);
  delay(100);
  digitalWrite(redLEDpin, HIGH);
  delay(100);
  digitalWrite(redLEDpin, LOW);
  delay(LOG_INTERVAL-500);
  }
}

void setup() {
  Serial.begin(9600);

  // use debugging LEDs
  pinMode(redLEDpin, OUTPUT);
  pinMode(greenLEDpin, OUTPUT);

  pinMode(relayPin1, OUTPUT);      // sets the digital pin as output
  pinMode(relayPin2, OUTPUT);      // sets the digital pin as output
  pinMode(relayPin3, OUTPUT);      // sets the digital pin as output
  pinMode(relayPin4, OUTPUT);      // sets the digital pin as output
//  pinMode(relayPin5, OUTPUT);      // sets the digital pin as output
//  pinMode(relayPin6, OUTPUT);      // sets the digital pin as output
//  pinMode(relayPin7, OUTPUT);      // sets the digital pin as output

  digitalWrite(relayPin1, HIGH);        // Prevents relays from starting up engaged
  digitalWrite(relayPin2, HIGH);        // Prevents relays from starting up engaged
  digitalWrite(relayPin3, HIGH);        // Prevents relays from starting up engaged
  digitalWrite(relayPin4, HIGH);        // Prevents relays from starting up engaged
//  digitalWrite(relayPin5, HIGH);        // Prevents relays from starting up engaged
//  digitalWrite(relayPin6, HIGH);        // Prevents relays from starting up engaged
//  digitalWrite(relayPin7, HIGH);        // Prevents relays from starting up engaged

  #if WAIT_TO_START
    #if ECHO_TO_SERIAL
      Serial.println("Type any character to start");
    #endif
    while (!Serial.available());
  #endif //WAIT_TO_START
  
  // initialize the Sensor
  #if ECHO_TO_SERIAL
    Serial.println("Initializing Sensor...");
  #endif
  dht_4.begin(); //initialize the sensor
  dht_5.begin();

  // initialize the SD card
  #if ECHO_TO_SERIAL
    Serial.println("Initializing SD card...");
  #endif
  // make sure that the default chip select pin is set to output, even if you don't use it:
  pinMode(10, OUTPUT);

  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    error("Card failed, or not present");
  }
  #if ECHO_TO_SERIAL
    Serial.println("card initialized.");
  #endif

  // create a new file
  char filename[] = "LOGGER00.CSV";
  for (uint8_t i = 0; i < 100; i++) {
    filename[6] = i/10 + '0';
    filename[7] = i%10 + '0';
    if (! SD.exists(filename)) {
      // only open a new file if it doesn't exist
      logfile = SD.open(filename, FILE_WRITE); 
      break;  // leave the loop!
    }
  }
  
  if (! logfile) {
    error("couldnt create file");
  }

  #if ECHO_TO_SERIAL
    Serial.print("Logging to: ");
    Serial.println(filename);
  #endif

  // connect to RTC
  Wire.begin();  
  if (!RTC.begin()) {
    logfile.println("RTC failed");
    #if ECHO_TO_SERIAL
      Serial.println("RTC failed");
    #endif 
  }

  logfile.println("Time,Time,Time,Sensor_4,Sensor_4,Sensor_4,Sensor_5,Sensor_5,Sensor_5,Relay,Relay,Relay,Relay,Relay,Relay,Relay,Relay,Relay,Relay");
  logfile.println("millis,stamp,datetime,tempF,tempC,Humidity,tempF,tempC,Humidity,exhaustOn,humidifierOn,heatOn,Relay1 (AC1),Relay2 (AC2),Relay3(Heat),Relay4 (Humidifiers),Relay5 (Exhaust),Relay6,Relay7");    
  #if ECHO_TO_SERIAL
    Serial.println("Time,Time,Time,Sensor_4,Sensor_4,Sensor_4,Sensor_5,Sensor_5,Sensor_5,Relay,Relay,Relay,Relay,Relay,Relay,Relay,Relay,Relay,Relay");
    Serial.println("millis,stamp,datetime,tempF,tempC,Humidity,tempF,tempC,Humidity,exhaustOn,humidifierOn,heatOn,Relay1 (AC1),Relay2 (AC2),Relay3(Heat),Relay4 (Humidifiers),Relay5 (Exhaust),Relay6,Relay7"); 
  #endif //ECHO_TO_SERIAL
}

void loop() {
  DateTime now;

  // delay for the amount of time we want between readings
  delay((LOG_INTERVAL -1) - (millis() % LOG_INTERVAL));

  digitalWrite(greenLEDpin, HIGH);

  // log milliseconds since starting
  uint32_t m = millis();
  logfile.print(m);           // milliseconds since start
  logfile.print(", ");    
  #if ECHO_TO_SERIAL
    Serial.print(m);         // milliseconds since start
    Serial.print(", ");  
  #endif

  // fetch the time
  now = RTC.now();

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h_4 = dht_4.readHumidity();
  // Read temperature as Celsius (the default)
  float cTemp_4 = dht_4.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float fTemp_4 = dht_4.readTemperature(true);
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h_5 = dht_5.readHumidity();
  // Read temperature as Celsius (the default)
  float cTemp_5 = dht_5.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float fTemp_5 = dht_5.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if ((isnan(h_4) || isnan(cTemp_4) || isnan(fTemp_4)) && (isnan(h_5) || isnan(cTemp_5) || isnan(fTemp_5))) {
    #if ECHO_TO_SERIAL
      Serial.println("Failed to read from DHT sensor!");
      #endif
    logfile.println("Failed to read from DHT sensor!");
    return;
  }

    if(heatOn==false && fTemp_5<heatingTempLowThreshold){
     digitalWrite(relayPin3, LOW);   // energizes the relay and lights the LED
     heatOn=true;
  } else if(heatOn==true && fTemp_5>heatingTempHighThreshold){
    digitalWrite(relayPin3, HIGH);   // de-energizes the relay and turns off the LED
    heatOn=false;
  }

  if(humidifierOn==false && h_5<humidityLowThreshold){
     digitalWrite(relayPin4, LOW);   // energizes the relay and lights the LED
     humidifierOn=true;
  } else if(humidifierOn==true && h_5>humidityHighThreshold){
    digitalWrite(relayPin4, HIGH);   // de-energizes the relay and turns off the LED
     humidifierOn=false;
  }

    if(exhaustOn==false && fTemp_5>coolingTempHighThreshold){
//     digitalWrite(relayPin5, LOW);   // energizes the relay and lights the LED
     digitalWrite(relayPin1, LOW);   // energizes the relay and lights the LED
     digitalWrite(relayPin2, LOW);   // energizes the relay and lights the LED
     exhaustOn=true;
  } else if(exhaustOn==true && fTemp_5<coolingTempLowThreshold){
//    digitalWrite(relayPin5, HIGH);   // de-energizes the relay and turns off the LED
    digitalWrite(relayPin1, HIGH);   // de-energizes the relay and turns off the LED
    digitalWrite(relayPin2, HIGH);   // de-energizes the relay and turns off the LED
     exhaustOn=false;
  }
  
  // log time
  logfile.print(now.unixtime()); // seconds since 1/1/1970
  logfile.print(",");
  logfile.print('"');
  logfile.print(now.year(), DEC);
  logfile.print("/");
  logfile.print(now.month(), DEC);
  logfile.print("/");
  logfile.print(now.day(), DEC);
  logfile.print(" ");
  logfile.print(now.hour(), DEC);
  logfile.print(":");
  logfile.print(now.minute(), DEC);
  logfile.print(":");
  logfile.print(now.second(), DEC);
  logfile.print('"');
  logfile.print(",");   
  logfile.print(fTemp_4);
  logfile.print(","); 
  logfile.print(cTemp_4);
  logfile.print(","); 
  logfile.print(h_4);
  logfile.print(",");   
  logfile.print(fTemp_5);
  logfile.print(","); 
  logfile.print(cTemp_5);
  logfile.print(","); 
  logfile.print(h_5);
  logfile.print(","); 
  logfile.print(exhaustOn);
  logfile.print(","); 
  logfile.print(humidifierOn);
  logfile.print(","); 
  logfile.print(heatOn);
  logfile.print(", "); 
  logfile.print(digitalRead(relayPin1));
  logfile.print(", "); 
  logfile.print(digitalRead(relayPin2));
  logfile.print(", "); 
  logfile.print(digitalRead(relayPin3));
  logfile.print(", "); 
  logfile.print(digitalRead(relayPin4));
//  logfile.print(", "); 
//  logfile.print(digitalRead(relayPin5));
//  logfile.print(", "); 
//  logfile.print(digitalRead(relayPin6));
//  logfile.print(", "); 
//  logfile.print(digitalRead(relayPin7));
  logfile.println();

  #if ECHO_TO_SERIAL
    Serial.print(now.unixtime()); // seconds since 1/1/1970
    Serial.print(", ");
    Serial.print('"');
    Serial.print(now.year(), DEC);
    Serial.print("/");
    Serial.print(now.month(), DEC);
    Serial.print("/");
    Serial.print(now.day(), DEC);
    Serial.print(" ");
    Serial.print(now.hour(), DEC);
    Serial.print(":");
    Serial.print(now.minute(), DEC);
    Serial.print(":");
    Serial.print(now.second(), DEC);
    Serial.print('"');
    Serial.print(", ");   
    Serial.print(fTemp_4);
    Serial.print(", "); 
    Serial.print(cTemp_4);
    Serial.print(", "); 
    Serial.print(h_4);
    Serial.print(",");
    Serial.print(", ");   
    Serial.print(fTemp_5);
    Serial.print(", "); 
    Serial.print(cTemp_5);
    Serial.print(", "); 
    Serial.print(h_5);
    Serial.print(", "); 
    Serial.print(exhaustOn);
    Serial.print(", "); 
    Serial.print(humidifierOn);
    Serial.print(", "); 
    Serial.print(heatOn);
    Serial.print(", "); 
    Serial.print(digitalRead(relayPin1));
    Serial.print(", "); 
    Serial.print(digitalRead(relayPin2));
    Serial.print(", "); 
    Serial.print(digitalRead(relayPin3));
    Serial.print(", "); 
    Serial.print(digitalRead(relayPin4));
//    Serial.print(", "); 
//    Serial.print(digitalRead(relayPin5));
//    Serial.print(", "); 
//    Serial.print(digitalRead(relayPin6));
//    Serial.print(", "); 
//    Serial.print(digitalRead(relayPin7));  
    Serial.println();
  #endif //ECHO_TO_SERIAL

  digitalWrite(greenLEDpin, LOW);

//  Serial.print((m - syncTime+5));
//  Serial.print("\t");
//  Serial.print(SYNC_INTERVAL);
//  Serial.print("\t");
//  Serial.println((m - syncTime+5) < SYNC_INTERVAL);

  // Now we write data to disk! Don't sync too often - requires 2048 bytes of I/O to SD card
  // which uses a bunch of power and takes time
  if ((m - syncTime+5) < SYNC_INTERVAL) return;
  syncTime = m;
  Serial.println("writing to SD card");

  // blink LED to show we are syncing data to the card & updating FAT!
  digitalWrite(redLEDpin, HIGH);
  logfile.flush();
  delay(100);
  digitalWrite(redLEDpin, LOW);

//relayTest();
}

void relayTest() {
  // delay for the amount of time we want between readings
//  delay(2000);
//  digitalWrite(relayPin7, HIGH);
  digitalWrite(relayPin1, LOW);   // energizes the relay and lights the LED
  digitalWrite(relayPin2, LOW);   // energizes the relay and lights the LED

  delay(5000);
  digitalWrite(relayPin1, HIGH); 
  digitalWrite(relayPin2, HIGH);   // energizes the relay and lights the LED

  delay(5000);
  digitalWrite(relayPin2, HIGH);
  digitalWrite(relayPin3, LOW);   // energizes the relay and lights the LED
    delay(5000);
  digitalWrite(relayPin3, HIGH); 
  digitalWrite(relayPin4, LOW);   // energizes the relay and lights the LED

      delay(5000);
  digitalWrite(relayPin4, HIGH); 
//  digitalWrite(relayPin5, LOW);   // energizes the relay and lights the LED

//      delay(2000);
//  digitalWrite(relayPin5, HIGH); 
//  digitalWrite(relayPin6, LOW);   // energizes the relay and lights the LED

//      delay(2000);
//  digitalWrite(relayPin6, HIGH); 
//  digitalWrite(relayPin7, LOW);   // energizes the relay and lights the LED

  }
