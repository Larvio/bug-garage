#include <DHT.h> //Needed for Sensors
#include <Wire.h> //Needed for RTC
#include <RTClib.h> //Needed for RTC
#include <SD.h> //Needed for SD Card


//***** GENERAL CONFIG & DEBUG TOOLS *********
#define ECHO_TO_SERIAL 1 // echo data to serial port
#define WAIT_TO_START 0 // Wait for serial input in setup()
#define DELAY_SEC 30 // seconds delay between measurements
#define INCLUDE_SD 1 // Include SD Card Code in upload
#define IF_LOGGER 1 //generate logger csv
#define IF_DEBUG 1 //generate debug txt file
#define BAUD_RATE 9600

//***** SENSOR PINS ***********
#define DHTPIN_A 22     // what digital pin the sensor is connected to (duplicate as necessary)
#define DHTPIN_B 23     // what digital pin the sensor is connected to (duplicate as necessary)
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
DHT dht_A(DHTPIN_A, DHTTYPE);
DHT dht_B(DHTPIN_B, DHTTYPE);

//****** GLOBAL ENV DATA *******
float h_A     = 0;
float fTemp_A = 0;
float cTemp_A = 0;
float h_B     = 0;
float fTemp_B = 0;
float cTemp_B = 0;

//******* RELAY VARIABLES *******
#define relayPin1 38 // AC1
#define relayPin2 39 // AC1
#define relayPin3 40 // Heat2
#define relayPin4 41 // Heat2
#define relayPin5 42 // not hooked up
#define relayPin6 43 // not hooked up 
#define relayPin7 44 // not hooked up
#define relayPin8 45 // Sensors
#define relayPin9 46 // Humidifiers
#define relayPin10 47 // blank
#define relayPin11 48 // blank
#define relayPin12 49 // blank
#define relayPin13 50 // blank
#define relayPin14 51 // blank
#define relayPin15 52 // blank
#define relayPin16 53 // blank

boolean exhaustOnState = false;
boolean humidifierOnState=false;
boolean heatOnState = false;

//*********** IMPORTANT THRESHOLDS FOR TEMP AND HUMIDITY**********
#define coolingTempHighThreshold 95 //95
#define coolingTempLowThreshold 90 //85

#define heatingTempHighThreshold 87 //87.5
#define heatingTempLowThreshold 82

#define humidityHighThreshold 80
#define humidityLowThreshold 75

//******************SD CARD VARIABLES ****************************
#if INCLUDE_SD
  
  // how many milliseconds before writing the logged data permanently to disk
  // set it to the LOG_INTERVAL to write each time (safest)
  // set it to 10*LOG_INTERVAL to write all data every 10 datareads, you could lose up to 
  // the last 10 reads if power is lost but it uses less power and is much faster!
  #define SYNC_INTERVAL 60000 // mills between calls to flush() - to write data to the card
  uint32_t syncTime = 0; // time of last sync()
  
  // the digital pins that connect to the LEDs on SD card shield
  #define greenLEDpin 28
  #define redLEDpin 29
  
  RTC_DS1307 RTC; // define the Real Time Clock object
  
  // for the data logging shield, we use digital pin for the SD cs line
  const int chipSelect = 24; //For Mega, MOSI=51, MISO=50, SCK=52, SS=53
  File logfile; // the logging file
  File debugFile; //the serial log
#endif //include sd

