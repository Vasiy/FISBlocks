/* MAIN PROJECT */
//#include <Arduino.h>
#include "KWP.h"
#include "FISLib.h"
#include "AnalogMultiButton.h" // https://github.com/dxinteractive/AnalogMultiButton

#define MAX_CONNECT_RETRIES 5

/* uncomment to enable boot message and boot image. Removed due excessive memory consumption */
//#define Atmega32u4    // limited memory - welcome message and graphics disabled
#define Atmega328 true 

#ifndef Atmega32u4 
//  #include "GetBootMessage.h"
//    #define bootmsg
//    #define bootimg
  #ifndef Atmega328
    #include "U8g2lib.h"  
      U8G2_SSD1306_128X64_NONAME_1_3W_SW_SPI u8g2(U8G2_R0, /* clock=*/ 13, /* data=*/ 11, /* cs=*/ 10, /* reset=*/ 8);
  #endif
#endif

// KWP 
#ifdef Atmega32u4 
  const uint8_t pinKLineRX = 5;
  const uint8_t pinKLineTX = 4;
#endif
#ifdef Atmega328
  const uint8_t pinKLineRX = 3;
  const uint8_t pinKLineTX = 2;
  // CDC
  /*const byte dinCDC = 6;
  const byte doutCDC = 7;
  const byte clkCDC = 8; */
#endif
KWP kwp(pinKLineRX, pinKLineTX);

// FIS
#ifdef Atmega32u4
  const uint8_t FIS_CLK = 14; 
  const uint8_t FIS_DATA = 10; 
  const uint8_t FIS_ENA = 16;
#endif
#ifdef Atmega328
  const uint8_t FIS_CLK = 4;
  const uint8_t FIS_DATA = 5; 
  const uint8_t FIS_ENA = 6; 
#endif  
FISLib FIS(FIS_ENA, FIS_CLK, FIS_DATA);

//Buttons
#ifdef Atmega32u4 
  #define btn1PIN A3
  #define btn2PIN A2
#endif
#ifdef Atmega328
  #define btn1PIN A0
  #define btn2PIN A1
#endif

const uint8_t BUTTONS_TOTAL = 2; // 2 button on each button pin
const int BUTTONS_VALUES[BUTTONS_TOTAL] = {30, 123}; // btn value
AnalogMultiButton btn1(btn1PIN, BUTTONS_TOTAL, BUTTONS_VALUES); // make an AnalogMultiButton object, pass in the pin, total and values array
const int btn_NAV = 30;
const int btn_RETURN=123;
AnalogMultiButton btn2(btn2PIN, BUTTONS_TOTAL, BUTTONS_VALUES);
const int btn_INFO = 30;
const int btn_CARS=123; 

#define NENGINEGROUPS 7
#define NDASHBOARDGROUPS 3
#define NMODULES 2              // numbers of controlled groups
int engineGroups[NENGINEGROUPS] = { 6, 101, 102, 115, 116, 117, 120 }; //2, 3, 20, 31, 118, 115, 15 };
int dashboardGroups[NDASHBOARDGROUPS] = { 1, 2, 3 };
int absGroups[NDASHBOARDGROUPS] = { 3 };                          
//int NABSGROUPS = { 1 };                                           //abs groups to read
//int airbagGroups[NDASHBOARDGROUPS] = { 1 };                       //airbag groups to read

// Note: Each unit can have his own baudrate.
// If 10400 not works whit your unit try whit other, posible values are:
// 115200, 57600, 38400, 31250, 28800, 19200,  14400, 10400, 9600, 4800, 2400, 1200, 600, 300

KWP_MODULE engine    = { "ECU",     ADR_Engine,    10400, engineGroups,    NENGINEGROUPS};
KWP_MODULE dashboard = { "CLUSTER", ADR_Dashboard, 10400, dashboardGroups, NDASHBOARDGROUPS};
//KWP_MODULE absunit = {"ABS", ADR_ABS_Brakes, absGroups, NABSGROUPS};
//KWP_MODULE airbag = {"AIRBAG", ADR_Airbag, airbagGroups, NAIRBAGGROUPS};
KWP_MODULE *modules[NMODULES]={ &dashboard, &engine }; //, &absunit };

