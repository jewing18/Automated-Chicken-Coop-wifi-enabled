//FUTURE ADDITIONS: HEATERS

#include <OneWire.h>                   // load the onewire library for thermometer
#include <DallasTemperature.h>         //for thermometer reading
#include <WiFiEspAT.h>                 //for wifi connection
#include <Time.h>                      //for some time management
#include <Dusk2Dawn.h>                 //for calculating sunset/rise time
#include <NTPClient.h>                 //for getting current time from internet
#include <WiFiUdp.h>                   //for more wifi stuff


#define BLYNK_PRINT Serial                      //Enables Blynk Serial Monitor
#define BLYNK_TEMPLATE_ID   "TMPLZ3LtXhEe"      //Input Blynk Template ID Here
#include <BlynkSimpleWifi.h>                    //for Blynk App connection

//Thermocouple data wire is plugged into digital pin 50 on the Arduino
#define ONE_WIRE_BUS 50

#include "arduino_secrets.h"
///////please enter your sensitive data in the Secret tab/arduino_secrets.h
const char ssid[] = SECRET_SSID;                    // your network SSID (name) taken from 'arduino_secrets.h'
const char pass[] = SECRET_PASS;                    // your network password taken from 'arduino_secrets.h'
char auth[] = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";   // You should get Auth Token from the Blynk App.

//*************CONSTANT INPUTS HERE*****************//
const boolean SerialDisplay = true;    // print debug messages or not to serial

const int coopDoorSunriseDelay = 0;    //Specify minutes past sunrise for coop door to open (can be negative)
const int runDoorSunriseDelay = 0;     //Specify minutes past sunrise for run door to open (can be negative)
const int coopDoorSunsetDelay = 0;    //Specify minutes past sunset for coop door to close (can be negative)
const int runDoorSunsetDelay = 0;     //Specify minutes past sunset for run door to close (can be negative)

const float longitude = 40.035058;     //Your Longitude here
const float latitude = -83.841127;     //Your Latitude here
const int DST_UTCoffset = -4;          //Your hour offset from UTC time during daylight savings (U.S. eastern shown)
const int standard_UTCoffset = -5;     //Your hour offset from UTC time NOT during daylight savings (U.S. eastern shown)

double feedRate = 10.0;                 //tested value (lb/min) - Auto Feeder shoots out xx lb/min
int feedingTime = 1020;                 //Feeding time in mins past midnight 

// Please don't send more that 10 values per second.
const int chickTempDelay = 1000;       //Send Chick temp to Blynk app every 1 second
const int coopTempDelay = 1000;        //Send Coop temp to Blynk app every 1 second
const int coopDoorStatusDelay = 1000;  //Send Coop Door Status to Blynk app every 1 second
const int dailyFeedDelay = 1000;       //Send daily feed status to Blynk every 1 second.
unsigned long timePingDelay = 5000;    // Ping current time from server every 5 seconds
//*************************************************//

// Setup a oneWire instance to communicate with thermocouple OneWire device
OneWire oneWire(ONE_WIRE_BUS);
// Pass oneWire reference to DallasTemperature library
DallasTemperature sensors(&oneWire);

//Start Blynktimer so as not to send/receive too much data to Blynk
BlynkTimer timer;

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

//Declare Dusk2Dawn timing variables
unsigned long epochTime;
int currentMinute;
int currentHour;
int monthDay;
int currentMonth;
int currentYear;
int DST;
int OhioSunrise;
int OhioSunset;
int currentTime;

//Declare time pinging variable for timezone
int UTCoffset;

//Define Week Days & Month Names
String weekDays[7] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
String months[12] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};

// pins assignments
const int enableCoopDoorMotorA = 10;          // enable motor a - pin 7
const int directionCloseCoopDoorMotorA = 9;  // direction close motor a - pin 8
const int directionOpenCoopDoorMotorA = 8;   // direction open motor a - pin 9
const int coopBottomSwitchPin = 26;              // bottom switch is connected to pin 26
const int coopTopSwitchPin = 27;                 // top switch is connected to pin 27

const int enableRunDoorMotorB = 5;          // enable motor b - pin 4
const int directionCloseRunDoorMotorB = 6;  // direction close motor b - pin 5
const int directionOpenRunDoorMotorB = 7;   // direction open motor b - pin 6
const int runBottomSwitchPin = 34;              // bottom switch is connected to pin 34
const int runTopSwitchPin = 35;                 // top switch is connected to pin 35

