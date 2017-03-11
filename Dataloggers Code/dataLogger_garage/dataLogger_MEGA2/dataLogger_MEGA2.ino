#include <Wire.h> //Needed for RTC
#include <RTClib.h> //Needed for RTC
#include <SD.h> //Needed for SD Card
#include <DHT.h> //Needed for Sensors
#include "config.h" //network information, etc
#include <avr/wdt.h>

//***** DEBUG TOOLS *********
#define ECHO_TO_SERIAL 1 // echo data to serial port
#define WAIT_TO_START 0 // Wait for serial input in setup()
#define INCLUDE_SD 0 // Include SD Card Code in upload

//***** SENSOR PINS ***********
#define DHTPIN_4 4     // what digital pin the sensor is connected to (duplicate as necessary)
#define DHTPIN_5 5     // what digital pin the sensor is connected to (duplicate as necessary)
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
DHT dht_4(DHTPIN_4, DHTTYPE);
DHT dht_5(DHTPIN_5, DHTTYPE);

//****** GLOBAL ENV DATA *******
float h_4     = 0;
float fTemp_4 = 0;
float cTemp_4 = 0;
float h_5     = 0;
float fTemp_5 = 0;
float cTemp_5 = 0;

//******* RELAY VARIABLES *******
int relayPin1 = 31; // AC1
int relayPin2 = 33; // AC1
int relayPin3 = 35; // DEAD
int relayPin4 = 37; // Humidifiers power strip
int relayPin5 = 39; // Heat2
int relayPin6 = 41; // Heat2
int relayPin7 = 43; 
int relayPin8 = 45;

boolean exhaustOn = false;
boolean humidifierOn=false;
boolean heatOn = false;

//********************FONA GLOBAL DECLARATIONS *******************

SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);
SoftwareSerial *fonaSerial = &fonaSS;

// Use this for FONA 800 and 808s
Adafruit_FONA fona = Adafruit_FONA(FONA_RST);
uint16_t vbat;

//******************SD CARD VARIABLES ****************************
#if INCLUDE_SD
  
// how many milliseconds before writing the logged data permanently to disk
// set it to the LOG_INTERVAL to write each time (safest)
// set it to 10*LOG_INTERVAL to write all data every 10 datareads, you could lose up to 
// the last 10 reads if power is lost but it uses less power and is much faster!
#define SYNC_INTERVAL 60000 // mills between calls to flush() - to write data to the card
#define LOG_INTERVAL 5000 //time between measurements in millis
uint32_t syncTime = 0; // time of last sync()

// the digital pins that connect to the LEDs on SD card shield
#define greenLEDpin 2
#define redLEDpin 3

RTC_DS1307 RTC; // define the Real Time Clock object

// for the data logging shield, we use digital pin for the SD cs line
const int chipSelect = 53; //For Mega, MOSI=51, MISO=50, SCK=52, SS=53
File logfile; // the logging file
#endif



//*********** IMPORTANT THRESHOLDS FOR TEMP AND HUMIDITY**********
#define coolingTempHighThreshold 95 //95
#define coolingTempLowThreshold 90 //85

#define heatingTempHighThreshold 87 //87.5
#define heatingTempLowThreshold 82

#define humidityHighThreshold 80
#define humidityLowThreshold 75

void setup() {
  Serial.begin(9600);

  pinMode(relayPin1, OUTPUT);      // sets the digital pin as output
  pinMode(relayPin2, OUTPUT);      // sets the digital pin as output
  pinMode(relayPin3, OUTPUT);      // sets the digital pin as output
  pinMode(relayPin4, OUTPUT);      // sets the digital pin as output  
  pinMode(relayPin5, OUTPUT);      // sets the digital pin as output
  pinMode(relayPin6, OUTPUT);      // sets the digital pin as output
  pinMode(relayPin7, OUTPUT);      // sets the digital pin as output
  pinMode(relayPin8, OUTPUT);      // sets the digital pin as output

  digitalWrite(relayPin1, HIGH);        // Prevents relays from starting up engaged
  digitalWrite(relayPin2, HIGH);        // Prevents relays from starting up engaged
  digitalWrite(relayPin3, HIGH);        // Prevents relays from starting up engaged
  digitalWrite(relayPin4, HIGH);        // Prevents relays from starting up engaged
  digitalWrite(relayPin5, HIGH);        // Prevents relays from starting up engaged
  digitalWrite(relayPin6, HIGH);        // Prevents relays from starting up engaged
  digitalWrite(relayPin7, HIGH);        // Prevents relays from starting up engaged
  digitalWrite(relayPin8, HIGH);        // Prevents relays from starting up engaged

  configureFONA(); 
  wdt_enable(WDTO_8S);
  wdt_reset();
  
  
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
  envControl();

#if INCLUDE_SD
  // use debugging LEDs
  pinMode(redLEDpin, OUTPUT);
  pinMode(greenLEDpin, OUTPUT);
  
  // initialize the SD card
  #if ECHO_TO_SERIAL
    Serial.println("Initializing SD card...");
  #endif
  // make sure that the default chip select pin is set to output, even if you don't use it:
  pinMode(chipSelect, OUTPUT);

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
    }
  }
  
  if (! logfile) {
    error("couldnt create file");
    wdt_reset();
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

  logfile.println(F("Time,Time,Time,Sensor_4,Sensor_4,Sensor_4,Sensor_5,Sensor_5,Sensor_5,Relay,Relay,Relay,Relay,Relay,Relay,Relay,Relay,Relay,Relay"));
  logfile.println(F("millis,stamp,datetime,tempF,tempC,Humidity,tempF,tempC,Humidity,exhaustOn,humidifierOn,heatOn,Relay1 (AC1),Relay2 (AC2),Relay3(Heat),Relay4 (Humidifiers),Relay5 (Exhaust),Relay6,Relay7"));    
#endif //INCLUDE_SD

}