void setup() {
  String DebugInitialText = "";
  
  Serial.begin(BAUD_RATE);

  pinMode(relayPin1, OUTPUT);      // sets the digital pin as output
  pinMode(relayPin2, OUTPUT);      // sets the digital pin as output
  pinMode(relayPin3, OUTPUT);      // sets the digital pin as output
  pinMode(relayPin4, OUTPUT);      // sets the digital pin as output  
  pinMode(relayPin5, OUTPUT);      // sets the digital pin as output
  pinMode(relayPin6, OUTPUT);      // sets the digital pin as output
  pinMode(relayPin7, OUTPUT);      // sets the digital pin as output
  pinMode(relayPin8, OUTPUT);      // sets the digital pin as output
  pinMode(relayPin9, OUTPUT);      // sets the digital pin as output
  pinMode(relayPin10, OUTPUT);      // sets the digital pin as output
  pinMode(relayPin11, OUTPUT);      // sets the digital pin as output
  pinMode(relayPin12, OUTPUT);      // sets the digital pin as output  
  pinMode(relayPin13, OUTPUT);      // sets the digital pin as output
  pinMode(relayPin14, OUTPUT);      // sets the digital pin as output
  pinMode(relayPin15, OUTPUT);      // sets the digital pin as output
  pinMode(relayPin16, OUTPUT);      // sets the digital pin as output

  digitalWrite(relayPin1, HIGH);        // Prevents relays from starting up engaged
  digitalWrite(relayPin2, HIGH);        // Prevents relays from starting up engaged
  digitalWrite(relayPin3, HIGH);        // Prevents relays from starting up engaged
  digitalWrite(relayPin4, HIGH);        // Prevents relays from starting up engaged
  digitalWrite(relayPin5, HIGH);        // Prevents relays from starting up engaged
  digitalWrite(relayPin6, HIGH);        // Prevents relays from starting up engaged
  digitalWrite(relayPin7, HIGH);        // Prevents relays from starting up engaged
  digitalWrite(relayPin8, HIGH);        // Prevents relays from starting up engaged
  digitalWrite(relayPin9, HIGH);        // Prevents relays from starting up engaged
  digitalWrite(relayPin10, HIGH);        // Prevents relays from starting up engaged
  digitalWrite(relayPin11, HIGH);        // Prevents relays from starting up engaged
  digitalWrite(relayPin12, HIGH);        // Prevents relays from starting up engaged
  digitalWrite(relayPin13, HIGH);        // Prevents relays from starting up engaged
  digitalWrite(relayPin14, HIGH);        // Prevents relays from starting up engaged
  digitalWrite(relayPin15, HIGH);        // Prevents relays from starting up engaged
  digitalWrite(relayPin16, HIGH);        // Prevents relays from starting up engaged
  
  #if WAIT_TO_START
    #if ECHO_TO_SERIAL
      Serial.println("Type any character to start");
    #endif
    #if IF_DEBUG
      DebugInitialText += "Type any character to start\n";
    #endif
    while (!Serial.available());
  #endif //WAIT_TO_START
  
  // initialize the Sensor
  #if ECHO_TO_SERIAL
    Serial.println("Initializing Sensor...");
  #endif
  #if IF_DEBUG
      DebugInitialText += "Initializing Sensor...\n";
  #endif
  dht_A.begin(); //initialize the sensor
  dht_B.begin(); //initialize the sensor

  #if INCLUDE_SD
    // use debugging LEDs
    pinMode(redLEDpin, OUTPUT);
    pinMode(greenLEDpin, OUTPUT);
    
    // initialize the SD card
    #if ECHO_TO_SERIAL
      Serial.println("Initializing SD card...");
    #endif
    #if IF_DEBUG
      DebugInitialText += "Initializing SD card...\n";
    #endif
    // make sure that the default chip select pin is set to output, even if you don't use it:
    pinMode(chipSelect, OUTPUT);
  
    // see if the card is present and can be initialized:
    if (!SD.begin(24, 25, 26, 27)) { //@andrew
      error("Card failed, or not present");
    }
    #if ECHO_TO_SERIAL
      Serial.println("card initialized.");
    #endif
    #if IF_DEBUG
      DebugInitialText += "card initialized.\n";
    #endif

    // connect to RTC
    Wire.begin();  
    if (!RTC.begin()) {
      #if ECHO_TO_SERIAL
        Serial.println("RTC failed");
      #endif
      #if IF_DEBUG
        DebugInitialText += "RTC failed\n";
      #endif
    }

    //Read RTC Vaue since Battery is Dead
    readAndSetTime(DebugInitialText);

    #if IF_DEBUG
      // create a new file
      char filename2[] = "DEBUG00.TXT";
      for (uint8_t i = 0; i < 100; i++) {
        filename2[5] = i/10 + '0';
        filename2[6] = i%10 + '0';
        if (! SD.exists(filename2)) {
          // only open a new file if it doesn't exist
          debugFile = SD.open(filename2, FILE_WRITE);
          break;
        }
      }
      
      if (! debugFile) {
        error("couldnt create file");
      }
  
      #if ECHO_TO_SERIAL
        Serial.print("Debug log to: ");
        Serial.println(filename2);
      #endif//echo to serial
      debugFile.print(DebugInitialText);
      debugFile.print("Debug log to: ");
      debugFile.println(filename2);
    #endif //ifdebug

    #if IF_LOGGER
      // create a new file
      char filename[] = "LOGGER00.CSV";
      for (uint8_t i = 0; i < 100; i++) {
        filename[6] = i/10 + '0';
        filename[7] = i%10 + '0';
        if (! SD.exists(filename)) {
          // only open a new file if it doesn't exist
          logfile = SD.open(filename, FILE_WRITE);
          break;
        }
      }
      
      if (! logfile) {
        error("couldnt create file");
      }
    
      #if ECHO_TO_SERIAL
        Serial.print("Logging to: ");
        Serial.println(filename);
      #endif
      #if IF_DEBUG
        debugFile.print("Logging to: ");
        debugFile.println(filename);
      #endif
      
      logfile.println(F("Time,Time,Time,Sensor_A,Sensor_A,Sensor_A,Sensor_B,Sensor_B,Sensor_B,BooleanState,BooleanState,BooleanState,Relay,Relay,Relay,Relay,Relay,Relay,Relay,Relay,Relay,Relay,Relay,Relay,Relay,Relay,Relay,Relay"));
      logfile.println(F("millis,unix,datetime,tempF,tempC,Humidity,tempF,tempC,Humidity,exhaustOnState,humidifierOnState,heatOnState,Relay1,Relay2,Relay3,Relay4,Relay5,Relay6,Relay7,Relay8,Relay9,Relay10,Relay11,Relay12,Relay13,Relay14,Relay15,Relay16"));
    #endif //iflogger
  #endif //INCLUDE_SD

}