const int enableFeederMotorC = 4;          // enable feeder motor - pin 4
const int directionCloseFeederMotorC = 3;  // direction close feeder motor - pin 3
const int directionOpenFeederMotorC = 2;   // direction open feeder motor - pin 2

// VARIABLES
// Time Gathering Variables
unsigned long lastPingedTime = 0;           // Last Pinged Time from NTP Server

// reed switches top and bottom of Doors
// Coop top switch
int coopTopSwitchPinVal;                   // top switch var for reading the pin status
int coopTopSwitchPinVal2;                  // top switch var for reading the pin delay/debounce status
int coopTopSwitchState;                    // top switch var for to hold the switch state

// Coop bottom switch
int coopBottomSwitchPinVal;                // bottom switch var for reading the pin status
int coopBottomSwitchPinVal2;               // bottom switch var for reading the pin delay/debounce status
int coopBottomSwitchState;                 // bottom switch var for to hold the switch state

// Run top switch
int runTopSwitchPinVal;                   // top switch var for reading the pin status
int runTopSwitchPinVal2;                  // top switch var for reading the pin delay/debounce status
int runTopSwitchState;                    // top switch var for to hold the switch state

// Run bottom switch
int runBottomSwitchPinVal;                // bottom switch var for reading the pin status
int runBottomSwitchPinVal2;               // bottom switch var for reading the pin delay/debounce status
int runBottomSwitchState;                 // bottom switch var for to hold the switch state


// Top/Bottom Switch Debounce Delay
unsigned long lastCTSDebounceTime = 0;    //CTS = "Coop Top Switch"
unsigned long lastCBSDebounceTime = 0;    //CBS = "Coop Bottom Switch"
unsigned long lastRTSDebounceTime = 0;    //RTS = "Run Top Switch"
unsigned long lastRBSDebounceTime = 0;    //RBS = "Run Bottom Switch"
unsigned long debounceDelay = 100;        //100ms delay for Debouncing switches

// temperature check
int deviceCount;                             // Initialize thermocouple Device Number
float tempC;                                 // Initialize debug-only temp checker
float ChickTempC;                            // Initialize Chick temperature C number
float CoopTempC;                             // Initialize Coop temperature C number
float ChickTempF;                            // Initialize Chick temperature F number
float CoopTempF;                             // Initialize Coop temperature F number

//AutoFeeder Variables
unsigned long lastFeederCheck = 0;              //initialize feed rate variable
bool dailyFeederTracker = false;                //Boolean to track if they have been fed today
double feedAmount;                              //Decimal value tracking feed qty from slider widget in Blynk
int manualFeedStatus;                           //Basically a bool (0 or 1) for the manual feeder button widget
unsigned long feederMotorRuntime;               //initialize variable to return millisecs of motor runtime (equation later in code)



// ************************************** the setup **************************************
void setup(void) {

  Serial.begin(115200);                    // initialize serial port hardware

  sensors.setWaitForConversion(false);     /*sensors.requestTemperatures(); is super slow! We dont wanna wait on it to
                                             return temp values, so they make this fancy little setting to speed it up*/
  sensors.begin();                         // Start up the Thermocouple library

  deviceCount = sensors.getDeviceCount();  //Locate and count the amount of Thermocouples on the single data-wire chain

  if (SerialDisplay) {
    Serial.print("Locating devices...");
    Serial.print("Found ");
    Serial.print(deviceCount, DEC);
    Serial.println(" devices.");
    Serial.println("");
  }

  // Coop door
  // coop door motor
  pinMode (enableCoopDoorMotorA, OUTPUT);           // enable motor pin = output
  pinMode (directionCloseCoopDoorMotorA, OUTPUT);   // motor close direction pin = output
  pinMode (directionOpenCoopDoorMotorA, OUTPUT);    // motor open direction pin = output

  // coop bottom switch
  pinMode(coopBottomSwitchPin, INPUT);                  // set bottom switch pin as input
  digitalWrite(coopBottomSwitchPin, HIGH);              // activate bottom switch resistor

  // coop top switch
  pinMode(coopTopSwitchPin, INPUT);                     // set top switch pin as input
  digitalWrite(coopTopSwitchPin, HIGH);                 // activate top switch resistor


  // Run door
  // Run door motor
  pinMode (enableRunDoorMotorB, OUTPUT);           // enable motor pin = output
  pinMode (directionCloseRunDoorMotorB, OUTPUT);   // motor close direction pin = output
  pinMode (directionOpenRunDoorMotorB, OUTPUT);    // motor open direction pin = output

  // run bottom switch
  pinMode(runBottomSwitchPin, INPUT);                  // set bottom switch pin as input
  digitalWrite(runBottomSwitchPin, HIGH);              // activate bottom switch resistor

  // run top switch
  pinMode(runTopSwitchPin, INPUT);                     // set top switch pin as input
  digitalWrite(runTopSwitchPin, HIGH);                 // activate top switch resistor

  // Auto-Feeder Motor
  pinMode (enableFeederMotorC, OUTPUT);           // enable motor pin = output
  pinMode (directionCloseFeederMotorC, OUTPUT);   // motor close direction pin = output
  pinMode (directionOpenFeederMotorC, OUTPUT);    // motor open direction pin = output
}

