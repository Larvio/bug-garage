#include <DHT.h> //Needed for Sensors

//***** GENERAL CONFIG & DEBUG TOOLS *********
#define ECHO_TO_SERIAL 0 // echo data to serial port
#define WAIT_TO_START 0 // Wait for serial input in setup()
#define DELAY_SEC 60 // seconds delay between measurements

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
#define relayPin8 45 // not hooked up
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
    while (!Serial.available());
  #endif //WAIT_TO_START
  
  // initialize the Sensor
  #if ECHO_TO_SERIAL
    Serial.println("Initializing Sensor...");
  #endif
  dht_A.begin(); //initialize the sensor
  dht_B.begin(); //initialize the sensor
}




/*=====================================   START LOOP    =======================================*/
void loop() {
  #if ECHO_TO_SERIAL
    Serial.println(F("Starting loop..."));
  #endif
  
  //******************MAKE SURE BUGS ARE OK ***********
  envControl(); 
  //******** I HOPE THEY'RE HAVING FUN IN THERE ************

    #if ECHO_TO_SERIAL
      Serial.print(millis()/1000, DEC);
      Serial.print(", ");   
      Serial.print(fTemp_A);
      Serial.print(", "); 
      Serial.print(cTemp_A);
      Serial.print(", "); 
      Serial.print(h_A);
      Serial.print(",");
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
    #endif //ECHO_TO_SERIAL
    
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
  // Read temperature as Celsius (the default)
  cTemp_A = dht_A.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  fTemp_A = dht_A.readTemperature(true);
  #if ECHO_TO_SERIAL
    Serial.print(F("fTemp_A (Gt) = "));
    Serial.println(fTemp_A);
  #endif
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  h_B = dht_B.readHumidity();
  #if ECHO_TO_SERIAL
    Serial.print(F("h_B (BCh) = "));
    Serial.println(h_B);
  #endif
  // Read temperature as Celsius (the default)
  cTemp_B = dht_B.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  fTemp_B = dht_B.readTemperature(true);
  #if ECHO_TO_SERIAL
    Serial.print(F("fTemp_B (BCt) = "));
    Serial.println(fTemp_B);
  #endif
  
  // Check if any reads failed and exit early (to try again).
  if (isnan(h_B) || isnan(cTemp_B) || isnan(fTemp_B)) {
    #if ECHO_TO_SERIAL
      Serial.println("Failed to read from DHT sensor!");
    #endif

    //*********** TURN OFF ALL EQUIPMENT UNTIL READINGS ARE MADE AGAIN *************
    heatOn(false);
    exhaustOn(false);
    humidifierOn(false);
    
    return; //restart loop
  }

 // Control section
    if(heatOnState==false && fTemp_B<heatingTempLowThreshold){
     heatOn(true);
  } else if(heatOnState==true && fTemp_B>heatingTempHighThreshold){
     heatOn(false);
  }

  if(humidifierOnState==false && h_B<humidityLowThreshold){
     humidifierOn(true);
  } else if(humidifierOnState==true && h_B>humidityHighThreshold){
     humidifierOn(false);
  }

    if(exhaustOnState==false && fTemp_B>coolingTempHighThreshold){
     exhaustOn(true);
  } else if(exhaustOnState==true && fTemp_B<coolingTempLowThreshold){
     exhaustOn(false);
  }
}

void heatOn(boolean on){
  int value = (on ? LOW : HIGH); // TRUE energizes the relay and lights the LED, FALSE de-energizes the relay and turns off the LED
  digitalWrite(relayPin3, value);   
  digitalWrite(relayPin4, value);
  heatOnState=on;
}

void exhaustOn(boolean on){
  int value = (on ? LOW : HIGH); // TRUE energizes the relay and lights the LED, FALSE de-energizes the relay and turns off the LED
  digitalWrite(relayPin1, value);
  digitalWrite(relayPin2, value);
  exhaustOnState=on;
}

void humidifierOn(boolean on){
  int value = (on ? LOW : HIGH); // TRUE energizes the relay and lights the LED, FALSE de-energizes the relay and turns off the LED
  digitalWrite(relayPin9, value);
  humidifierOnState=on;
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
  }
}