/*=====================================   START LOOP    =======================================*/
void loop() {
  #if ECHO_TO_SERIAL
    Serial.println(F("Starting loop..."));
  #endif
  
  #if INCLUDE_SD
    DateTime now;    
    // delay for the amount of time we want between readings
    delay((LOG_INTERVAL -1) - (millis() % LOG_INTERVAL));digitalWrite(greenLEDpin, HIGH);
  
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
    #if ECHO_TO_SERIAL
      Serial.println(F("Time fetched..."));
    #endif
  #endif



  
  //******************MAKE SURE BUGS ARE OK ***********
  envControl(); 
  //******** I HOPE THEY'RE HAVING FUN IN THERE ************



  
  #if INCLUDE_SD
    /* --------- LOG ALL THE THINGS -------------------- */
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
      Serial.print(", "); 
      Serial.print(digitalRead(relayPin5));
      Serial.print(", "); 
      Serial.print(digitalRead(relayPin6));
      Serial.print(", "); 
      Serial.print(digitalRead(relayPin7));  
      Serial.println();
    #endif //ECHO_TO_SERIAL
  
    digitalWrite(greenLEDpin, LOW);
  
    // Now we write data to disk! Don't sync too often - requires 2048 bytes of I/O to SD card
    // which uses a bunch of power and takes time
    //if ((m - syncTime+5) < SYNC_INTERVAL) return;
    syncTime = m;
    Serial.println("writing to SD card");
  
    // blink LED to show we are syncing data to the card & updating FAT!
    digitalWrite(redLEDpin, HIGH);
    logfile.flush();
    delay(100);
    digitalWrite(redLEDpin, LOW);
  #endif //INCLUDE_SD

  /******FONA ****/  
  if (! fona.getBattPercent(&vbat)) {
    Serial.println(F("Failed to read Batt"));
  } else {
    Serial.print(F("VPct = ")); Serial.print(vbat); Serial.println(F("%"));
  }
  
  sendData(fTemp_4, h_4, fTemp_5, h_5, vbat);
  wdt_reset();
  delay_sec(DELAY_SEC);
}

/*=====================================END LOOP=======================================*/