/*=====================================   START LOOP    =======================================*/
void loop() {
  #if ECHO_TO_SERIAL
    Serial.println(F("Starting loop..."));
  #endif
  #if IF_DEBUG
    debugFile.println(F("Starting loop..."));
  #endif

  #if INCLUDE_SD
    DateTime now;    
    digitalWrite(greenLEDpin, HIGH);
  
    // log milliseconds since starting
    uint32_t m = millis();
    #if IF_LOGGER
      logfile.print(m);           // milliseconds since start
      logfile.print(", ");
    #endif
        
    #if ECHO_TO_SERIAL
      Serial.print(m);         // milliseconds since start
      Serial.print(", ");
    #endif
    #if IF_DEBUG
      debugFile.print(m);
      debugFile.print(", ");
    #endif
    
    // fetch the time
    now = RTC.now();
    #if ECHO_TO_SERIAL
      Serial.println(F("Time fetched..."));
    #endif
    #if IF_DEBUG
      debugFile.println(F("Time fetched..."));
    #endif
  #endif //includesd
  
  //******************MAKE SURE BUGS ARE OK ***********
  envControl(); 
  //******** I HOPE THEY'RE HAVING FUN IN THERE ************

  #if INCLUDE_SD
    #if IF_LOGGER
      /* --------- LOG ALL THE THINGS -------------------- */
      logfile.print(m);
      logfile.print(",");
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
      logfile.print(fTemp_A);
      logfile.print(","); 
      logfile.print(cTemp_A);
      logfile.print(","); 
      logfile.print(h_A);
      logfile.print(",");   
      logfile.print(fTemp_B);
      logfile.print(","); 
      logfile.print(cTemp_B);
      logfile.print(","); 
      logfile.print(h_B);
      logfile.print(","); 
      logfile.print(exhaustOnState);
      logfile.print(","); 
      logfile.print(humidifierOnState);
      logfile.print(","); 
      logfile.print(heatOnState);
      logfile.print(", "); 
      logfile.print(digitalRead(relayPin1));
      logfile.print(", "); 
      logfile.print(digitalRead(relayPin2));
      logfile.print(", "); 
      logfile.print(digitalRead(relayPin3));
      logfile.print(", "); 
      logfile.print(digitalRead(relayPin4));
      logfile.print(", "); 
      logfile.print(digitalRead(relayPin5));
      logfile.print(", "); 
      logfile.print(digitalRead(relayPin6));
      logfile.print(", "); 
      logfile.print(digitalRead(relayPin7));
      logfile.print(", "); 
      logfile.print(digitalRead(relayPin8));
      logfile.print(", "); 
      logfile.print(digitalRead(relayPin9));
      logfile.print(", "); 
      logfile.print(digitalRead(relayPin10));
      logfile.print(", "); 
      logfile.print(digitalRead(relayPin11));
      logfile.print(", "); 
      logfile.print(digitalRead(relayPin12));
      logfile.print(", "); 
      logfile.print(digitalRead(relayPin13));
      logfile.print(", "); 
      logfile.print(digitalRead(relayPin14)); 
      logfile.print(", "); 
      logfile.print(digitalRead(relayPin15));
      logfile.print(", "); 
      logfile.print(digitalRead(relayPin16)); 
      logfile.println();
    #endif //iflogger
    #if ECHO_TO_SERIAL
      Serial.print(m);
      Serial.print(",");
      Serial.print(now.unixtime()); // seconds since 1/1/1970
      Serial.print(",");
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
      Serial.print(fTemp_A);
      Serial.print(", "); 
      Serial.print(cTemp_A);
      Serial.print(", "); 
      Serial.print(h_A);
      Serial.print(", ");   
      Serial.print(fTemp_B);
      Serial.print(", "); 
      Serial.print(cTemp_B);
      Serial.print(", "); 
      Serial.print(h_B);
      Serial.print(", "); 
      Serial.print(exhaustOnState);
      Serial.print(", "); 
      Serial.print(humidifierOnState);
      Serial.print(", "); 
      Serial.print(heatOnState);
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
      Serial.print(", "); 
      Serial.print(digitalRead(relayPin8));
      Serial.print(", "); 
      Serial.print(digitalRead(relayPin9));
      Serial.print(", "); 
      Serial.print(digitalRead(relayPin10));
      Serial.print(", "); 
      Serial.print(digitalRead(relayPin11));
      Serial.print(", "); 
      Serial.print(digitalRead(relayPin12));
      Serial.print(", "); 
      Serial.print(digitalRead(relayPin13));
      Serial.print(", "); 
      Serial.print(digitalRead(relayPin14)); 
      Serial.print(", "); 
      Serial.print(digitalRead(relayPin15));
      Serial.print(", "); 
      Serial.print(digitalRead(relayPin16)); 
      Serial.println();
    #endif //echotoserial
    #if IF_DEBUG
      debugFile.print(m);
      debugFile.print(",");
      debugFile.print(now.unixtime()); // seconds since 1/1/1970
      debugFile.print(",");
      debugFile.print('"');
      debugFile.print(now.year(), DEC);
      debugFile.print("/");
      debugFile.print(now.month(), DEC);
      debugFile.print("/");
      debugFile.print(now.day(), DEC);
      debugFile.print(" ");
      debugFile.print(now.hour(), DEC);
      debugFile.print(":");
      debugFile.print(now.minute(), DEC);
      debugFile.print(":");
      debugFile.print(now.second(), DEC);
      debugFile.print('"');
      debugFile.print(", ");   
      debugFile.print(fTemp_A);
      debugFile.print(", "); 
      debugFile.print(cTemp_A);
      debugFile.print(", "); 
      debugFile.print(h_A);
      debugFile.print(", ");   
      debugFile.print(fTemp_B);
      debugFile.print(", "); 
      debugFile.print(cTemp_B);
      debugFile.print(", "); 
      debugFile.print(h_B);
      debugFile.print(", "); 
      debugFile.print(exhaustOnState);
      debugFile.print(", "); 
      debugFile.print(humidifierOnState);
      debugFile.print(", "); 
      debugFile.print(heatOnState);
      debugFile.print(", "); 
      debugFile.print(digitalRead(relayPin1));
      debugFile.print(", "); 
      debugFile.print(digitalRead(relayPin2));
      debugFile.print(", "); 
      debugFile.print(digitalRead(relayPin3));
      debugFile.print(", "); 
      debugFile.print(digitalRead(relayPin4));
      debugFile.print(", "); 
      debugFile.print(digitalRead(relayPin5));
      debugFile.print(", "); 
      debugFile.print(digitalRead(relayPin6));
      debugFile.print(", "); 
      debugFile.print(digitalRead(relayPin7));
      debugFile.print(", "); 
      debugFile.print(digitalRead(relayPin8));
      debugFile.print(", "); 
      debugFile.print(digitalRead(relayPin9));
      debugFile.print(", "); 
      debugFile.print(digitalRead(relayPin10));
      debugFile.print(", "); 
      debugFile.print(digitalRead(relayPin11));
      debugFile.print(", "); 
      debugFile.print(digitalRead(relayPin12));
      debugFile.print(", "); 
      debugFile.print(digitalRead(relayPin13));
      debugFile.print(", "); 
      debugFile.print(digitalRead(relayPin14)); 
      debugFile.print(", "); 
      debugFile.print(digitalRead(relayPin15));
      debugFile.print(", "); 
      debugFile.print(digitalRead(relayPin16)); 
      debugFile.println();
    #endif //ifdebug
  
    digitalWrite(greenLEDpin, LOW);
  
    // Now we write data to disk! Don't sync too often - requires 2048 bytes of I/O to SD card
    // which uses a bunch of power and takes time
    //if ((m - syncTime+5) < SYNC_INTERVAL) return;
    syncTime = m;
    #if ECHO_TO_SERIAL
      Serial.println("writing to SD card");
    #endif
    #if IF_DEBUG
      debugFile.println("writing to SD card");
    #endif
  
    // blink LED to show we are syncing data to the card & updating FAT!
    digitalWrite(redLEDpin, HIGH);
    #if IF_LOGGER
      logfile.flush();
    #endif
    #if IF_DEBUG
      debugFile.flush();
    #endif
    writeSDTime(now);
    delay(100);
    digitalWrite(redLEDpin, LOW);
  #endif //INCLUDE_SD
    
  delay_sec(DELAY_SEC);
}

