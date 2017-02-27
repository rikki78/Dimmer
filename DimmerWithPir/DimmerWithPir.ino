// Enable repeater functionality for this node
#define MY_REPEATER_FEATURE

#define MY_RADIO_NRF24
//#define MY_DEBUG    // Enables debug messages in the serial log
#define MY_BAUD_RATE  115200 // Sets the serial baud rate for console and serial gateway
#define MY_NODE_ID 6  // Sets a static id for a node

#define SN "Dimmer"
#define SV "2.0"

#include <MySensors.h>  
#include <Dimmer.h>
#include <ControllerMonitor.h>
#include <timeout.h>

#include <SPI.h>
#include <DHT.h>  

#define CHILD_ID_DIM_1 4
#define CHILD_ID_DIM_2 5

#define LED_PIN_1	5
#define LED_PIN_2	6

#define MAX_DIMMERS 2
#define DIMMER_DEFAULT_DIMMING  10

DimmerClass dimmer[2];

ControllerMonitor controller;

MyMessage dimmerMsg;


boolean metric = true, motion, lastMotion, motionChange; 


#define CHILD_ID_HUM 0
#define CHILD_ID_TEMP 1
#define CHILD_ID_MOT 2   // Id of the sensor child
#define CHILD_ID_LIGHT 3

#define HUMIDITY_SENSOR_DIGITAL_PIN 4
#define LIGHT_SENSOR_ANALOG_PIN A0
#define MOTION_INPUT 3   // The digital input you attached your motion sensor.  (Only 2 and 3 generates interrupt!)
#define INTERRUPT MOTION_INPUT-2 // Usually the interrupt = pin -2 (on uno/nano anyway)

#define INTERVAL_DHT    900L  // seconds
#define INTERVAL_LIGHT   60L
#define FAST_LIGHT_CHANGE_DIFF 10 

MyMessage msgHum(CHILD_ID_HUM, V_HUM);
MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);
MyMessage msgMot(CHILD_ID_MOT, V_TRIPPED);
MyMessage msgLight(CHILD_ID_LIGHT, V_LIGHT_LEVEL);

unsigned long SLEEP_TIME = 30000; // Sleep time between reads (in milliseconds)

// MySensor gw;
DHT dht;
float lastTemp;
float lastHum;
//int   lightLevel;
int   lastLightLevel;
//int 	intervalDHT;
unsigned long lastDhtRead;
unsigned long lastLightRead;
unsigned long motionTimeout, motionTimer;
byte  state;


void setup()  
{ 
  char dimPeriod;
  Serial.println("setup");
  dimmer[0].begin(0, LED_PIN_1);
  dimmer[1].begin(1, LED_PIN_2);
  controller.set_callback(request_values);
  dht.setup(HUMIDITY_SENSOR_DIGITAL_PIN); 
  pinMode(MOTION_INPUT, INPUT);      // sets the motion sensor digital pin as input

 // load dimming time
  if ( (dimPeriod = loadState(CHILD_ID_DIM_1)) == 0xFF)
    dimPeriod = DIMMER_DEFAULT_DIMMING;
  saveState(CHILD_ID_DIM_1, dimPeriod);
  dimmer[0].setFadeIn(dimPeriod);
  
  if ( (dimPeriod = loadState(CHILD_ID_DIM_1 + 2)) == 0xFF)
    dimPeriod = DIMMER_DEFAULT_DIMMING;
  saveState(CHILD_ID_DIM_1 + 2, dimPeriod);
  dimmer[0].setFadeOut(dimPeriod);
  
  if ((dimPeriod = loadState(CHILD_ID_DIM_2)) == 0xFF)
    dimPeriod = DIMMER_DEFAULT_DIMMING;
  saveState(CHILD_ID_DIM_2, dimPeriod);
  dimmer[1].setFadeIn(dimPeriod);
  
  if ((dimPeriod = loadState(CHILD_ID_DIM_2 + 2)) == 0xFF)
    dimPeriod = DIMMER_DEFAULT_DIMMING;
  saveState(CHILD_ID_DIM_2 + 2, dimPeriod);
  dimmer[1].setFadeOut(dimPeriod);
  
}

uint8_t request_values(uint8_t set)
{
  // request the current values
  request(CHILD_ID_DIM_1, V_DIMMER);  
  // request(CHILD_ID_DIM_2, V_DIMMER);  
  Serial.println("Request values");
}


