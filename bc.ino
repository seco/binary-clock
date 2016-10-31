// VCC GND CLK DAT RST

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "FastLED.h"
#include <stdio.h>
#include <Time.h>        //http://www.arduino.cc/playground/Code/Time
#include <Timezone.h>    //https://github.com/JChristensen/Timezone
#include <Wire.h>
#include "RTClib.h"

// For led chips like Neopixels, which have a data line, ground, and power, you just
// need to define DATA_PIN.  For led chipsets that are SPI based (four wires - data, clock,
// ground, and power), like the LPD8806 define both DATA_PIN and CLOCK_PIN
#define DATA_PIN D6
#define NUM_LEDS 20

// Which colors for each digit? These are named COLOR_XY where X is the index of the digit,
// (1,2 for hours; 3,4 for minutes; 5,6 for seconds) and Y is the state of the bit,
// (0 for off, 1 for on)
#define COLOR_10 0x000000
#define COLOR_20 0x000000
#define COLOR_30 0x000000
#define COLOR_40 0x000000
#define COLOR_50 0x000000
#define COLOR_60 0x000000

#define COLOR_11 0x100000
#define COLOR_21 0x101000
#define COLOR_31 0x001000
#define COLOR_41 0x001010
#define COLOR_51 0x000010
#define COLOR_61 0x100010

/**************** WIFI ****************/
char ssid[] = "";  //  your network SSID (name)
char pass[] = "";       // your network password

unsigned int localPort = 2390;      // local port to listen for UDP packets

/* Don't hardwire the IP address or we won't get the benefits of the pool.
 *  Lookup the IP address for the host name instead */
//IPAddress timeServer(129, 6, 15, 28); // time.nist.gov NTP server
IPAddress timeServerIP; // time.nist.gov NTP server address
const char* ntpServerName = "time.nist.gov";

// Define the array of leds
CRGB leds[NUM_LEDS];

const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message

byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;

/**************** TIMEZONE ****************/
// Set the appropriate digital I/O pin connections. These are the pin
// assignments for the Arduino as well for as the DS1302 chip. See the DS1302
// datasheet:
//
//   http://datasheets.maximintegrated.com/en/ds/DS1302.pdf
const int kCePin   = D3;  // Chip Enable
const int kIoPin   = D4;  // Input/Output
const int kSclkPin = D5;  // Serial Clock

//EU Central Time Zone (New York, Detroit)
TimeChangeRule myDST = {"CEST", Last, Sun, Mar, 2, 120};    //Daylight time = UTC + 2 hours
TimeChangeRule mySTD = {"CET", Last, Sun, Oct, 3, 60};     //Standard time = UTC + 1 hours
Timezone myTZ(myDST, mySTD);

TimeChangeRule *tcr;        //pointer to the time change rule, use to get TZ abbrev
time_t utc, local;

bool sync = true;

RTC_DS3231 rtc;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

// digital pin D3 has a powerswitch attached to it. Give it a name:
int powerSwitch = D3;

void setup() {

  /**************** WIFI ****************/
  Serial.begin(115200);
  Serial.println();
  Serial.println();

  // We start by connecting to a WiFi network
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Starting UDP");
  udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(udp.localPort());
  
  /**************** FASTLED ****************/
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, lets set the time!");
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
    sync = false;
  }

  // make the powerswitch's pin an input:
  pinMode(powerSwitch, INPUT);

}

void loop() {
  
  // Get the current time and date from the chip.
  DateTime now = rtc.now();

  if(now.hour() % 20 == 0 && now.minute() == 50){
    sync = false;
  }

  if((now.second() % 10 == 0 && !sync)){
    syncTime();
  }

  // Get the current time and split digits.
  int hr[] = {now.hour() / 10, now.hour() % 10};
  int min[] = {now.minute() / 10, now.minute() % 10};
  int sec[] = {now.second() / 10, now.second() % 10};

  if(digitalRead(powerSwitch)){
    for (int i = 0; i < 2; i++) {
      if (hr[0] % 2) {
        leds[i] = COLOR_11;
      } else {
        leds[i] = COLOR_10;
      }
      hr[0] = hr[0] / 2;
    }
    for (int i = 2; i < 6; i++) {
      if (hr[1] % 2) {
        leds[i] = COLOR_21;
      } else {
        leds[i] = COLOR_20;
      }
      hr[1] = hr[1] / 2;
    }
    for (int i = 6; i < 9; i++) {
      if (min[0] % 2) {
        leds[i] = COLOR_31;
      } else {
        leds[i] = COLOR_30;
      }
      min[0] = min[0] / 2;
    }
    for (int i = 9; i < 13; i++) {
      if (min[1] % 2) {
        leds[i] = COLOR_41;
      } else {
        leds[i] = COLOR_40;
      }
      min[1] = min[1] / 2;
    }
    for (int i = 13; i < 16; i++) {
      if (sec[0] % 2) {
        leds[i] = COLOR_51;
      } else {
        leds[i] = COLOR_50;
      }
      sec[0] = sec[0] / 2;
    }
    for (int i = 16; i < 20; i++) {
      if (sec[1] % 2) {
        leds[i] = COLOR_61;
      } else {
        leds[i] = COLOR_60;
      }
      sec[1] = sec[1] / 2;
    }
  }
  else {
    for (int i = 0; i < 20; i++) {
     leds[i] = 0x000000;
    }
  }
  
  FastLED.show();
  delay(500);  
}

// send an NTP request to the time server at the given address
unsigned long sendNTPpacket(IPAddress& address)
{
  Serial.println("sending NTP packet...");
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}

void syncTime(){
  //get a random server from the pool
  WiFi.hostByName(ntpServerName, timeServerIP); 

  sendNTPpacket(timeServerIP); // send an NTP packet to a time server
  // wait to see if a reply is available
  delay(1000);
  
  int cb = udp.parsePacket();
  if (!cb) {
    Serial.println("no packet yet");
    
  }
  else {
    Serial.print("packet received, length=");
    Serial.println(cb);
    // We've received a packet, read the data from it
    udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    Serial.print("Seconds since Jan 1 1900 = " );
    Serial.println(secsSince1900);

    // now convert NTP time into everyday time:
    Serial.print("Unix time = ");
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventyYears;
    // print Unix time:
    Serial.println(epoch);


    // print the hour, minute and second:
    Serial.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
    Serial.print((epoch  % 86400L) / 3600); // print the hour (86400 equals secs per day)
    Serial.print(':');
    if ( ((epoch % 3600) / 60) < 10 ) {
      // In the first 10 minutes of each hour, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.print((epoch  % 3600) / 60); // print the minute (3600 equals secs per minute)
    Serial.print(':');
    if ( (epoch % 60) < 10 ) {
      // In the first 10 seconds of each minute, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.println(epoch % 60); // print the second

    utc = epoch;
    local = myTZ.toLocal(utc, &tcr);

    Serial.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
    Serial.print(hour(utc), DEC);
    Serial.print(':');
    Serial.print(minute(utc), DEC);
    Serial.print(':');
    Serial.print(second(utc), DEC);
    Serial.println();

    Serial.print("The local time is ");     // local is the time in Amsterdam (CET / CEST)
    Serial.print(hour(local), DEC);
    Serial.print(':');
    Serial.print(minute(local), DEC);
    Serial.print(':');
    Serial.print(second(local), DEC);
    Serial.println();
    
    // Set the time and date on the chip.
    rtc.adjust(DateTime(year(local), month(local), day(local), hour(local), minute(local), second(local)));

    sync = true;
  }
}