/*=====================================END LOOP=======================================*/

void envControl(){
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  h_A = dht_A.readHumidity();
  #if ECHO_TO_SERIAL
    Serial.print(F("h_A (Gh) = "));
    Serial.println(h_A);
  #endif
  #if IF_DEBUG
    debugFile.print(F("h_A (Gh) = "));
    debugFile.println(h_A);
  #endif
  // Read temperature as Celsius (the default)
  cTemp_A = dht_A.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  fTemp_A = dht_A.readTemperature(true);
  #if ECHO_TO_SERIAL
    Serial.print(F("fTemp_A (Gt) = "));
    Serial.println(fTemp_A);
  #endif
  #if IF_DEBUG
    debugFile.print(F("fTemp_A (Gt) = "));
    debugFile.println(fTemp_A);
  #endif
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  h_B = dht_B.readHumidity();
  #if ECHO_TO_SERIAL
    Serial.print(F("h_B (BCh) = "));
    Serial.println(h_B);
  #endif
  #if IF_DEBUG
    debugFile.print(F("h_B (BCh) = "));
    debugFile.println(h_B);
  #endif
  // Read temperature as Celsius (the default)
  cTemp_B = dht_B.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  fTemp_B = dht_B.readTemperature(true);
  #if ECHO_TO_SERIAL
    Serial.print(F("fTemp_B (BCt) = "));
    Serial.println(fTemp_B);
  #endif
  #if IF_DEBUG
    debugFile.print(F("fTemp_B (BCt) = "));
    debugFile.println(fTemp_B);
  #endif
  
  // Check if any reads failed and exit early (to try again).
  if (isnan(h_B) || isnan(cTemp_B) || isnan(fTemp_B)) {
    #if ECHO_TO_SERIAL
      Serial.println("Failed to read from DHT sensor!");
    #endif
    #if IF_DEBUG
      debugFile.println("Failed to read from DHT sensor!");
    #endif

    //*********** TURN OFF ALL EQUIPMENT UNTIL READINGS ARE MADE AGAIN *************
    heatOn(false);
    exhaustOn(false);
    humidifierOn(false);

    digitalWrite(relayPin8, LOW);
    delay_sec(DELAY_SEC);
    #if ECHO_TO_SERIAL
      Serial.println("");
    #endif
    #if IF_DEBUG
      debugFile.println("");
    #endif
    digitalWrite(relayPin8, HIGH);
    return; //end env control method and return to loop
  }

 // Control section
    if(heatOnState==false && fTemp_B<heatingTempLowThreshold){
     #if ECHO_TO_SERIAL
       Serial.println("Please turn heat on");
     #endif
     #if IF_DEBUG
       debugFile.println("Please turn heat on");
     #endif
     heatOn(true);
  } else if(heatOnState==true && fTemp_B>heatingTempHighThreshold){
     #if ECHO_TO_SERIAL
       Serial.println("Please turn heat off");
     #endif
     #if IF_DEBUG
       debugFile.println("Please turn heat off");
     #endif
     heatOn(false);
  }

  if(humidifierOnState==false && h_B<humidityLowThreshold){
     #if ECHO_TO_SERIAL
       Serial.println("Please turn humidifier on");
     #endif
     #if IF_DEBUG
       debugFile.println("Please turn humidifier on");
     #endif
     humidifierOn(true);
  } else if(humidifierOnState==true && h_B>humidityHighThreshold){
     #if ECHO_TO_SERIAL
       Serial.println("Please turn humidifer off");
     #endif
     #if IF_DEBUG
       debugFile.println("Please turn humidifer off");
     #endif
     humidifierOn(false);
  }

    if(exhaustOnState==false && fTemp_B>coolingTempHighThreshold){
     #if ECHO_TO_SERIAL
       Serial.println("Please turn exhaust on");
     #endif
     #if IF_DEBUG
       debugFile.println("Please turn exhaust on");
     #endif
     exhaustOn(true);
  } else if(exhaustOnState==true && fTemp_B<coolingTempLowThreshold){
     #if ECHO_TO_SERIAL
       Serial.println("Please turn exhaust off");
     #endif
     #if IF_DEBUG
       debugFile.println("Please turn exhaust off");
     #endif
     exhaustOn(false);
  }
}