void presentation()  
{ 
  
  Serial.println("presentation  ");
  // Send the Sketch Version Information to the Gateway
  sendSketchInfo(SN, SV);

  present(CHILD_ID_DIM_1, S_DIMMER);  // present as dimmer
  present(CHILD_ID_DIM_2, S_DIMMER);  // present as dimmer


//  lastControllerMsgTime = millis();
 
  present(CHILD_ID_HUM, S_HUM);
  present(CHILD_ID_TEMP, S_TEMP);
  present(CHILD_ID_MOT, S_MOTION);
  present(CHILD_ID_LIGHT, S_LIGHT_LEVEL);
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
      /* Serial.print("Saving ");
      Serial.print(dimValue, DEC);
      Serial.print(" in ");
      Serial.println(CHILD_ID_DIM_1 + index); */
      dimmer[index].setFadeIn(dimValue);
      saveState(CHILD_ID_DIM_1 + index, dimValue);
      
      break;

    case V_VAR2:
      
      dimValue = atoi( message.data );
      // Clip incoming level to valid range of 0 to 100
      dimValue = dimValue > 100 ? 100 : dimValue;
      dimValue = dimValue < 0   ? 0   : dimValue; 
/* 
      Serial.print("Saving ");
      Serial.print(dimValue, DEC);
      Serial.print(" in ");
      Serial.println(CHILD_ID_DIM_1 + index + 2); */
      dimmer[index].setFadeOut(dimValue);
      saveState(CHILD_ID_DIM_1 + index + 2, dimValue);
      break;
    
    default:
      break;

  }
  Serial.println("receive");
  controller.answer();
}

void resend(MyMessage &msg, int repeats) // Resend messages if not received by node
{
  int repeat = 0;
  int repeatDelay = 0;
  boolean ack = false;

  while ((ack == false) and (repeat < repeats)) {
    if (send(msg)) {
      ack = true;
    } else {
      ack = false;
      repeatDelay += 100;
    } 
    repeat++;
    wait(repeatDelay);
  }
}

void pir_sensor(void)
{
  // Read digital motion value
  boolean motion = digitalRead(MOTION_INPUT) == HIGH; 
  if (lastMotion != motion) 
  {
    lastMotion = motion;     
    motionChange = true;
    Serial.print("motion ");
    Serial.println(motion ? "1" : "0" );
    resend(msgMot.set(motion ? "1" : "0" ), 5);  // Send motion value to gw
    
    if (motion)
    {
      motionTimeout = millis();
      // requestedLevel[0] = preferredLevel[0];
    }
  }
}

void dht_sensor(void)
{
  if (secTimeout(lastDhtRead, INTERVAL_DHT))
  {
    lastDhtRead = millis();
    delay(dht.getMinimumSamplingPeriod()); 
    float temperature = dht.getTemperature();
    if (isnan(temperature)) {
      Serial.println("Failed reading temperature from DHT");
    } 
    else// if (temperature != lastTemp) 
    {
      lastTemp = temperature;
      send(msgTemp.set(temperature, 1));
      Serial.print("T: ");
      Serial.println(temperature);
    }
  
    float humidity = dht.getHumidity();
    if (isnan(humidity)) {
      Serial.println("Failed reading humidity from DHT");
    } else // if (humidity != lastHum) 
    {
      lastHum = humidity;
      send(msgHum.set(humidity, 1));
      Serial.print("H: ");
      Serial.println(humidity);
    }
  }
  
}

void light_sensor(void)
{
  int delta;
  
  int lightLevel = (1023-analogRead(LIGHT_SENSOR_ANALOG_PIN))/10.23; 
  delta = lightLevel - lastLightLevel;
  
  if (secTimeout(lastLightRead, INTERVAL_LIGHT) || motionChange || (delta >= FAST_LIGHT_CHANGE_DIFF) || (delta <= -FAST_LIGHT_CHANGE_DIFF) )
  {
    motionChange = false;
    lastLightRead = millis();
    
 //   if (lightLevel != lastLightLevel) 
    {
      send(msgLight.set(lightLevel));
      lastLightLevel = lightLevel;
      Serial.print("L: ");
      Serial.println(lightLevel);
      
    }
  }
} 

void localFunctionality()
{
  switch (state)
  {
    case 0:   // idle
      if (digitalRead(MOTION_INPUT))
      {
        dimmer[0].setLevel(60);
        motionTimer = millis();
        state = 1;
      }
      break;
    
    case 1:   // lights on
      if (digitalRead(MOTION_INPUT))
        motionTimer = millis();
      else if (secTimeout(motionTimer, 30))
      {
        dimmer[0].setLevel(0);
        motionTimer = millis();
        state = 0;
      }
      break;
    
    default:
      break;
  }

}
  
void loop()      
{
  for (byte i = 0; i < 2; i++)
    dimmer[i].process();
  controller.monitor();
  pir_sensor();
  dht_sensor();
  light_sensor();
  if (!controller.alive())
    localFunctionality();
}



