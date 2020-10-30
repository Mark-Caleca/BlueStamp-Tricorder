//code written by Mark C at BlueStamp Engineering 2018 
//3.15.19
//libraries for screen + touchscreen
#include "Adafruit_GFX.h"    // Core graphics library
#include "SPI.h"
#include "Adafruit_HX8357.h"
#include "TouchScreen.h"
//touchscreen 
#define YP A2  
#define XM A3 
#define YM 7  
#define XP 8   
#define TS_MINX 110
#define TS_MINY 80
#define TS_MAXX 900
#define TS_MAXY 940
#define MINPRESSURE 2
#define MAXPRESSURE 1000
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);
#define BOXSIZE_w 160//148
#define BOXSIZE_h 136
// The display uses hardware SPI, plus #9 & #10
#define TFT_RST -1  // dont use a reset pin, tie to arduino RST if you like
#define TFT_DC 9
#define TFT_CS 10
Adafruit_HX8357 tft = Adafruit_HX8357(TFT_CS, TFT_DC, TFT_RST);
//DHT library 
#include "DHT.h"
#define DHTPIN 2     // what digital pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
// Initialize DHT sensor.
DHT dht(DHTPIN, DHTTYPE);
//Define DHT variables; otherwise functions won't recognize them
float h=0;
float t=0;
float f=0;
float hic=0;
float hif=0;
//photocell
int photocellPin = 6;     // the cell and 10K pulldown are connected to a0
int photocellReading;     // the analog reading from the analog resistor divider
//UV sensor 
#include "Wire.h"
#include "Adafruit_SI1145.h"
Adafruit_SI1145 uv = Adafruit_SI1145();
//define UV variables
float UVindex=0;
//set cursor, used to center text
unsigned long cursor1(int numb_right,int numb_up, int stringlength){
  //numb is number of rectangles, stringlength is length of text
  tft.setCursor(BOXSIZE_w *(numb_right-0.5) -(3*stringlength*3), BOXSIZE_h *numb_up);
  //only works with default font, change (textsize*stringlength*3) for different text size (currently size is 3)
}
//defeault colors  
#define txtcolor 0x001F //0x0FE9
#define bkgndcolor HX8357_WHITE
#define boxcolor 0xFE9 
//bool definitions for later
bool dht_selected = false;
bool gps_selected = false;
bool uv_selected = false;
bool light_selected = false;
bool thermal_selected = false;
//GPS
#include "Adafruit_GPS.h"
#include "SoftwareSerial.h"
SoftwareSerial mySerial(3, 5);
Adafruit_GPS GPS(&mySerial);
#define GPSECHO  false //true debugs, false does not
boolean usingInterrupt = false;
void useInterrupt(boolean);
//run once at startup
void setup() {
  //multiplexer cd4053b
  pinMode(6, OUTPUT);
  pinMode(A4, INPUT);
  pinMode(A5, INPUT);
  //GPS
  GPS.begin(9600);
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);   // 1 Hz update rate
  GPS.sendCommand(PGCMD_ANTENNA);
  useInterrupt(true);
  //UV
  digitalWrite(6,HIGH);
  uv.begin();
  //Serial
  Serial.begin(9600);
  //DHT
  dht.begin();
  //screen + touchscreen 
  tft.begin(HX8357D);
  //home screen load on startup 
  home1();
}
TSPoint p = ts.getPoint();
void loop(){
  //touchscreen
  //pressure detection (makes sure not too low)
  TSPoint p = ts.getPoint();
  if (p.z < MINPRESSURE || p.z > MAXPRESSURE) {
     return;
  }
  p.x = map(p.x, TS_MINX, TS_MAXX, 0, tft.width());
  p.y = map(p.y, TS_MINY, TS_MAXY, 0, tft.height()); 
  //note: graphics originate from top left corner, touch originates from bottom left corner
  //divided and multiplied numbers below to account for not registering touch correctly 
  //detect for GPS pressed 
  if ((p.y < (BOXSIZE_w /1.57)) && (p.x < (BOXSIZE_h*1.5))){
    gps_selected = true;
  }
  //Other bottom center
  else if ((p.y < ((BOXSIZE_w*2)/1.57)) && (p.x < (BOXSIZE_h*1.5))){
    thermal_selected = true;
  }
  else if ((p.y < ((BOXSIZE_w*3)/1.57)) && (p.x < (BOXSIZE_h*1.5))){
  }
  //DHT
  else if ((p.y < (BOXSIZE_w /1.57)) && (p.x < (2*BOXSIZE_h*1.5))){
    dht_selected = true;
  }
  else if((p.y < ((BOXSIZE_w *2) / 1.57)) && (p.x <(2*BOXSIZE_h*1.5))){
    uv_selected = true;
  }
  else if((p.y < ((BOXSIZE_w*3)/1.57)) && (p.x<(2*BOXSIZE_h*1.5))){
     light_selected = true;   
  }
   else {Serial.print("top");}  
 while (dht_selected == true){
   dht_c();
  }
 while (gps_selected == true){
   gps_c();
 } 
 while (light_selected == true){
  light_c();
 }
 while(uv_selected == true){
   uv_c();
  }
/* while(thermal_selected == true){
  tft.fillScreen(HX8357_WHITE);
  tft.setCursor(1,1);
  tft.println("For the thermal camera, please run 'thermal_camera' - otherwise Arduino runs out of memory");
  delay(2000);
  home1();
 }*/
} 
//GPS
SIGNAL(TIMER0_COMPA_vect) {
  char c = GPS.read();
  // if you want to debug, this is a good time to do it!
  #ifdef UDR0
    if (GPSECHO)
      if (c) UDR0 = c;  
  #endif
}
void useInterrupt(boolean v) {
  if (v) {
    OCR0A = 0xAF;
    TIMSK0 |= _BV(OCIE0A);
    usingInterrupt = true;
  }
  else {
    // do not call the interrupt function COMPA anymore
    TIMSK0 &= ~_BV(OCIE0A);
    usingInterrupt = false;
  }
}
uint32_t timer = millis();
unsigned long gpsbk = HX8357_GREEN;
unsigned long gpstxt = HX8357_RED;
void gps_c() {
  tft.setCursor(2,2);
  tft.setTextColor(gpstxt);
  tft.fillScreen(gpsbk);
  if (GPS.newNMEAreceived()) {
    if (!GPS.parse(GPS.lastNMEA()))   
      return;
  }
  if (millis() - timer > 2000) { 
    timer = millis(); 
    GPS.read();
    tft.print("Time:");
    tft.print(GPS.hour, DEC); tft.print(":");
    tft.print(GPS.minute, DEC); tft.print(':');
    tft.print(GPS.seconds, DEC); tft.print('.');
    tft.println(GPS.milliseconds);
    tft.print("Date: ");
    tft.print(GPS.month, DEC); tft.print('/');
    tft.print(GPS.day, DEC); tft.print("/20");
    tft.println(GPS.year, DEC);
    tft.print("Fix: "); tft.print((int)GPS.fix);
    tft.print(" quality: "); tft.println((int)GPS.fixquality); 
    if (GPS.fix){
      tft.print("Location: ");
      tft.print(GPS.latitude, 4); tft.print(GPS.lat);
      tft.print(", "); 
      tft.print(GPS.longitude, 4); tft.println(GPS.lon);
      tft.print("Location in degrees: ");
      tft.print(GPS.latitudeDegrees, 4);
      tft.print(", "); 
      tft.println(GPS.longitudeDegrees, 4);
      tft.print("Speed (knots): "); tft.println(GPS.speed);
      tft.print("Angle: "); tft.println(GPS.angle);
      tft.print("Altitude: "); tft.println(GPS.altitude);
      tft.print("Satellites: "); tft.println((int)GPS.satellites);
    }
    else {
      tft.print("No GPS fix, sorry!");
    }
    delay(2000);
 }
}
//home screen
void home1(){
  //bool used later for if something is selected 
  dht_selected = false;
  gps_selected = false;
  uv_selected = false;
  light_selected = false;
  thermal_selected = false;
  tft.fillScreen(bkgndcolor);
  cursor1(2,0,13);
  tft.setTextColor(txtcolor);
  tft.setTextSize(3);
  tft.setTextWrap(true);
  tft.setRotation(1);
  tft.print("Welcome back!\n");
  tft.setCursor(BOXSIZE_w *(2-0.5) -(3*16*3),24);
  tft.print("Select a Sensor:");
  tft.drawRect(0, 48, BOXSIZE_w, BOXSIZE_h, boxcolor);
  cursor1(1,1,5);
  tft.print("DHT22");
  tft.drawRect(BOXSIZE_w, 48,BOXSIZE_w, BOXSIZE_h, boxcolor); 
  cursor1(2,1,2);
  tft.print("UV");
  cursor1(3,1,5);
  tft.print("Light");
  tft.drawRect(BOXSIZE_w*2, 48,BOXSIZE_w, BOXSIZE_h, boxcolor); 
  cursor1(1,2,3);
  tft.print("GPS");
  tft.drawRect(0, 48+BOXSIZE_h,BOXSIZE_w, BOXSIZE_h, boxcolor); 
  cursor1(2,2,8);
  tft.drawRect(BOXSIZE_w, 48+BOXSIZE_h,BOXSIZE_w, BOXSIZE_h, boxcolor); 
  tft.print("Heat Cam");
  cursor1(3,2,5);
  tft.drawRect(BOXSIZE_w*2, 48+BOXSIZE_h,BOXSIZE_w, BOXSIZE_h, boxcolor); 
  tft.print("Other");
}
//uv            
unsigned long uvbkcolor = 0xFFE0;
unsigned long uvtxtcolor = HX8357_RED;
void uv_c(){
   digitalWrite(6,HIGH);   //high is uv, low is thermal camera 
   //uv variables to fix error uv index = 0
   UVindex = uv.readUV();
   UVindex /=100.0;
   String UVadvice="";
   if (UVindex < 2){
    UVadvice="Low UV";
    uvbkcolor = 0x0400;
    uvtxtcolor = 0x07FF;
   }
   else if (UVindex < 5){
    UVadvice="Moderate UV";
    uvbkcolor = 0xEF40;
    uvtxtcolor = 0xF800;
   }
   else if (UVindex < 7){
    UVadvice="High UV";
    uvbkcolor = 0xFAA0;
    uvtxtcolor = 0x041F;
   }
   else if (UVindex < 10){
    UVadvice="Very High UV";
    uvbkcolor = 0xF800;
    uvtxtcolor = 0xFDB6;
   }
   else if (UVindex > 10){
    UVadvice="Extremely High UV";
    uvbkcolor = 0x701C;
    uvtxtcolor = 0x0408;
   }
   tft.fillScreen(uvbkcolor);
   tft.setTextColor(uvtxtcolor);
   tft.setCursor(2,2);
   tft.print("Visible:");
   tft.println(uv.readVisible());
   tft.print("IR:");
   tft.println(uv.readIR());
   tft.print("UV index:");
   tft.println(UVindex);
   tft.print(UVadvice);
   delay(2000);
}
//photocell "light"
unsigned long lbkcolor = 0xFFE0;
unsigned long ltxtcolor = 0xF800;
void light_c(){
  photocellReading = analogRead(photocellPin); 
  String light_reading = "";
  if (photocellReading < 10) {
    light_reading=" Dark";
    lbkcolor = HX8357_BLACK;
    ltxtcolor = HX8357_WHITE;
  } else if (photocellReading < 200) {
    light_reading=" Dim";
    lbkcolor = 0x8410;
    ltxtcolor = 0x69A0;
  } else if (photocellReading < 500) {
    light_reading=" Light";
    lbkcolor = 0xFFF0;
    ltxtcolor = 0xFC10;
  } else if (photocellReading < 800) {
    light_reading=" Bright";
    lbkcolor = 0xFFE0;
    ltxtcolor = 0xF800;
  } else {
    light_reading=" Very bright";
    lbkcolor = HX8357_WHITE;
    ltxtcolor = HX8357_BLACK;
  } 
  tft.fillScreen(lbkcolor);
  tft.setTextColor(ltxtcolor);
  tft.setCursor(2,2);
  tft.print("Photocell says it is:");
  tft.println(photocellReading);
  tft.print("This is:");
  tft.println(light_reading);
  delay(2000);
}
//DHT 
unsigned long dhtbkcolor = 0x07E0; 
unsigned long dhttxtcolor = HX8357_BLACK; 
void dht_c() {
    h = dht.readHumidity();
    // Read temperature as Fahrenheit (isFahrenheit = true)
    f = dht.readTemperature(true);    
    hif = dht.computeHeatIndex(f, h);
    //change color based on heat index
    if (hif > 60 && hif <80) {
     dhtbkcolor = 0xE643;
     dhttxtcolor = 0x8420;
    }
    else if (hif > 80){
    dhtbkcolor = 0xFFE0;
    dhttxtcolor = 0xF800;
   }
   else if (hif < 60 && hif > 40) {
    dhtbkcolor = 0x07FF;
    dhttxtcolor = 0xFFFF;
   }
   else if (hif < 40 && hif >20){
    dhtbkcolor = 0x0019;
    dhttxtcolor = 0xFFF6;
   }
   else if (hif < 20){
    dhtbkcolor = 0x004B;
    dhttxtcolor = 0x9CF3;
   }
    tft.fillScreen(dhtbkcolor);
    tft.setTextColor(dhttxtcolor);
    // Compute heat index, temperature, humidity in Fahrenheit (the default)
    tft.setCursor(tft.width()/4.7,tft.height()/2-36);
    tft.print("Humidity: "); tft.print(h); tft.print("%\n");
    tft.setCursor(tft.width()/7.2,tft.height()/2-12);
    tft.print("Temperature: "); tft.print(f); tft.print("*F\n");
    tft.setCursor(tft.width()/6.2,tft.height()/2+12);
    tft.print("Heat index: "); tft.print(hif); tft.print("*F\n");
    delay(2000); //refresh screen with new data 
}