void heatOn(boolean on){
  int value = (on ? LOW : HIGH); // TRUE energizes the relay and lights the LED, FALSE de-energizes the relay and turns off the LED
  digitalWrite(relayPin3, value);   
  digitalWrite(relayPin4, value);
  heatOnState=on;
  #if ECHO_TO_SERIAL
       Serial.print("Heat is now (1 is on): ");
       Serial.println(on);
  #endif
  #if IF_DEBUG
       debugFile.print("Heat is now (1 is on): ");
       debugFile.println(on);
  #endif
}

void exhaustOn(boolean on){
  int value = (on ? LOW : HIGH); // TRUE energizes the relay and lights the LED, FALSE de-energizes the relay and turns off the LED
  digitalWrite(relayPin1, value);
  digitalWrite(relayPin2, value);
  exhaustOnState=on;
  #if ECHO_TO_SERIAL
       Serial.print("Exhaust is now (1 is on): ");
       Serial.println(on);
  #endif
  #if IF_DEBUG
       debugFile.print("Exhaust is now (1 is on): ");
       debugFile.println(on);
  #endif
}

void humidifierOn(boolean on){
  int value = (on ? LOW : HIGH); // TRUE energizes the relay and lights the LED, FALSE de-energizes the relay and turns off the LED
  digitalWrite(relayPin9, value);
  humidifierOnState=on;
  #if ECHO_TO_SERIAL
       Serial.print("Humidifer is now (1 is on): ");
       Serial.println(on);
  #endif
  #if IF_DEBUG
       debugFile.print("Humidifer is now (1 is on): ");
       debugFile.println(on);
  #endif
}

