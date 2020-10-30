#include "Adafruit_GFX.h"    // Core graphics library
#include "Adafruit_HX8357.h" // Hardware-specific library
#include "SPI.h" //built in library should be SPI.h 
#include "Adafruit_SPITFT_Macros.h"
#include <Wire.h>
#include "Adafruit_AMG88xx.h"
#include <avr/pgmspace.h>
#define TFT_CS     10 //chip select pin for the TFT screen
#define TFT_RST    -1  // you can also connect this to the Arduino reset
                      // in which case, set this #define pin to 0!
#define TFT_DC     9
float pixels[AMG88xx_PIXEL_ARRAY_SIZE];

//low range of the sensor (this will be blue on the screen)
#define MINTEMP 22

//high range of the sensor (this will be red on the screen)
#define MAXTEMP 34


// 0 no optimse |1 pixels only written whe color changed| 2 pixels also optimized for most changed ones first (deals with noise issues)
#define optimize 0

#define interpolatemode 1 
#define colorMode 1  //can be 0=64 color adafruit, 1=256 color map 0,1 use same 256 color table , just 0 has same value 4 times
#define spi_optimized_st77xx false 
#if spi_optimized_st77xx == true
#define fillRectFast tft.fillRectFast
#else
#define fillRectFast tft.fillRect
#endif
#if colorMode == 1
const PROGMEM uint16_t camColors[] =  
{0x0004,0x0004,0x0005,0x0005,0x0006,0x0007,0x0007,
0x0008,0x0008,0x0009,0x0009,0x000a,0x000b,0x000b,
0x000c,0x000c,0x000d,0x000d,0x000e,0x000f,0x000f,
0x0010,0x0010,0x0011,0x0012,0x0012,0x0013,0x0013,
0x0014,0x0014,0x0015,0x0016,0x0016,0x0017,0x0017,
0x0018,0x0018,0x0019,0x001a,0x001a,0x001b,0x001b,
0x001c,0x001c,0x001d,0x001e,0x001e,0x001f,0x001f,
0x001f,0x003e,0x003e,0x005d,0x005d,0x007c,0x009b,
0x009b,0x00ba,0x00ba,0x00d9,0x00d8,0x00f8,0x0117,
0x0117,0x0136,0x0136,0x0155,0x0174,0x0174,0x0193,
0x0193,0x01b2,0x01b2,0x01d1,0x01f0,0x01f0,0x020f,
0x020f,0x022e,0x024d,0x024d,0x026c,0x026c,0x028b,
0x028b,0x02aa,0x02c9,0x02c9,0x02e8,0x02e8,0x0307,
0x0307,0x0326,0x0345,0x0345,0x0364,0x0364,0x0383,
0x03a3,0x03a2,0x03c1,0x03c1,0x03e0,0x03e0,0x0400,
0x0c20,0x0c20,0x1440,0x1440,0x1c60,0x1c60,0x2480,
0x2ca0,0x2ca0,0x34c0,0x34c0,0x3ce0,0x44e0,0x4500,
0x4d20,0x4d20,0x5540,0x5540,0x5d60,0x6580,0x6580,
0x6da0,0x6da0,0x75c0,0x75c0,0x7de0,0x8600,0x8600,
0x8e20,0x8e20,0x9640,0x9640,0x9e60,0xa680,0xa680,
0xaea0,0xaea0,0xb6c0,0xbec0,0xbee0,0xc700,0xc700,
0xcf20,0xcf20,0xd740,0xdf40,0xdf60,0xe780,0xe780,
0xefa0,0xefa0,0xf7c0,0xffe0,0xffe0,0xffe0,0xffe0,
0xffc0,0xffc0,0xffa0,0xffa0,0xffa0,0xff80,0xff80,
0xff60,0xff60,0xff60,0xff40,0xff40,0xff20,0xff20,
0xff00,0xff00,0xff00,0xfee0,0xfee0,0xfec0,0xfec0,
0xfec0,0xfea0,0xfea0,0xfe80,0xfe80,0xfe80,0xfe60,
0xfe60,0xfe40,0xfe40,0xfe40,0xfe20,0xfe20,0xfe00,
0xfe00,0xfde0,0xfde0,0xfde0,0xfdc0,0xfdc0,0xfda0,
0xfda0,0xfda0,0xfd80,0xfd80,0xfd60,0xfd60,0xfd60,
0xfd40,0xfd40,0xfd20,0xfd20,0xfd00,0xfd00,0xfce0,
0xfcc0,0xfca0,0xfca0,0xfc80,0xfc60,0xfc40,0xfc40,
0xfc20,0xfc00,0xfbe0,0xfbe0,0xfbc0,0xfba0,0xfb80,
0xfb80,0xfb60,0xfb40,0xfb20,0xfb20,0xfb00,0xfae0,
0xfac0,0xfac0,0xfaa0,0xfa80,0xfa60,0xfa60,0xfa40,
0xfa20,0xfa00,0xfa00,0xf9e0,0xf9c0,0xf9a0,0xf9a0,
0xf980,0xf960,0xf940,0xf940};
#endif
Adafruit_HX8357 tft = Adafruit_HX8357(TFT_CS,  TFT_DC, TFT_RST);