KWP_MODULE *currentModule=modules[0];
int currentGroup=0;
int currentSensor=0;
int nSensors=0;
int maxSensors=4;
int connRetries=0;

int count=0;

// Will return:
// 0 - Nothing
// 1 - Key Up pressed
// 2 - Key Down pressed

int getKeyStatus() {
/*  btn1.update();
  btn2.update();
  if ( btn1.isPressedAfter(btn_NAV,2000) ) return 0; // disable screen after holding more than 2 sec
  if ( btn1.isPressed(btn_RETURN) ) return 1; // return and cars switch between modules
  if ( btn2.isPressed(btn_CARS) ) return 2;
  if ( btn2.isPressed(btn_INFO) ) return 3;   // switch between groups */
  btn1.update();
  btn2.update();
  if ( btn1.isPressedAfter(btn_NAV,2000) ) return 4;  // disable screen after holding more than 2 sec
  if ( btn1.isPressed(btn_RETURN) ) return 1;         // return and cars switch between modules
  if ( btn1.isPressedAfter(btn_RETURN,2000) ) return 11;         // return and cars switch between modules
  if ( btn2.isPressed(btn_CARS) ) return 2;
  if ( btn2.isPressedAfter(btn_CARS,2000) ) return 21;
  if ( btn2.isPressed(btn_INFO) ) return 3;           // switch between groups
  else return 0;  
}

void refreshParams(int type){
  if(type==1){
    if(currentSensor < nSensors -1) currentSensor++;
    else{
      currentSensor=0;
      if(currentGroup < (currentModule->ngroups) - 1) currentGroup++;
      else{
        if(currentModule->addr == ADR_Dashboard) currentModule=modules[1];
        else currentModule=modules[0];
        currentGroup=0;
        kwp.disconnect();
      }
    }
  }
  else if(type==2){
    if(currentSensor > 0) currentSensor--;
    else{
      currentSensor=nSensors-1;
      if(currentGroup > 0) currentGroup--;
      else{
        if(currentModule->addr == ADR_Dashboard) currentModule=modules[1];
        else currentModule=modules[0];
        currentGroup=currentModule->ngroups-1;
        kwp.disconnect();
      }
    }
  }
}

void setup(){
  Serial.begin(9600);
  for(int i=0; i<8; i++){
    FIS.showText("IS FIS","BLOCKS!");
    delay(500);
  }
  for(int i=0; i<8; i++){
    FIS.showText("*** AUDI ***","ALLROAD 1 GEN");
    delay(500);
  }
//  Serial.println('Init complete');
}

void loop(){
  getKeyStatus();
//  Serial.print('key pressed - ');Serial.println( getKeyStatus() ); 
  
  if(!kwp.isConnected()){
    FIS.showText("Starting",currentModule->name);
    if(kwp.connect(currentModule->addr, currentModule->baudrate)){
      FIS.showText("Con. OK!","Reading");
      connRetries=0;
    }
    else{ // Antiblocking
      if(connRetries > MAX_CONNECT_RETRIES){
        if(currentModule->addr == ADR_Dashboard) currentModule=modules[1];
        else currentModule=modules[0];
        currentGroup=0;
        currentSensor=0;
        nSensors=0;
        connRetries=0;
      }
      else connRetries++;
    }

  }
  else{
    SENSOR resultBlock[maxSensors];
    nSensors=kwp.readBlock(currentModule->addr, currentModule->groups[currentGroup], maxSensors, resultBlock);
    if(resultBlock[currentSensor].value != ""){
      FIS.showText(resultBlock[currentSensor].desc, resultBlock[currentSensor].value+" "+resultBlock[currentSensor].units);
      if(count>8){
        refreshParams(1);
        count=0;
      }
      else count++;
    }
    else{
      refreshParams(1); // WTF?
      count=0;
    }
  }
}