// ************************************** functions **************************************

void WifiCheck() {                          //Checks if wifi is actually connected, if not, attempt to reconnect
  if (WiFi.status() != WL_CONNECTED) {
    connectWifi();
  }
}


void connectWifi() {                        //Connects to Wifi and then to Blynk App
  //---PERSISTENT WIFI SETUP---

  /*This portion connects to a WiFi network and sets the AT firmware
    to remember this WiFi network and auto-connect to it at startup.*/
  Serial3.begin(115200);            //Initialize Wifi Chip Communication which is on Serial 3
  WiFi.init(Serial3);               //Initialize Wifi Connection

  if (WiFi.status() == WL_NO_MODULE) {   //Check if there is good communication between arduino and ESP8266
    if (SerialDisplay) {
      Serial.println();
      Serial.println("Communication with WiFi module failed!");
    }
    // don't continue
    while (true);
  }

  WiFi.disconnect();        // to clear the way. not persistent
  WiFi.setPersistent();     // set the following WiFi connection as persistent
  WiFi.endAP();             // to disable default automatic start of persistent AP at startup

  if (SerialDisplay) {
    Serial.println();
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
  }

  int status = WiFi.begin(ssid, pass);

  //Check if Wifi is actually connected
  if (status == WL_CONNECTED) {
    if (SerialDisplay) {
      Serial.println();
      Serial.println("Connected to WiFi network.");
      printWifiStatus();
    }
  } else {
    WiFi.disconnect(); // remove the WiFi connection
    if (SerialDisplay) {
      Serial.println();
      Serial.println("Connection to WiFi network failed.");
    }
  }


  //Blynk App Setup
  Blynk.begin(auth, ssid, pass);  //Begin Blynk App Communications

  // Setup a Blynk function to be called every set inteval.
  // This is where you will set the datastream send/receive DELAY to/from Blynk
  // Please don't send more than 10 values per second.
  timer.setInterval(chickTempDelay, ChickTemp); //Run Blynk function ChickTemp every __ sec (Blynk-specific functions located below Loop)
  timer.setInterval(coopTempDelay, CoopTemp); //Run Blynk function CoopTemp every __ sec (Blynk-specific functions located below Loop)
  timer.setInterval(coopDoorStatusDelay, SendDoorStatuses); //Run Blynk function CoopTemp every __ sec (Blynk-specific functions located below Loop)
  timer.setInterval(dailyFeedDelay, dailyFeedStatus); //Run Blynk function dailyFeedStatus every __ sec (Blynk-specific functions located below Loop)

}


void printWifiStatus() {
  // print the SSID of the network you're attached to:
  char ssid[33];
  WiFi.SSID(ssid);
  Serial.print("SSID: ");
  Serial.println(ssid);


  // print the BSSID of the network you're attached to:
  uint8_t bssid[6];
  WiFi.BSSID(bssid);
  if (SerialDisplay) {
    Serial.print("BSSID: ");
    printMacAddress(bssid);


    uint8_t mac[6];
    WiFi.macAddress(mac);
    Serial.print("MAC: ");
    printMacAddress(mac);

    // print your board's IP address:
    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);

    // print the received signal strength:
    long rssi = WiFi.RSSI();
    Serial.print("signal strength (RSSI):");
    Serial.print(rssi);
    Serial.println(" dBm");
  }
}

void printMacAddress(byte mac[]) {  //Prints your MAC Address
  for (int i = 5; i >= 0; i--) {
    if (mac[i] < 16) {
      Serial.print("0");
    }
    Serial.print(mac[i], HEX);
    if (i > 0) {
      Serial.print(":");
    }
  }
  Serial.println();
}