void envControl(){
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  h_4 = dht_4.readHumidity();
  #if ECHO_TO_SERIAL
    Serial.print(F("h_4 (Gh) = "));
    Serial.println(h_4);
  #endif
  // Read temperature as Celsius (the default)
  cTemp_4 = dht_4.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  fTemp_4 = dht_4.readTemperature(true);
  #if ECHO_TO_SERIAL
    Serial.print(F("fTemp_4 (Gt) = "));
    Serial.println(fTemp_4);
  #endif
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  h_5 = dht_5.readHumidity();
  #if ECHO_TO_SERIAL
    Serial.print(F("h_5 (BCh) = "));
    Serial.println(h_5);
  #endif
  // Read temperature as Celsius (the default)
  cTemp_5 = dht_5.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  fTemp_5 = dht_5.readTemperature(true);
  #if ECHO_TO_SERIAL
    Serial.print(F("fTemp_5 (BCt) = "));
    Serial.println(fTemp_5);
  #endif
  
  // Check if any reads failed and exit early (to try again).
  if ((isnan(h_4) || isnan(cTemp_4) || isnan(fTemp_4)) && (isnan(h_5) || isnan(cTemp_5) || isnan(fTemp_5))) {
    #if ECHO_TO_SERIAL
      Serial.println("Failed to read from DHT sensor!");
      #endif
    #if INCLUDE_SD
      logfile.println("Failed to read from DHT sensor!");
    #endif
    
    return;
  }

 // Control section
    if(heatOn==false && fTemp_5<heatingTempLowThreshold){
     digitalWrite(relayPin5, LOW);   // energizes the relay and lights the LED
     digitalWrite(relayPin6, LOW);   // energizes the relay and lights the LED
     heatOn=true;
  } else if(heatOn==true && fTemp_5>heatingTempHighThreshold){
     digitalWrite(relayPin5, HIGH);   // de-energizes the relay and turns off the LED
     digitalWrite(relayPin6, HIGH);   // de-energizes the relay and turns off the LED
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
     digitalWrite(relayPin1, LOW);   // energizes the relay and lights the LED
     digitalWrite(relayPin2, LOW);   // energizes the relay and lights the LED
     exhaustOn=true;
  } else if(exhaustOn==true && fTemp_5<coolingTempLowThreshold){
    digitalWrite(relayPin1, HIGH);   // de-energizes the relay and turns off the LED
    digitalWrite(relayPin2, HIGH);   // de-energizes the relay and turns off the LED
     exhaustOn=false;
  }
}


/* THROW ERROR FUNCTION FOR SD CARD*/
void error(char *str){
  #if INCLUDE_SD
    #if ECHO_TO_SERIAL
      Serial.print("error: ");
      Serial.println(str);
    #endif
  
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
  #endif
  wdt_reset();
}

void configureFONA(){
  Serial.println(F("Initializing....(May take 3 seconds)"));
  fonaSerial->begin(4800);
  if (! fona.begin(*fonaSerial)) {
    Serial.println(F("Couldn't find FONA"));
  }
  Serial.println(F("FONA is OK"));
  wdt_reset();
  // configure a GPRS APN, username, and password.
  fona.setGPRSNetworkSettings(F(APN), F(USER), F(PASS));
}

void enableGPRS(){
  int i=0;
  while (!fona.enableGPRS(true) && i<10){
    wdt_reset();
    Serial.println(F("Failed to turn on"));
    fona.enableGPRS(false);
    delay(1500);
    Serial.println(i++);
    }
}

//void sendData(float fTemp_4, float h_4, float fTemp_5, float h_5, uint16_t vbat){
void sendData(float BCf, float BCh, float Gf, float Gh, uint16_t vbat){
  enableGPRS();
  // Post data to website
  wdt_reset();
  Serial.println(F("Posting data..."));
  uint16_t statuscode;
  int16_t length;
  char url[107]="things.ubidots.com/api/v1.6/collections/values/?token="TOKEN;
  char data[300];
  char val1[10], val2[10], val3[10], val4[10], val5[10];
  char val6[5];
  bool val7, val8, val9;
  dtostrf(BCf,6, 2, val1);
  dtostrf(BCh,6, 2, val2);
  dtostrf(Gf,6, 2, val3);
  dtostrf(Gh,6, 2, val4);
  dtostrf(vbat,3, 0, val5);
  sprintf(data,"[{\"variable\": \"%s\", \"value\":%s}, {\"variable\": \"%s\", \"value\":%s}, {\"variable\": \"%s\", \"value\":%s}, {\"variable\": \"%s\", \"value\":%s},{\"variable\": \"%s\", \"value\":%s}]", ID1, val1, ID2, val2, ID3, val3, ID4, val4, ID5, val5);

  //flushSerial();
  Serial.print(F("http://"));
  Serial.println(url);
  Serial.println(data);
  Serial.println(F("****SENDING****"));
  if (!fona.HTTP_POST_start(url, F("application/json"), (uint8_t *) data, strlen(data), &statuscode, (uint16_t *)&length)) {
    Serial.println("Failed!");
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
  Serial.println(F("\n****"));
  fona.HTTP_POST_end();
  int i=0;
  while (!fona.enableGPRS(false) && i<10){
    wdt_reset();
    Serial.println(F("Failed to turn off"));
    delay(1500);
    Serial.println(i++);
    }
  Serial.println(F("GPRS STATUS:"));
  fona.GPRSstate();
  // flush input
  //flushSerial();
  while (fona.available()) {
    Serial.write(fona.read());
  }  
  }

void delay_sec(int seconds){
  int sec=seconds;
  while (sec>0){
    delay(1000);
    sec--;
    wdt_reset();
    #if ECHO_TO_SERIAL
      Serial.print(sec);
      Serial.print(" ");
    #endif 
  }
}

  
