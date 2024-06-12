#include <Arduino.h>
#include <TFT_eSPI.h>
#include <gauge_face.h>
#include <needle.h>
#include <XPLPro.h>              //  include file for the X-plane direct interface 

/*
 * 
 * XPLProBeaconDemo
 * 
 * Created by Curiosity Workshop for XPL/Pro arduino->XPlane system.
 * 
 * This sketch was developed and tested on an Arduino Mega.
 * 
   To report problems, download updates and examples, suggest enhancements or get technical support:
  
      discord:  https://discord.gg/RacvaRFsMW
      patreon:  www.patreon.com/curiosityworkshop
      YouTube:  https://youtube.com/channel/UCISdHdJIundC-OSVAEPzQIQ
 * 
 * 
 */

#include <XPLPro.h>              //  include file for the X-plane direct interface 
XPLPro XP(&Serial);      // create an instance of it

long int startTime;
int drefOxygenPSI;         // this stores a handle to the Oxygen PSI
int drefInstrumentBrightness;           // Instrument Brightness 


float OxygenPSIValue;     // Value of the Oxygen PSI
float InstrumentBrightnessRatio = 1; // Instrument Brightness ratio
float InstrumentBrightness; // Instrument Brightness for TFT

int drefRPM;

float engineRPM = 0;

TFT_eSPI tft = TFT_eSPI(); // Invoke custom library

// Sprites used in display
TFT_eSprite mainSpr = TFT_eSprite(&tft); // Main sprite. Full screen.

// The main gauge sprite
TFT_eSprite gaugeSpr = TFT_eSprite(&tft); // Needle Sprite

// The needle need to be sprite so it can be rotated and masked.
TFT_eSprite needleSpr = TFT_eSprite(&tft); // Needle Sprite

TFT_eSprite needleBackSpr = TFT_eSprite(&tft); // Needle Sprite


// Create image buffers for double buffering
#define INSTRUMENT_WIDTH 240
#define INSTRUMENT_HEIGHT 240


// put function declarations here:
int i = 0;
int needleRotDegrees = 0;
int direction = 0; // 0 = clockwise, 1 = counterclockwise

// Declare functions
void xplInboundHandler(inStruct *inData);
void xplRegister();
void xplShutdown();
float scaleValue (float x, float in_min, float in_max, float out_min, float out_max);


void setup() 
{
 
  pinMode(LED_BUILTIN, OUTPUT);     // built in LED on arduino board will turn on and off with the status of the beacon light
 
  Serial.begin(XPL_BAUDRATE); // start serial interface.  Baudrate is specified in the header, dont change   

  while (!Serial)
    ;

delay(1000); // wait for serial monitor to open
Serial.println("\r\nOxygen Gauge Start\n");

 /*
    needed for initialization.  Parameters are:
        a texual identifier of your device 
        your function that will be called when xplane and the plugin are ready for dataref and command registrations
        your function that will be called when xplane either shuts down or unloads an aircraft model
        your function that will be called when we have requested sends of datarefs that have changed
 */
  XP.begin("Oxygen Gauge Start", &xplRegister, &xplShutdown, &xplInboundHandler);               
  digitalWrite(LED_BUILTIN, LOW);

tft.begin();
tft.setRotation(0); // 0 & 2 Portrait. 1 & 3 landscape
tft.fillScreen(TFT_BLACK); // Clear screen. We are only going to use the top part. If you don't clear, the bottom half will be noise.

// Create the gauge sprite
gaugeSpr.createSprite(240, 240);
gaugeSpr.setSwapBytes(true);
gaugeSpr.fillSprite(TFT_BLACK);
// Set the pivot point for the needle to rotate
gaugeSpr.setPivot(120, 120);
gaugeSpr.pushImage(0, 0, 240, 240, gauge_face);

// Create the needle sprite
needleSpr.createSprite(28, 148);
needleSpr.setSwapBytes(true);
needleSpr.setPivot(14, 110);
needleSpr.fillScreen(TFT_BLACK);
needleSpr.pushImage(0, 0, 28, 148, needle);

Serial.println("\r\nInitialisation done.\r\n");

}


void loop() 
{  


  XP.xloop();  //  needs to run every cycle.  


  gaugeSpr.pushImage(0, 0, 240, 240, gauge_face);

  needleSpr.pushImage(0, 0, 28, 148, needle);
  needleSpr.setSwapBytes(false);

  // Map the Oxygen PSI value to the degrees of rotation of the needle
  needleRotDegrees = map(OxygenPSIValue, 0, 4000, -140, 140);

  
  Serial.println(needleRotDegrees);

  needleSpr.setSwapBytes(false);
  needleSpr.pushRotated(&gaugeSpr, needleRotDegrees, TFT_BLACK);
  needleSpr.setSwapBytes(true);
  
  gaugeSpr.pushSprite(0, 0, TFT_BLACK);

  InstrumentBrightness = int(scaleValue(InstrumentBrightnessRatio, 0.2, 1, 0, 255));
  // Set the instrument light brightness
  analogWrite(TFT_BL, InstrumentBrightness);


/************************************************************************************************************************************
 * everything after the next line will only occur every 100ms.  You can also utilize other time values.
 * This helps maintain serial data flow and also helps with switch debounce.
 * do NOT add delays anywhere in the loop cycle, it will interfere with the reception of serial data from xplane and the plugin.
 ************************************************************************************************************************************
*/
 
   if (millis() - startTime > 100) startTime = millis();   else return;
}


/*
 * This function is the callback function we specified that will be called any time our requested data is sent to us.
 * handle is the handle to the dataref.  The following variables within the plugin are how we receive the data:
 * 
 * inStruct *inData is a pointer to a structure that contains information about the incoming dataref. 
 * 
 *  int inData->handle          The handle of the incoming dataref
 *  int inData->element         If the dataref is an array style dataref, this is the element of the array
 *  long inData->inLong         long value for datarefs that are long values
 *  float inData->inFloat       float value for datarefs that are float values
 */


 void xplInboundHandler(inStruct *inData)
{

  // get the value of the Oxygen PSI from the XPL Pro Plugin in X-Plane
  if (inData->handle == drefOxygenPSI)
    OxygenPSIValue = inData->inFloat;
  
  if (inData->handle == drefInstrumentBrightness)
    if(inData->element == 1)
      InstrumentBrightnessRatio = inData->inFloat;

}

/*
 *  this is the function we set as a callback for when the plugin is ready to receive dataref and command bindings requests
 *  It could be called multiple times if for instance xplane is restarted or loads a different aircraft
 */
void xplRegister()         
{

  drefOxygenPSI = XP.registerDataRef("sim/cockpit2/oxygen/indicators/o2_bottle_pressure_psi");  
  drefInstrumentBrightness = XP.registerDataRef("sim/cockpit2/electrical/instrument_brightness_ratio_manual");  

  XP.requestUpdates(drefOxygenPSI, 100, 0);          // Tell xplane to send us updates when the value of the Oxygen PSI.  
  XP.requestUpdates(drefInstrumentBrightness, 100, 0.01, 1);          // Tell xplane to send us updates when the value of the Instrument Brightness

}

void xplShutdown()
{
  // if you need to do things when xplane shuts down or unloads an aircraft, do it here.
  
}

float scaleValue (float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}