void GetTemps() {         //Gets temperatures from all Thermocouple connected to the thermocouple data wire
  sensors.requestTemperatures();

  //Thermocouples are identified by Index starting with 0, 1...
  ChickTempC = sensors.getTempCByIndex(0);  //Gets temp in C from first thermocouple
  CoopTempC = sensors.getTempCByIndex(1);   //Gets temp in C from second thermocouple

  ChickTempF = DallasTemperature::toFahrenheit(ChickTempC); //Converts first thermocouple signal to Fahrenheit
  CoopTempF = DallasTemperature::toFahrenheit(CoopTempC);   //Converts second thermocouple signal to Fahrenheit

  if (SerialDisplay) {             // Display temperature from each sensor
    for (int i = 0;  i < deviceCount;  i++)
    {
      Serial.print("Sensor ");
      Serial.print(i + 1);
      Serial.print(" : ");
      tempC = sensors.getTempCByIndex(i);
      Serial.print(tempC);
      Serial.print("\xc2\xb0");//shows degrees character
      Serial.print("C  |  ");
      Serial.print(DallasTemperature::toFahrenheit(tempC));
      Serial.print("\xc2\xb0");//shows degrees character
      Serial.println("F");
    }
  }
}


void GetTime() {  //Gathers the current time from the NTP Server

  // NTPClient Setup to get accurate time from the NTP Server
  timeClient.begin();

  //This portion is run with standard UTC offset in order to get accurate month in order to determine IF we are in DST or not.
  //Theoretically running this first could allow for an inaccurate time count 1 hr before 
  //    or after DST switchover, but its at 2am, so who cares...
  // Set offset time in SECONDS to adjust for your timezone, for example:
  // GMT +1 = 3600  ||  GMT +8 = 28800  ||  GMT -1 = -3600 ||  GMT 0 = 0
  timeClient.setTimeOffset(standard_UTCoffset*3600); //initial set @ USA Eastern Time (DST)
  //for US Eastern time, if DST=false, we are in UDP-5 (-18000), but during DST, it is UPD-4(-14400)

  timeClient.update(); //Gets current time

  // This Function determines whether Daylight Savings time is in effect or not based on above determined time
  // Calculate offset for Sunday
  int y = currentYear - 2000;       // Get year from RTC and subtract 2000
  int x = (y + y / 4 + 2) % 7;      /* remainder will identify which day of month is Sunday by subtracting x from the one
                                      or two week window. First two weeks for March and first week for November*/
                                      
  // Test DST: BEGINS on 2nd Sunday of March @ 2:00 AM
  if (currentMonth == 3 && monthDay == (14 - x) && currentHour >= 2) {
    DST = true; // Daylight Savings Time is TRUE (add one hour)
    UTCoffset = DST_UTCoffset;
  }
  if (currentMonth == 3 && monthDay > (14 - x) || currentHour > 3) {
    DST = true;
    UTCoffset = DST_UTCoffset;
  }
  // Test DST: ENDS on 1st Sunday of Nov @ 2:00 AM
  if (currentMonth == 11 && monthDay == (7 - x) && currentHour >= 2) {
    DST = false; // Daylight Savings Time is FALSE (standard time)
    UTCoffset = standard_UTCoffset;
  }
  if (currentMonth == 11 && monthDay > (7 - x) || currentMonth > 11 || currentMonth < 3) {
    DST = false;
    UTCoffset = standard_UTCoffset;
  }

  // Now we run this AGAIN to get accurate time including DST status. 
  // Set offset time in SECONDS to adjust for your timezone, for example:
  // GMT +1 = 3600  ||  GMT +8 = 28800  ||  GMT -1 = -3600 ||  GMT 0 = 0
  timeClient.setTimeOffset(UTCoffset*3600); //Set USA Eastern Time
  //for US Eastern time, if DST=false, we are in UDP-5 (-18000), but during DST, it is UPD-4(-14400)

  timeClient.update(); //Gets current time
  
  epochTime = timeClient.getEpochTime();          //determine the Epoch Time

  //Make a time structure
  struct tm *ptm = gmtime ((time_t *)&epochTime);

  //Defines useful variables for future use
  currentHour = timeClient.getHours();            //determine current Hour
  currentMinute = timeClient.getMinutes();        //determine current Minute
  monthDay = ptm->tm_mday;                        //determine the Day of the month
  currentMonth = ptm->tm_mon + 1;                 //determine the Month Number
  currentYear = ptm->tm_year + 1870;              //determine the current year -NOT SURE WHY THIS NEEDED TO BE 1870....SHOULD BE 1900??

  //Setup Dusk2Dawn to determine today's sunset & sunrise
  Dusk2Dawn Ohio(longitude, latitude, UTCoffset);               //Arguments are longitude, latitude, and time zone offset in hours from UTC.

 
  //Determine the Sunrise and Sunset times - returns as "# OF MINUTES PAST MIDNIGHT"
  OhioSunrise  = Ohio.sunrise(currentYear, currentMonth, monthDay, DST);      //Pass D2D the year, month, day, & if DST is active
  OhioSunset   = Ohio.sunset(currentYear, currentMonth, monthDay, DST);       //Pass D2D the year, month, day, & if DST is active

  //Format and Print out today's sunrise and sunset times (24hr format)
  if (SerialDisplay) {
    char sunrise[6];
    Dusk2Dawn::min2str(sunrise, OhioSunrise);
    Serial.print("Today's Sunrise Time: ");
    Serial.print(sunrise);
    Serial.println(" AM");

    char sunset[6];
    Dusk2Dawn::min2str(sunset, OhioSunset);
    Serial.print("Today's Sunset Time: ");
    Serial.print(sunset);
    Serial.println(" PM");

    //Print current time and date for verifying internet connection
    String currentDate = String(currentMonth) + "-" + String(monthDay) + "-" + String(currentYear);
    Serial.print("Current date: ");
    Serial.println(currentDate);
    String formattedTime = timeClient.getFormattedTime();
    Serial.print("Current Time: ");
    Serial.println(formattedTime);
    Serial.print("Current UTC offset: ");
    Serial.print(UTCoffset);
    Serial.println("    [-4 for Mar to Nov. -5 for Nov to Mar]");
  }
}

