// Enable repeater functionality for this node
#define MY_REPEATER_FEATURE

#define MY_RADIO_NRF24
//#define MY_DEBUG    // Enables debug messages in the serial log
#define MY_BAUD_RATE  115200 // Sets the serial baud rate for console and serial gateway
#define MY_NODE_ID 4  // Sets a static id for a node

#define SN "Dimmer"
#define SV "1.0"

#include <MySensors.h>  
#include "Dimmer.h"  
#include "ControllerMonitor.h"
#include "timeout.h"

#include <SPI.h>

#define CHILD_ID_DIM_1 4
#define CHILD_ID_DIM_2 5
#define CHILD_ID_DIM_VAL_1 6
#define CHILD_ID_DIM_VAL_2 7

#define LED_PIN_1	5
#define LED_PIN_2	6

#define MAX_DIMMERS 2
#define DIMMER_DEFAULT_DIMMING  10

DimmerClass dimmer[2];

ControllerMonitor controller;

MyMessage dimmerMsg;

void setup()  
{ 
  Serial.println("setup");
  dimmer[0].begin(0, LED_PIN_1);
  dimmer[1].begin(1, LED_PIN_2);
  controller.set_callback(request_values);

  }

uint8_t request_values(uint8_t set)
{
  // request the current values
  request(CHILD_ID_DIM_1, V_DIMMER);  
  // request(CHILD_ID_DIM_2, V_DIMMER);  
  
}


void presentation()  
{ 
  char dimPeriod;
  
  // Send the Sketch Version Information to the Gateway
  sendSketchInfo(SN, SV);

  present(CHILD_ID_DIM_1, S_DIMMER);  // present as dimmer
  present(CHILD_ID_DIM_2, S_DIMMER);  // present as dimmer


 // load dimming time
  if (dimPeriod = loadState(CHILD_ID_DIM_1) == 0xFF)
    dimPeriod = DIMMER_DEFAULT_DIMMING;
  saveState(CHILD_ID_DIM_1, dimPeriod);
  dimmer[0].setFadeIn(dimPeriod);
  
  if (dimPeriod = loadState(CHILD_ID_DIM_1 + 2) == 0xFF)
    dimPeriod = DIMMER_DEFAULT_DIMMING;
  saveState(CHILD_ID_DIM_1 + 2, dimPeriod);
  dimmer[0].setFadeOut(dimPeriod);
  
  if (dimPeriod = loadState(CHILD_ID_DIM_2) == 0xFF)
    dimPeriod = DIMMER_DEFAULT_DIMMING;
  saveState(CHILD_ID_DIM_2, dimPeriod);
  dimmer[1].setFadeIn(dimPeriod);

  if (dimPeriod = loadState(CHILD_ID_DIM_2 + 2) == 0xFF)
    dimPeriod = DIMMER_DEFAULT_DIMMING;
  saveState(CHILD_ID_DIM_2 + 2, dimPeriod);
  dimmer[1].setFadeOut(dimPeriod);
  
//  lastControllerMsgTime = millis();
 
}


void receive(const MyMessage &message)
{
  int dimValue;
  byte index = message.sensor == CHILD_ID_DIM_1?0:1;
  switch (message.type)
  {
    case V_LIGHT:
    case V_DIMMER:
      dimmer[index].setLevel(atoi(message.data));
      break;
    
    case V_VAR1:
      
      dimValue = atoi( message.data );
      // Clip incoming level to valid range of 0 to 100
      dimValue = dimValue > 100 ? 100 : dimValue;
      dimValue = dimValue < 0   ? 0   : dimValue; 

      dimmer[index].setFadeIn(dimValue);
      saveState(CHILD_ID_DIM_1 + index, dimValue);
      break;

    case V_VAR2:
      
      dimValue = atoi( message.data );
      // Clip incoming level to valid range of 0 to 100
      dimValue = dimValue > 100 ? 100 : dimValue;
      dimValue = dimValue < 0   ? 0   : dimValue; 

      dimmer[index].setFadeOut(dimValue);
      saveState(CHILD_ID_DIM_1 + index + 2, dimValue);
      break;
    
    default:
      break;

  }
  controller.answer();
}


void loop()      
{
  for (byte i = 0; i < 2; i++)
    dimmer[i].process();
  controller.monitor();
 
}



