#include <Arduino.h>
#include "KWP.h"
#include "FISLib.h"
#include "AnalogMultiButton.h" // https://github.com/dxinteractive/AnalogMultiButton

#define MAX_CONNECT_RETRIES 5
#define NENGINEGROUPS 7
#define NDASHBOARDGROUPS 1
#define NMODULES 2

// KWP
#define pinKLineRX 5
#define pinKLineTX 4
KWP kwp(pinKLineRX, pinKLineTX);

//FIS
const int pinENABLE = 14;
const int pinCLOCK = 15;
const int pinDATA = 16;
FISLib LCD(pinENABLE, pinCLOCK, pinDATA);

//Buttons
#define btn1PIN A3
#define btn2PIN A2
const uint8_t BUTTONS_TOTAL = 2; // 2 button on each button pin
const int BUTTONS_VALUES[BUTTONS_TOTAL] = {30, 123}; // btn value
AnalogMultiButton btn1(btn1PIN, BUTTONS_TOTAL, BUTTONS_VALUES); // make an AnalogMultiButton object, pass in the pin, total and values array
const int btn_NAV = 30;
const int btn_RETURN=123;
AnalogMultiButton btn2(btn2PIN, BUTTONS_TOTAL, BUTTONS_VALUES);
const int btn_INFO = 30;
const int btn_CARS=123; 

int engineGroups[NENGINEGROUPS] = { 2, 3, 20, 31, 118, 115, 15 };
int dashboardGroups[NDASHBOARDGROUPS] = { 2 };

// Note: Each unit can have his own baudrate.
// If 10400 not works whit your unit try whit other, posible values are:
// 115200, 57600, 38400, 31250, 28800, 19200,  14400, 10400, 9600, 4800, 2400, 1200, 600, 300

KWP_MODULE engine    = { "ECU",     ADR_Engine,    10400, engineGroups,    NENGINEGROUPS};
KWP_MODULE dashboard = { "CLUSTER", ADR_Dashboard, 10400, dashboardGroups, NDASHBOARDGROUPS};
KWP_MODULE *modules[NMODULES]={ &dashboard, &engine };

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
  btn1.update();
  btn2.update();
  if ( btn1.isPressedAfter(btn_NAV,2000) ) return 0; // disable screen after holding more than 2 sec
  if ( btn1.isPressed(btn_RETURN) ) return 1; // return and cars switch between modules
  if ( btn2.isPressed(btn_CARS) ) return 2;
  if ( btn2.isPressed(btn_INFO) ) return 3;   // switch between groups
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
    LCD.showText("IS FIS","BLOCKS!");
    delay(500);
  }
  for(int i=0; i<8; i++){
    LCD.showText("HI! I AM","A GTI :)");
    delay(500);
  }
}

void loop(){
  getKeyStatus();
     
  if(!kwp.isConnected()){
    LCD.showText("Starting",currentModule->name);
    if(kwp.connect(currentModule->addr, currentModule->baudrate)){
      LCD.showText("Con. OK!","Reading");
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
      LCD.showText(resultBlock[currentSensor].desc, resultBlock[currentSensor].value+" "+resultBlock[currentSensor].units);
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