// ***********Functions to operate the Coop Doors*************

void debounceCoopBottomReedSwitch() {   //debounce bottom reed switch

  //debounce bottom reed switch
  coopBottomSwitchPinVal = digitalRead(coopBottomSwitchPin);       // read input value and store it in val

  if ((millis() - lastCBSDebounceTime) > debounceDelay) {    // delay 10ms for consistent readings
    lastCBSDebounceTime = millis();
    coopBottomSwitchPinVal2 = digitalRead(coopBottomSwitchPin);    // read input value again to check or bounce

    if (coopBottomSwitchPinVal == coopBottomSwitchPinVal2) {       // make sure we have 2 consistant readings
      if (coopBottomSwitchPinVal != coopBottomSwitchState) {       // the switch state has changed!
        coopBottomSwitchState = coopBottomSwitchPinVal;
      }
      if (SerialDisplay) {
        Serial.print (" Coop Bottom Switch Value: ");           // display "Bottom Switch Value:"
        Serial.println(digitalRead(coopBottomSwitchPin));      // display current value of bottom switch;
      }
    }
  }
}


void debounceRunBottomReedSwitch() {   //debounce bottom reed switch

  //debounce bottom reed switch
  runBottomSwitchPinVal = digitalRead(runBottomSwitchPin);       // read input value and store it in val

  if ((millis() - lastRBSDebounceTime) > debounceDelay) {    // delay 10ms for consistent readings
    lastRBSDebounceTime = millis();
    runBottomSwitchPinVal2 = digitalRead(runBottomSwitchPin);    // read input value again to check or bounce

    if (runBottomSwitchPinVal == runBottomSwitchPinVal2) {       // make sure we have 2 consistant readings
      if (runBottomSwitchPinVal != runBottomSwitchState) {       // the switch state has changed!
        runBottomSwitchState = runBottomSwitchPinVal;
      }
      if (SerialDisplay) {
        Serial.print (" Run Bottom Switch Value: ");           // display "Bottom Switch Value:"
        Serial.println(digitalRead(runBottomSwitchPin));      // display current value of bottom switch;
      }
    }
  }
}


void debounceCoopTopReedSwitch() {// debounce top reed switch

  coopTopSwitchPinVal = digitalRead(coopTopSwitchPin);             // read input value and store it in val
  //  delay(10);

  if ((millis() - lastCTSDebounceTime) > debounceDelay) {     // delay 10ms for consistent readings
    lastCTSDebounceTime = millis();
    coopTopSwitchPinVal2 = digitalRead(coopTopSwitchPin);          // read input value again to check or bounce

    if (coopTopSwitchPinVal == coopTopSwitchPinVal2) {             // make sure we have 2 consistant readings
      if (coopTopSwitchPinVal != coopTopSwitchState) {             // the button state has changed!
        coopTopSwitchState = coopTopSwitchPinVal;
      }
      if (SerialDisplay) {
        Serial.print (" Coop Top Switch Value: ");              // display "Bottom Switch Value:"
        Serial.println(digitalRead(coopTopSwitchPin));         // display current value of bottom switch;
      }
    }
  }
}


