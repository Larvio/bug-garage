#include "Adafruit_FONA.h"
#include <SoftwareSerial.h>

#define DELAY_SEC 50

/* FONA PINS */
#define FONA_RX A15
#define FONA_TX A14
#define FONA_RST 10

/*TING APN INFORMATION*/
#define APN "wholesale"
#define USER ""
#define PASS ""

/* UBIDOTS variables */
#define TOKEN "3hB2gtSrMKDaZKyPkRt6Egqh7Lft8c"
#define ID1 "57f1d28176254242f42ef05c" //breeding chamber temp
#define ID2 "57fc51d4762542630edea20e" //breeding chamber humidity
#define ID3 "57fc51df7625426342b9e83d" //garage temperature
#define ID4 "57fc51e57625426342b9e85e" //garage humidity
#define ID5 "57fe1c3676254202cb373d0c" //battery percentage
#define ID6 "57fef5d0762542605d3e8531"  //Fan Status
#define ID7 "57fef5c07625425fc1403f8d"  //Heater Status
#define ID8 "57fef5c7762542603c7383bf"  //Cooling Status
#define ID9 ""
#define ID10 ""
