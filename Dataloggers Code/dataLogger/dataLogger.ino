#include <Wire.h> //Needed for RTC
#include <RTClib.h> //Needed for RTC
#include <SD.h> //Needed for SD Card
#include <DHT.h> //Needed for Sensors
#include <Adafruit_FONA.h> //Needed for cellular module (SIM800)
#include <SoftwareSerial.h> //Needed for communicating with FONA

#define FONA_RST 1
#define FONA_RX 2  //what digital pin is selected for receiving from FONA cellular module
#define FONA_TX 3  //what digital pin is selected for transmitting to FONA cellular module
#define APN "wholesale" //info from TING, needed to send data
#define USER ""
#define PASS ""
#define TOKEN "3hB2gtSrMKDaZKyPkRt6Egqh7Lft8c"
#define ID1 "57f1d28176254242f42ef05c" //breeding chamber temp
#define ID2 "57fc51d4762542630edea20e" //breeding chamber humidity
#define ID3 "57fc51df7625426342b9e83d" //garage temperature
#define ID4 "57fc51e57625426342b9e85e" //garage humidity
#define ID5 "57fe1c3676254202cb373d0c" //battery percentage
//#define ID6 "Your_id_here" 

#define DHTPIN_4 4     // what digital pin the sensor is connected to (duplicate as necessary)
#define DHTPIN_5 5     // what digital pin the sensor is connected to (duplicate as necessary)
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
DHT dht_4(DHTPIN_4, DHTTYPE);
DHT dht_5(DHTPIN_5, DHTTYPE);

#define relayPin1 6                 // IN1 connected to digital pin 7
#define relayPin2 7                 // IN2 connected to digital pin 8
#define relayPin3 8
#define relayPin4 9

// for the data logging shield, we use digital pin 10 for the SD cs line
#define CHIPSELECTPIN 10
//int relayPin5 = 11;SPI MOSI
//int relayPin6 = 12;SPI MISO
//int relayPin7 = 13;SPI clock


RTC_DS1307 RTC; // define the Real Time Clock object
// the logging file
File logfile;

#define coolingTempHighThreshold 95 //95
#define coolingTempLowThreshold 90 //85

#define heatingTempHighThreshold 87 //87.5
#define heatingTempLowThreshold 82

#define humidityHighThreshold 80
#define humidityLowThreshold 75

boolean exhaustOn = false;
boolean humidifierOn=false;
boolean heatOn = false;
uint8_t battery = 0;

#define LOG_INTERVAL 120000 //time between measurements in millis

// how many milliseconds before writing the logged data permanently to disk
// set it to the LOG_INTERVAL to write each time (safest)
// set it to 10*LOG_INTERVAL to write all data every 10 datareads, you could lose up to 
// the last 10 reads if power is lost but it uses less power and is much faster!
#define SYNC_INTERVAL 300000 // mills between calls to flush() - to write data to the card
uint32_t syncTime = 0; // time of last sync()

//conditional compiling for DEV
#define ECHO_TO_SERIAL 1 // echo data to serial port for monitor/debugging
#define WAIT_TO_START 0 // Wait for serial input in setup() to start running

// the digital pins that connect to the LEDs
#define greenLEDpin 1
#define redLEDpin 2

//software serial is default
SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);
SoftwareSerial *fonaSerial = &fonaSS;

Adafruit_FONA fona = Adafruit_FONA(FONA_RST);