void debounceRunTopReedSwitch() {// debounce top reed switch

  runTopSwitchPinVal = digitalRead(runTopSwitchPin);             // read input value and store it in val
  //  delay(10);

  if ((millis() - lastRTSDebounceTime) > debounceDelay) {     // delay 10ms for consistent readings
    lastRTSDebounceTime = millis();
    runTopSwitchPinVal2 = digitalRead(runTopSwitchPin);          // read input value again to check or bounce

    if (runTopSwitchPinVal == runTopSwitchPinVal2) {             // make sure we have 2 consistant readings
      if (runTopSwitchPinVal != runTopSwitchState) {             // the button state has changed!
        runTopSwitchState = runTopSwitchPinVal;
      }
      if (SerialDisplay) {
        Serial.print (" Run Top Switch Value: ");              // display "Bottom Switch Value:"
        Serial.println(digitalRead(runTopSwitchPin));         // display current value of bottom switch;
      }
    }
  }
}


void stopCoopDoorMotorA() { // stop the coop door motor
  digitalWrite (directionCloseCoopDoorMotorA, LOW);      // turn off motor close direction
  digitalWrite (directionOpenCoopDoorMotorA, LOW);       // turn on motor open direction
  analogWrite (enableCoopDoorMotorA, 0);                 // enable motor, 0 speed
}


void stopRunDoorMotorB() { // stop the coop door motor
  digitalWrite (directionCloseRunDoorMotorB, LOW);      // turn off motor close direction
  digitalWrite (directionOpenRunDoorMotorB, LOW);       // turn on motor open direction
  analogWrite (enableRunDoorMotorB, 0);                 // enable motor, 0 speed
}


void closeCoopDoorMotorA() {    // close the coop door motor (motor dir close = clockwise)
  digitalWrite (directionCloseCoopDoorMotorA, HIGH);     // turn on motor close direction
  digitalWrite (directionOpenCoopDoorMotorA, LOW);       // turn off motor open direction
  analogWrite (enableCoopDoorMotorA, 255);               // enable motor, full speed
  if (coopBottomSwitchPinVal == 0) {                         // if bottom reed switch circuit is closed
    stopCoopDoorMotorA();
    if (SerialDisplay) {
      //Serial.println(" Coop Door Closed - no danger");
    }
  }
}


void closeRunDoorMotorB() {    // close the coop door motor (motor dir close = clockwise)
  digitalWrite (directionCloseRunDoorMotorB, HIGH);     // turn on motor close direction
  digitalWrite (directionOpenRunDoorMotorB, LOW);       // turn off motor open direction
  analogWrite (enableRunDoorMotorB, 255);               // enable motor, full speed
  if (runBottomSwitchPinVal == 0) {                         // if bottom reed switch circuit is closed
    stopRunDoorMotorB();
    if (SerialDisplay) {
      //Serial.println(" Coop Door Closed - no danger");
    }
  }
}


void openCoopDoorMotorA() {   // open the coop door (motor dir open = counter-clockwise)
  digitalWrite(directionCloseCoopDoorMotorA, LOW);       // turn off motor close direction
  digitalWrite(directionOpenCoopDoorMotorA, HIGH);       // turn on motor open direction
  analogWrite(enableCoopDoorMotorA, 255);                // enable motor, full speed
  if (coopTopSwitchPinVal == 0) {                            // if top reed switch circuit is closed
    stopCoopDoorMotorA();
    if (SerialDisplay) {
      //Serial.println(" Coop Door open - danger!");
    }
  }
}


void openRunDoorMotorB() {   // open the coop door (motor dir open = counter-clockwise)
  digitalWrite(directionCloseRunDoorMotorB, LOW);       // turn off motor close direction
  digitalWrite(directionOpenRunDoorMotorB, HIGH);       // turn on motor open direction
  analogWrite(enableRunDoorMotorB, 255);                // enable motor, full speed
  if (runTopSwitchPinVal == 0) {                            // if top reed switch circuit is closed
    stopRunDoorMotorB();
    if (SerialDisplay) {
      //Serial.println(" Coop Door open - danger!");
    }
  }
}



