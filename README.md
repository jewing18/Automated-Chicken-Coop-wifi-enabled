# Automated-Chicken-Coop-wifi-enabled
This is the code and wiring diagram for an Automated Chicken Coop that is wifi enabled.

This is my first (and likely only) GitHub post, as I am only a mechanical engineer that loves
chickens and random projects like this. I'd like to share this with all of you out there looking
to create your own automated chicken coop.

**~ Features of this Automated Coop Include: ~**
- 2 Coop Doors (a coop door and a run door), powered by 12V DC electric motors that will 
  automatically open and close at user-specified times of day. 
- An automatic chicken feeder that activates at a user-specified time of day (once per day)
  and for a user-specified duration depending on how much feed is desired.
- 2 Thermocouples to monitor the temperature of the coop and (if desired) chicks. 
- A smartphone user-interface using the "Blynk App" to control and monitor all coop functions. 

**~ Components needed for Automation: ~**
- (1) Songhe Mega 2560 + WiFi (Arduino Mega w/ ESP8266 built-in)
- (2) L298N H-Bridge Motor Driver
- (4) RC-33 Reed Switches 
- (2) DS18B20 Thermocouples
- (1) 12V Power Supply
- (3) ~30rpm DC Electric Motors -> 2 for doors, 1 for feed drive
- (1) 4.7kohm resistor
- (4) 10kohm resistor
- (-) Necessary Wiring
- (1) Smartphone with "Blynk" App installed


1.) **Initial Board Setup information**
https://www.gabrielcsapo.com/arduino-web-server-mega-2560-r3-built-in-esp8266/
- The link above is a great reference for how to set up the Mega 2560 + Wifi
- Getting connected to wifi may be the hardest part of setting up the Automated coop. 
- Use his guide above to get hooked up to wifi. If there is still trouble, it may be helpful to
  use the wifi portions of my code in tandem with his setup info to get connected.
  
2.) **Blynk App Setup**
- Download and login to the 'Blynk' app to your smartphone
- Go online to Blynk website using computer and set up the necessary datastreams.
- In total, this project has 8 datastreams. Datastreams are what Blynk uses to 
  communicte with the arduino and your smartphone app. Please see any tutorial for how
  to set up datastreams. Use the datastream settings shown in the "Blynk_Datastream_Setup.png" file
  for this project.
  
3.) **Code Inputs**
This is a list of the inputs that you will need to modify in your code in the "CONSTANT INPUTS HERE" section of the code to suit your own situation:
- minutes past sunrise for coop door to open
- minutes past sunrise for run door to open
- minutes past sunset for coop door to close
- minutes past sunset for run door to close
- your longitude
- your latitude
- your hour offset from UTC time during daylight savings time
- your hour offset from UTC time NOT during daylight savings time
- feedrate tested value (lb/min) - Auto Feeder shoots out xx lb/min
- the time in which you want to feed to chickens daily (mins past midnight)
- Blynk app communication delays (how often to communicate with the Blynk App)

Additionally you will need to input your wifi SSID and Password in the 'arduino_serets.h' file.
Also, you will need to input your 'Blynk Template ID' and 'Authorization Token'