void setup() {
  Serial.begin(9600);

  pinMode(relayPin1, OUTPUT);      // sets the digital pin as output
  pinMode(relayPin2, OUTPUT);      // sets the digital pin as output
  pinMode(relayPin3, OUTPUT);      // sets the digital pin as output
  pinMode(relayPin4, OUTPUT);      // sets the digital pin as output

  digitalWrite(relayPin1, HIGH);        // Prevents relays from starting up engaged
  digitalWrite(relayPin2, HIGH);        // Prevents relays from starting up engaged
  digitalWrite(relayPin3, HIGH);        // Prevents relays from starting up engaged
  digitalWrite(relayPin4, HIGH);        // Prevents relays from starting up engaged

  #if WAIT_TO_START
    #if ECHO_TO_SERIAL
      Serial.println(F("Type any character to start"));
    #endif
    while (!Serial.available());
  #endif //WAIT_TO_START
  
  // initialize the Sensor
  #if ECHO_TO_SERIAL
    Serial.println(F("Initializing Sensor..."));
  #endif
  dht_4.begin(); //initialize the sensor
  dht_5.begin();

  // initialize FONA
  #if ECHO_TO_SERIAL
    Serial.println(F("Initializing FONA..."));
  #endif
  fonaSerial->begin(4800);
  if (! fona.begin(*fonaSerial)) {
    #if ECHO_TO_SERIAL
      Serial.println(F("Couldn't find FONA"));
    #endif
    while (1);
  }

  fona.setGPRSNetworkSettings(F(APN), F(USER), F(PASS));
  #if ECHO_TO_SERIAL
    Serial.println(F("FONA Initialized..."));
  #endif

  // initialize the SD card
  #if ECHO_TO_SERIAL
    Serial.println(F("Initializing SD card..."));
  #endif
  // make sure that the default chip select pin is set to output, even if you don't use it:
  pinMode(CHIPSELECTPIN, OUTPUT);

  // see if the card is present and can be initialized:
  if (!SD.begin(CHIPSELECTPIN)) {
    #if ECHO_TO_SERIAL
      Serial.println(F("Card failed, or not present"));
    #endif
  }
  #if ECHO_TO_SERIAL
    Serial.println(F("card initialized."));
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

  #if ECHO_TO_SERIAL
    if (! logfile) {
       #if ECHO_TO_SERIAL
       Serial.println(F("couldnt create file"));
       #endif       
    }
  #endif

  #if ECHO_TO_SERIAL
    Serial.print(F("Logging to: "));
    Serial.println(filename);
  #endif

  // connect to RTC
  Wire.begin();  
  if (!RTC.begin()) {
    logfile.println("RTC failed");
    #if ECHO_TO_SERIAL
      Serial.println(F("RTC failed"));
    #endif 
  }

  logfile.println(F("Time,Time,Time,Sensor_4,Sensor_4,Sensor_4,Sensor_5,Sensor_5,Sensor_5,Relay,Relay,Relay,Relay,Relay,Relay,Relay,Relay,Relay,Relay"));
  logfile.println(F("millis,stamp,datetime,tempF,tempC,Humidity,tempF,tempC,Humidity,exhaustOn,humidifierOn,heatOn,Relay1 (AC1),Relay2 (AC2),Relay3(Heat),Relay4 (Humidifiers),Relay5 (Exhaust),Relay6,Relay7"));    
  #if ECHO_TO_SERIAL
    Serial.println(F("Time,Time,Time,Sensor_4,Sensor_4,Sensor_4,Sensor_5,Sensor_5,Sensor_5,Relay,Relay,Relay,Relay,Relay,Relay,Relay,Relay,Relay,Relay"));
    Serial.println(F("millis,stamp,datetime,tempF,tempC,Humidity,tempF,tempC,Humidity,exhaustOn,humidifierOn,heatOn,Relay1 (AC1),Relay2 (AC2),Relay3(Heat),Relay4 (Humidifiers),Relay5 (Exhaust),Relay6,Relay7")); 
  #endif //ECHO_TO_SERIAL
}