Adafruit_AMG88xx amg;
unsigned long delayTime;
void setup() {
  // Serial.begin(9600);
  Serial.begin(115200);
  tft.begin(HX8357D);
  digitalWrite(6,LOW);
  Serial.println(F("AMG88xx thermal camera!"));

  tft.fillScreen(HX8357_BLACK);

  #define    displayPixelWidth  tft.width() /8  //allows values to be hardcoded
  #define    displayPixelHeight   tft.height() / 8

  bool status;
    
  // default settings
  status = amg.begin();
  if (!status) {
      Serial.println(F("Could not find a valid AMG88xx sensor, check wiring!"));
      while (1);
  }
  
  Serial.println( F("-- Thermal Camera Test --"));
  
  delay(100); // let sensor boot up

}
#define speedUpCompression 8
uint8_t compressionnumber;//this counts pixels processed, if to many processed we make compression flux higher
uint8_t compressionflux=50;//this number goes up and down depending on how many pixesl are changing (usually from noise)
byte runagain=0;//used to refesh different amounts, for example we only process 1/4 of low quality pixes to make sure refesh happens besides with difference
void loop() {
  digitalWrite(6,LOW);
  amg.readPixels(pixels);
  
  
  byte i=0;
  byte j=0; 
  byte k=0;
  
  compressionnumber=0;// we reset each time thru
  runagain++;runagain=runagain&15; //we run this routing to update a few times0,1, maybe more later on
  while(k<8 ){ 
    switch(k){// this allows j to do interleaving of page, when updates are slower, and improvese results of compressed bandwitdh
    case 0: j=0; break;case 1: j=2; break;case 2: j=4; break;case 3: j=6; break;case 4: j=1; break;case 5: j=3; break;case 6: j=5; break;case 7: j=7; break;
    }
    if (i>7){i=0;k++;}//here we run code that makes sense of supersubsamples, sample is done when i=8,k=16
    
    
    // |
    // V i . j ------>direction of display sensor. helps when figuring out interpolation
   #if colorMode <2
    int colorIndex = map(pixels[i+j*8], MINTEMP, MAXTEMP, 0, 255); //we resuse this sample 
    #else
    //trying to average pixle data//
     int colorIndex = map(pixels[i+j*8], MINTEMP, MAXTEMP, 0, 1023); //we resuse this sample 
    #endif
   
    //we now compress and only update pixels on screen that change the most! it works and over time they all change, but to be sure force updates
  
    
    #if interpolatemode == 0
       //this is original for display code pixel placement
       //there is also spi optimizations to allow spi bursts during pixel writes
        tft.fillRect(displayPixelWidth *j, displayPixelHeight * i,displayPixelWidth, displayPixelHeight,(uint16_t)pgm_read_word_near(camColors+colorIndex));
    #endif
    
    
    
    
    #if interpolatemode > 0
    //how it updates when more than 2x2 sub pixels
    int pixelSizeDivide= 2*interpolatemode ; 
    //[0][4][8][c] or [0][2] //order reverses depending on sample interpolateSampleDir 
    //[1][5][9][d]    [1][3]
    //[2][6][a][e]
    //[3][7][b][f]
    //fast subdivide low memory pixel enhancing code (by James Villeneuve 7-2018 also referencing MIT code and adafruit library for pixel placement code)
         
    int interpolatesampledir2=1; if(i<4){interpolatesampledir2=1;}else{interpolatesampledir2=-1;}//top(1) or bottom quadrunt (-1)
    int interpolateSampleDir =1;// left  (1) or right quadrunt (-1)
    int offset=0;
    
    //getsubpixelcolor()//side pixel,vertical pixil,LeftOrRigh,current pixel
    if (j<4){interpolateSampleDir =1;}// we process left to right here . we need to change this so it scales with display resolution
    else{interpolateSampleDir =-1;offset=displayPixelHeight-displayPixelHeight/pixelSizeDivide;}//if past half way on display we sample in other direction
    //long timecount=micros();
    
    // getsubpixelcolor(pixelSizerDivide);//side pixel,vertical pixil,LeftOrRigh,current pixel
    for (int raster_x=0;raster_x !=(pixelSizeDivide*interpolateSampleDir)  ;raster_x += 1*interpolateSampleDir){ //done with != instead of <> so i could invert direction ;)
    for (int raster_y=0;raster_y != (pixelSizeDivide* interpolateSampleDir) ;raster_y += 1* interpolateSampleDir){ //0,1  
    //we keep sample size from nieghbor pixels even when sample divides increase
    
    #if colorMode <2
    int  tempcolor= map(pixels[(i+(interpolatesampledir2+raster_y/(pixelSizeDivide/2)))+(j+(raster_x/(pixelSizeDivide/2)))*8], MINTEMP, MAXTEMP, 0, 255);//we constrain color after subsampling
    #else
    int  tempcolor= map(pixels[(i+(interpolatesampledir2+raster_y/(pixelSizeDivide/2)))+(j+(raster_x/(pixelSizeDivide/2)))*8], MINTEMP, MAXTEMP, 0, 1023);//we constrain color after subsampling
    #endif
    
    //next line changes the average of the color between the main pixel and the sub pixels
    tempcolor=(( tempcolor*(pixelSizeDivide-raster_y)+ colorIndex*raster_y)/pixelSizeDivide+(tempcolor*(raster_x)+colorIndex*(pixelSizeDivide-raster_x))/pixelSizeDivide)/2;//subsample with real pixel and surounding pixels
    
    // prebuffer[AMG88xx_PIXEL_ARRAY_SIZE*4]
    //Serial.print("x:");
    //Serial.print(raster_x/2);
    //Serial.print(" y:");
    //Serial.println(raster_y/2);
    //tempcolor=(tempcolor+ colorIndex)/2;//subsample with real pixel and surounding pixels
    #if colorMode < 2
    tempcolor=constrain(tempcolor,0,255);//subsample with real pixel and surounding pixels
    #else
    tempcolor=constrain(tempcolor,0,1023);//subsample with real pixel and surounding pixels
    #endif
    
    
    //formated line below for easier reading
    //*******place pixels*************
    fillRectFast(displayPixelWidth *j
    +offset+ (interpolateSampleDir*displayPixelWidth/pixelSizeDivide)*(raster_x*interpolateSampleDir), //we reduce pixle size and step over in raster extra pixels created
    displayPixelHeight* i+offset+(interpolateSampleDir*displayPixelHeight/pixelSizeDivide)*(raster_y*interpolateSampleDir),
    displayPixelWidth/pixelSizeDivide,//we divide width of pixels. /2,4,8,16 is fast and compiler can just do bit shift for it
    displayPixelHeight/pixelSizeDivide,//we divide hieght of pixels.
    (uint16_t)pgm_read_word_near(camColors+tempcolor));  //we update pixel location with new subsampled pixel.
    //would it make sense to subdivide to color of pixel directly? yes except camcolors is set with colors translated for heat.
    //timertemp=micros()-timertemp;//we stop counter and subtract time of routine
    //Serial.println(timertemp);
       }//interpolatepixel_y 
    }//interpolatepixel_x    
    #endif
    
    //below ends the check buffer loop
    #if optimize >0 
  }
  #endif
  #if optimize >1
      }
  #endif
     //we update each pixel data
   #if optimise > 1
   //pixelsbuf[i+j*8]=colorIndex;
   #endif
  
  
   i++;   
  //old way
    //         tft.fillRect(displayPixelHeight * (i /8), displayPixelWidth * (i % 8),
      //    displayPixelHeight, displayPixelWidth, camColors[colorIndex]);
   
    }
  
  //we sample one time per frame 
  if (compressionnumber>speedUpCompression){if (compressionflux<255) {compressionflux+=1;};
  }//we increase range more slowly 
  else{
    if (compressionflux>0){compressionflux-=1;};
  }
  //here we rerun routine, but up resolution
  

}