// Coop Door Open/CLose Logic
void doCoopDoors() {

  if ((millis() - lastPingedTime) >= timePingDelay) {    // check time every "timePingDelay" set above
    GetTime();                                               //Go get the time
    lastPingedTime = millis();
  }
  currentTime = (currentHour * 60) + currentMinute;         //current time in 'minutes past midnight'
  //This is what will be used to evaluate against sunset/rise

  //currentTime = 1200;   //DEBUGGING ONLY: HARD-SET CURRENT TIME TO TEST MOTOR RUN LOGIC
  if (SerialDisplay) {
    Serial.print("Current Mins past Midnight: ");             //For debugging and Checking
    Serial.println(currentTime);
    Serial.println();

    Serial.print("Sunrise Mins past Midnight: ");             //For debugging and Checking
    Serial.println(OhioSunrise);
    Serial.println();

    Serial.print("Sunset Mins past Midnight: ");             //For debugging and Checking
    Serial.println(OhioSunset);
    Serial.println("----------------------");
  }

  //COOP DOOR
  if ((currentTime >= (OhioSunset + coopDoorSunsetDelay)) || (currentTime <= (OhioSunrise + coopDoorSunriseDelay))) { // if it's dark
    debounceCoopTopReedSwitch();                                  // read and debounce the switches
    debounceCoopBottomReedSwitch();
    closeCoopDoorMotorA();                                    // close the door
  }
  if ((currentTime < (OhioSunset + coopDoorSunsetDelay)) && (currentTime > OhioSunrise + coopDoorSunriseDelay)) { // if it's light
    debounceCoopTopReedSwitch();                                  // read and debounce the switches
    debounceCoopBottomReedSwitch();
    openCoopDoorMotorA();                                     // Open the door
  }

  //RUN DOOR
  if ((currentTime >= (OhioSunset + runDoorSunsetDelay)) || (currentTime <= (OhioSunrise + runDoorSunriseDelay))) { // if it's dark
    debounceRunTopReedSwitch();                                  // read and debounce the switches
    debounceRunBottomReedSwitch();
    closeRunDoorMotorB();                                    // close the door
  }
  if ((currentTime < (OhioSunset + runDoorSunsetDelay)) && (currentTime > (OhioSunrise + coopDoorSunriseDelay))) { // if it's light
    debounceRunTopReedSwitch();                                  // read and debounce the switches
    debounceRunBottomReedSwitch();
    openRunDoorMotorB();                                     // Open the door
  }
}

// ***********Functions to operate the Auto-Feeder*************
void startFeederMotorC() { //start the Feeder Motor
  digitalWrite(directionCloseFeederMotorC, LOW);       // turn off motor close direction
  digitalWrite(directionOpenFeederMotorC, HIGH);       // turn on motor open direction
  analogWrite(enableFeederMotorC, 255);     // enable motor, full speed
  Serial.println("STARTING FEEDER MOTOR!!!!!!!!!!");
}

void stopFeederMotorC() { // stop the Feeder Motor
  digitalWrite (directionCloseFeederMotorC, LOW);      // turn off motor close direction
  digitalWrite (directionOpenFeederMotorC, LOW);       // turn off motor open direction
  analogWrite (enableFeederMotorC, 0);                 // enable motor, 0 speed
}

void doChickenFeeder() { 
  feederMotorRuntime = feedAmount/(feedRate/60000);    //Calculation to determine how long the feeder screw will spin.
  
  if (SerialDisplay) {                        //Debugging Stuff
    Serial.println("---------------------------------------------");
    if(dailyFeederTracker == false){
      Serial.println("Chickens have NOT been fed today");
    }
    if(dailyFeederTracker == true){
      Serial.println("Chickens HAVE been fed today");
    }
    Serial.print("Feeding Time: ");
    Serial.println(feedingTime);
    Serial.print("Current Time: ");
    Serial.println(currentTime);
    Serial.print("millis: ");
    Serial.println(millis());
    Serial.print("lastFeederCheck: ");
    Serial.println(lastFeederCheck);
    Serial.print("Runtime of Auto Feeder (ms): ");
    Serial.println(feederMotorRuntime);
    Serial.print("Feed Amount (Slider Value) is: ");
    Serial.print(feedAmount);
    Serial.println(" lbs");
    Serial.print("Manual Feed Button Status is: ");
    Serial.println(manualFeedStatus);
    Serial.println("---------------------------------------------");  
  }
  
  if (currentTime >= feedingTime) {                              //if it is time
    if(dailyFeederTracker == false){                             //and if they havent been fed today already
      startFeederMotorC();                                       //start feed motor
      if (millis() - lastFeederCheck > feederMotorRuntime) {     //stop motor after proper feed amount reached
        lastFeederCheck = millis();
        stopFeederMotorC();                                      //stop feed motor
        dailyFeederTracker = true;                               //change the daily feed tracker to true
       }       
    }  
  }
  
  if(currentTime == 1){           //If it is 1 minute past midnight, change daily tracker status to false             
    dailyFeederTracker = false;
  }
 
}

// ************************************** the loop **************************************