void delay_sec(int seconds){
  int sec=seconds;
  while (sec>0){
    delay(1000);
    sec--;
    #if ECHO_TO_SERIAL
      Serial.print(sec);
      Serial.print(" ");
    #endif
    #if IF_DEBUG
      debugFile.print(sec);
      debugFile.print(" ");
    #endif 
  }
}

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
      delay(DELAY_SEC*1000-500);
  #endif
}

void readAndSetTime(String DebugInitialText){
#if INCLUDE_SD
 char character;
 String settingName;
 String settingValue;
 int year;
 int month;
 int day;
 int hour;
 int minute;
 int second;
 
File myFile = SD.open("time.txt");
 if (myFile) {
  Serial.println("time.txt parameter settings:");
  DebugInitialText += "time.txt parameter settings:\n";
   while (myFile.available()) {
     character = myFile.read();
     while((myFile.available()) && (character != '[')){ //loop until open bracket found
      character = myFile.read();
     }
     character = myFile.read(); //read next char aftet open bracker
     while((myFile.available()) && (character != '=')){ //loop until = is found
       settingName = settingName + character; //build settingName one char at a time
       character = myFile.read();
     }
     character = myFile.read(); //read next char after =
     while((myFile.available()) && (character != ']')){ // loop until close bracket is found
       settingValue = settingValue + character; //build settingValue one char at a time
       character = myFile.read();
     }
     if(character == ']'){ //end of paramter string reached
       
       // Apply the value to the parameter
       if(settingName=="year"){
          year=settingValue.toInt();
          #if ECHO_TO_SERIAL
            Serial.print("year: ");
            Serial.println(year);
          #endif
          #if IF_DEBUG
            DebugInitialText += "year: ";
            DebugInitialText += year;
          #endif
       }

        if(settingName=="month"){
          month=settingValue.toInt();
          #if ECHO_TO_SERIAL
            Serial.print("month: ");
            Serial.println(month);
          #endif
          #if IF_DEBUG
            DebugInitialText += "month: ";
            DebugInitialText += month;
          #endif
        }

        if(settingName=="day"){
          day=settingValue.toInt();
          #if ECHO_TO_SERIAL
            Serial.print("day: ");
            Serial.println(day);
          #endif
          #if IF_DEBUG
            DebugInitialText += "day: ";
            DebugInitialText += day;
          #endif
        }

        if(settingName=="hour"){
          hour=settingValue.toInt();
          #if ECHO_TO_SERIAL
            Serial.print("hour: ");
            Serial.println(hour);
          #endif
          #if IF_DEBUG
            DebugInitialText += "hour: ";
            DebugInitialText += hour;
          #endif
        }

        if(settingName=="minute"){
          minute=settingValue.toInt();
          #if ECHO_TO_SERIAL
            Serial.print("minute: ");
            Serial.println(minute);
          #endif
          #if IF_DEBUG
            DebugInitialText += "minute: ";
            DebugInitialText += minute;
          #endif
        }

        if(settingName=="second"){
          second=settingValue.toInt();
          #if ECHO_TO_SERIAL
            Serial.print("second: ");
            Serial.println(second);
          #endif
          #if IF_DEBUG
            DebugInitialText += "second: ";
            DebugInitialText += second;
          #endif
        }
       }
       
       // Reset Strings
       settingName = "";
       settingValue = "";
   }
   // close the file:
   myFile.close();
 } else {
   // if the file didn't open, print an error:
    #if ECHO_TO_SERIAL
      Serial.println("error opening time.txt");
    #endif
    #if IF_DEBUG
      DebugInitialText += "error opening time.txt\n";
    #endif
 }

  #if ECHO_TO_SERIAL
    Serial.println("Adjusting RTC Sensor");
  #endif
  #if IF_DEBUG
    DebugInitialText += "Adjusting RTC Sensor\n";
  #endif

  DateTime compileTime = DateTime(F(__DATE__), F(__TIME__));
  DateTime fileTime = DateTime(year, month, day, hour, minute, second);

  boolean useFileTime = compileTime.unixtime() <= fileTime.unixtime();

  #if ECHO_TO_SERIAL
    Serial.print("Compile <= file time (1 is true and means filetime is used): ");
    Serial.println(useFileTime);
  #endif
  #if IF_DEBUG
    DebugInitialText += "Compile <= file time (1 is true and means filetime is used): \n";
   DebugInitialText+= useFileTime + "\n";
  #endif
  
  if(useFileTime){ // determine if filetime is most recent
    RTC.adjust(fileTime); //sets RTC to time of file
  } else { // compile time is most recent
    RTC.adjust(compileTime); //sets RTC to time of compile
  }
  
  

#endif //includeSD
}