void loop() {
  DateTime now;

  // delay for the amount of time we want between readings
  delay((LOG_INTERVAL -1) - (millis() % LOG_INTERVAL));

  // log milliseconds since starting
  uint32_t m = millis();
  logfile.print(m);           // milliseconds since start
  logfile.print(", ");    
  #if ECHO_TO_SERIAL
    Serial.print(m);         // milliseconds since start
    Serial.print(F(", "));  
  #endif

  // fetch the time
  now = RTC.now();

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h_4 = dht_4.readHumidity();
  float fTemp_4 = dht_4.readTemperature(true);
  float h_5 = dht_5.readHumidity();
  float fTemp_5 = dht_5.readTemperature(true);
  // Read battery level on FONA LiPo Battery
  uint16_t vbat;
  if (! fona.getBattPercent(&vbat)) {
      #if ECHO_TO_SERIAL
      Serial.println(F("Failed to read Batt"));
      #endif
  } else {
      #if ECHO_TO_SERIAL
      Serial.print(F("VPct = ")); Serial.print(vbat); Serial.println(F("%"));
      #endif
  }

  // Check if any reads failed and exit early (to try again).
  if ((isnan(h_4) || isnan(fTemp_4)) && (isnan(h_5) || isnan(fTemp_5))) {
    #if ECHO_TO_SERIAL
      Serial.println(F("Failed to read from DHT sensor!"));
      #endif
    logfile.println(F("Failed to read from DHT sensor!"));
    return;
  }

  // Control section
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
//  logfile.print(cTemp_4);
//  logfile.print(","); 
  logfile.print(h_4);
  logfile.print(",");   
  logfile.print(fTemp_5);
  logfile.print(","); 
//  logfile.print(cTemp_5);
//  logfile.print(","); 
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
//    Serial.print(cTemp_4);
//    Serial.print(", "); 
    Serial.print(h_4);
    Serial.print(",");
    Serial.print(", ");   
    Serial.print(fTemp_5);
    Serial.print(", "); 
//    Serial.print(cTemp_5);
//    Serial.print(", "); 
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

//  Serial.print((m - syncTime+5));
//  Serial.print("\t");
//  Serial.print(SYNC_INTERVAL);
//  Serial.print("\t");
//  Serial.println((m - syncTime+5) < SYNC_INTERVAL);

  // Now we write data to disk! Don't sync too often - requires 2048 bytes of I/O to SD card
  // which uses a bunch of power and takes time
  if ((m - syncTime+5) < SYNC_INTERVAL) return;
  syncTime = m;
  Serial.println(F("writing to SD card"));

  // blink LED to show we are syncing data to the card & updating FAT!
  //digitalWrite(redLEDpin, HIGH);
  logfile.flush();
  delay(100);
  //digitalWrite(redLEDpin, LOW);

// Post data to website
  #if ECHO_TO_SERIAL
     Serial.println(F("Posting data to ubidots..."));
  #endif
//GPRS on
  GPRStoggle(true);
  uint16_t statuscode;
  int16_t length;
  //char url[107]="things.ubidots.com/api/v1.6/collections/values/?token=" TOKEN;
  char data[300];
  char val1[10], val2[10], val3[10], val4[10], val5[10], val6[10];
  
  //parsing data into char array to send in HTTP POST
  dtostrf(fTemp_4,7, 3, val1);
  dtostrf(h_4,7, 3, val2);
  dtostrf(fTemp_5,7, 3, val3);
  dtostrf(h_5,7, 3, val4);
  dtostrf(vbat,7, 3, val5);
  sprintf(data,"[{\"variable\": \"%s\", \"value\":%s}, {\"variable\": \"%s\", \"value\":%s}, {\"variable\": \"%s\", \"value\":%s}, {\"variable\": \"%s\", \"value\":%s},{\"variable\": \"%s\", \"value\":%s}]", ID1, val1, ID2, val2, ID3, val3, ID4, val4, ID5, val5);

  #if ECHO_TO_SERIAL
    Serial.print(F("http://"));
    Serial.println(F("things.ubidots.com/api/v1.6/collections/values/?token=" TOKEN));
    Serial.println(data);
    Serial.println(F("****SENDING****"));
  #endif

  if (!fona.HTTP_POST_start("things.ubidots.com/api/v1.6/collections/values/?token=" TOKEN, F("application/json"), (uint8_t *) data, strlen(data), &statuscode, (uint16_t *)&length)) {
    #if ECHO_TO_SERIAL
      Serial.println(F("Failed to POST!"));
    #endif
  }
  while (length > 0) {
    while (fona.available()) {
      char c = fona.read();

#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
      loop_until_bit_is_set(UCSR0A, UDRE0); /* Wait until data register empty. */
      UDR0 = c;
#else
      Serial.write(c);
#endif

      length--;
      if (! length) break;
    }
  }

  #if ECHO_TO_SERIAL
    Serial.println(F("\n****"));
  #endif

  fona.HTTP_POST_end();
  
  GPRStoggle(false);//GPRS off

  while (fona.available()) {
    Serial.write(fona.read());
  }

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

void GPRStoggle(bool state){
  uint8_t i = 0;
  while (!fona.enableGPRS(state) && i<10){
    i++;
    #if ECHO_TO_SERIAL
      if (state) Serial.println(F("Failed to turn on"));
        else Serial.println(F("Failed to turn off"));
      Serial.println(i);
    #endif
    delay(1500);
    }
  }

  //NEED TO MAKE FUNCTION THAT DOES THIS:
  /*
    void senddata(float fval1, float fval2, float fval3, float fval4, float fval5){
      GPRStoggle(true);
      uint16_t statuscode;
      int16_t length;
      //char url[107]="things.ubidots.com/api/v1.6/collections/values/?token=" TOKEN;
      char data[300];
      char val1[10], val2[10], val3[10], val4[10], val5[10], val6[10];
      
      //parsing data into char array to send in HTTP POST
      dtostrf(fval1,7, 3, val1);
      dtostrf(fval2,7, 3, val2);
      dtostrf(fval3,7, 3, val3);
      dtostrf(favl4,7, 3, val4);
      dtostrf(fval5,7, 3, val5);
      sprintf(data,"[{\"variable\": \"%s\", \"value\":%s}, {\"variable\": \"%s\", \"value\":%s}, {\"variable\": \"%s\", \"value\":%s}, {\"variable\": \"%s\", \"value\":%s},{\"variable\": \"%s\", \"value\":%s}]", ID1, val1, ID2, val2, ID3, val3, ID4, val4, ID5, val5);
    
      #if ECHO_TO_SERIAL
        Serial.print(F("http://"));
        Serial.println(F("things.ubidots.com/api/v1.6/collections/values/?token=" TOKEN));
        Serial.println(data);
        Serial.println(F("****SENDING****"));
      #endif
    
      if (!fona.HTTP_POST_start("things.ubidots.com/api/v1.6/collections/values/?token=" TOKEN, F("application/json"), (uint8_t *) data, strlen(data), &statuscode, (uint16_t *)&length)) {
        #if ECHO_TO_SERIAL
          Serial.println(F("Failed to POST!"));
        #endif
      }
      while (length > 0) {
        while (fona.available()) {
          char c = fona.read();
    
    #if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
          loop_until_bit_is_set(UCSR0A, UDRE0); // Wait until data register empty.
          UDR0 = c;
    #else
          Serial.write(c);
    #endif
    
          length--;
          if (! length) break;
        }
      }
    
      #if ECHO_TO_SERIAL
        Serial.println(F("\n****"));
      #endif
    
      fona.HTTP_POST_end();
      
  GPRStoggle(false);//GPRS off
    }
  */