void loop() {
  WifiCheck();        //This will check the wifi connection and if not connected, will keep trying
  GetTemps();         //Gather Temperature Info
  Blynk.run();        //Run Blynk communications
  timer.run();        //Initiate BlynkTimer so as not to send/receive too much data to Blynk at once
  doCoopDoors();      //Judge Coop Door Status and adjust as needed
  doChickenFeeder();  //Judge the time, if chickens have been fed, and feed them accordingly.
}

// ************************************** BLYNK FUNCTIONS  **************************************


BLYNK_CONNECTED(){  //Updates all virtual pins to correct in-app values at start-up
  Blynk.syncVirtual(V1);  // will cause BLYNK_WRITE(V1) to be executed
  Blynk.syncVirtual(V2);  // will cause BLYNK_WRITE(V2) to be executed
  Blynk.syncVirtual(V3);  // will cause BLYNK_WRITE(V3) to be executed
  Blynk.syncVirtual(V4);  // will cause BLYNK_WRITE(V4) to be executed
  Blynk.syncVirtual(V5);  // will cause BLYNK_WRITE(V5) to be executed
  Blynk.syncVirtual(V6);  // will cause BLYNK_WRITE(V6) to be executed
  Blynk.syncVirtual(V7);  // will cause BLYNK_WRITE(V7) to be executed
  Blynk.syncVirtual(V8);  // will cause BLYNK_WRITE(V8) to be executed
}

void ChickTemp()
{
  // Sends the temperature of the Chick Thermocouple to Blynk App on virtual pin V1
  Blynk.virtualWrite(V1, ChickTempF);
}
void CoopTemp()
{
  // Sends the temperature of the Coop Thermocouple to Blynk App on virtual pin V2
  Blynk.virtualWrite(V2, CoopTempF);
}
void SendDoorStatuses()
{
/* Sends the status of the Coop Door to Blynk App on virtual pin V3.
Reed Switch 0 value means they are touching.

This function requires a "button" widget to be used in the Blynk App
and set to Virtual Pin V3 with "ON" value set to "CLOSED" and 
"OFF" value set to "OPEN." */

//COOP DOOR STATUS
  if (coopBottomSwitchPinVal == 0 && coopTopSwitchPinVal == 1) {  // if bottom reed switch circuit is closed
    Blynk.virtualWrite(V3, coopBottomSwitchPinVal);               // print to Blynk on V3 
  }
  else if (coopBottomSwitchPinVal == 1 && coopTopSwitchPinVal == 0) {  // if top reed switch circuit is closed
    Blynk.virtualWrite(V3, coopBottomSwitchPinVal);                 // print to Blynk on V3 
  }
  else {                        // if bottom and top reed switch circuits are open
    Blynk.virtualWrite(V3, 1);  // print DOOR OPEN signal to Blynk on V3
  }

//RUN DOOR STATUS
  if (runBottomSwitchPinVal == 0 && runTopSwitchPinVal == 1) {  // if bottom reed switch circuit is closed
    Blynk.virtualWrite(V4, runBottomSwitchPinVal);               // print to Blynk on V3 
  }
  else if (runBottomSwitchPinVal == 1 && runTopSwitchPinVal == 0) {  // if top reed switch circuit is closed
    Blynk.virtualWrite(V4, runBottomSwitchPinVal);                 // print to Blynk on V3 
  }
  else {                        // if bottom and top reed switch circuits are open
    Blynk.virtualWrite(V4, 1);  // print DOOR OPEN signal to Blynk on V3
  }
}

void dailyFeedStatus() //Simply sends whether or not they've been fed today to Blynk app
{
  // Sends the daily feed status to the Blynk app on virtual pin V5
  Blynk.virtualWrite(V5, dailyFeederTracker);
}

BLYNK_WRITE(V6){   //SLIDER WIDGET - Executes only when Virtual Pin V6 changes
  feedAmount = param.asDouble(); // assigning incoming value from virtual pin V6 to a variable
  // You can also use:
  // String i = param.asStr();
  // double d = param.asInt();
  Blynk.virtualWrite(V7, feedAmount);  //Reads the feed amount slider value and sends to V7 upon update
}

//Executes only when V8 changes
BLYNK_WRITE(V8){   //MANUAL FEEDER BUTTON WIDGET - Executes only when Virtual Pin V8 changes
  manualFeedStatus = param.asInt(); // assigning incoming value from virtual pin V6 to a variable
  if(manualFeedStatus == true){
    startFeederMotorC();
  }
  else{
    stopFeederMotorC();
  }
}