void writeSDTime(DateTime now) {
  #if ECHO_TO_SERIAL
    Serial.print("[");
    Serial.print("year=");
    Serial.print(now.year(), DEC);
    Serial.println("]");
    Serial.print("[");
    Serial.print("month=");
    Serial.print(now.month(), DEC);
    Serial.println("]");
    Serial.print("[");
    Serial.print("day=");
    Serial.print(now.day(), DEC);
    Serial.println("]");
    Serial.print("[");
    Serial.print("hour=");
    Serial.print(now.hour(), DEC);
    Serial.println("]");
    Serial.print("[");
    Serial.print("minute=");
    Serial.print(now.minute(), DEC);
    Serial.println("]");
    Serial.print("[");
    Serial.print("second=");
    Serial.print(now.second(), DEC);
    Serial.println("]");
  #endif
  #if IF_DEBUG
    debugFile.print("[");
    debugFile.print("year=");
    debugFile.print(now.year(), DEC);
    debugFile.println("]");
    debugFile.print("[");
    debugFile.print("month=");
    debugFile.print(now.month(), DEC);
    debugFile.println("]");
    debugFile.print("[");
    debugFile.print("day=");
    debugFile.print(now.day(), DEC);
    debugFile.println("]");
    debugFile.print("[");
    debugFile.print("hour=");
    debugFile.print(now.hour(), DEC);
    debugFile.println("]");
    debugFile.print("[");
    debugFile.print("minute=");
    debugFile.print(now.minute(), DEC);
    debugFile.println("]");
    debugFile.print("[");
    debugFile.print("second=");
    debugFile.print(now.second(), DEC);
    debugFile.println("]");
  #endif
  
  #if INCLUDE_SD
     // Delete the old One
     SD.remove("time.txt");
     // Create new one
     File myFile = SD.open("time.txt", FILE_WRITE);
     // writing in the file works just like regular print()/println() function
     myFile.print("[");
     myFile.print("year=");
     myFile.print(now.year(), DEC);
     myFile.println("]");
     myFile.print("[");
     myFile.print("month=");
     myFile.print(now.month(), DEC);
     myFile.println("]");
     myFile.print("[");
     myFile.print("day=");
     myFile.print(now.day(), DEC);
     myFile.println("]");
     myFile.print("[");
     myFile.print("hour=");
     myFile.print(now.hour(), DEC);
     myFile.println("]");
     myFile.print("[");
     myFile.print("minute=");
     myFile.print(now.minute(), DEC);
     myFile.println("]");
     myFile.print("[");
     myFile.print("second=");
     myFile.print(now.second(), DEC);
     myFile.println("]");
     // close the file:
     myFile.close();
   #endif //includeSD
 }
