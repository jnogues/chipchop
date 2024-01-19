#include <Arduino.h>
// define max number of tasks to save precious Arduino RAM
#define TASKER_MAX_TASKS 10
#include "Tasker.h"
#include <Temperature_LM75_Derived.h>
//Generic_LM75 temperature;
Generic_LM75_12Bit temperature;
#include <Adafruit_NeoPixel.h>

// Which pin on the Arduino is connected to the NeoPixels?
#define PIN 15 // On Trinket or Gemma, suggest changing this to 1
#define NUMPIXELS 1// Popular NeoPixel ring size
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

#ifdef ESP32
  #include <WiFi.h>
  int BAUD_RATE = 115200;
  
#elif defined(ARDUINO_UNOR4_WIFI)
    #include <WiFiS3.h>
    int BAUD_RATE = 9600;

#elif ESP8266
  #include <ESP8266WiFi.h>
  int BAUD_RATE = 9600;

#endif

Tasker tasker;

////// ***** ChipChop definitions  ***** ///////
#include <ChipChopManager.h> 
ChipChopManager ChipChop;

String server_uri = "...your allocated Endpoint Server URI..."; //found in the Dev Console > My Account
String uuid = "...your ChipChop UUID ..."; //found in the Dev Console > My Account
String auth_code = "... your Security Authentication Code ..."; //found in the Dev Console > My Account

String device_id = "... your Device ID ..."; // same id as in the Dev Console > Devices for this device

////// ***** Led variables ***** //////

String gpio13_status = "OFF"; // the device will start with the led turned off
String gpio0_status = "OFF"; // the device will start with the led turned off
String gpio2_status = "OFF";
String gpio12_status = "OFF";
bool gpio12_now = 0;
bool gpio12_before = 0;

float t_LM75 = 0.0;

String neopixel_status = "0, 0, 0";
int red=0;
int green=0;
int blue=0;
int dimmer=10;
String dimmer_status = "10";
float factor = 1.0;
float redFloat = 0.0;
float greenFloat = 0.0;
float blueFloat = 0.0;

// ChipChop callback function triggered every time you send a command to your device
// this callback will handle the switching of the led on/off
void ChipChop_onCommandReceived(String target_component,String command_value, String command_source, int command_age){

    if(target_component == "GPIO13")
    {
        if(command_value == "ON")
        {

            gpio13_status = "ON"; //update the status
            digitalWrite(13,HIGH); // turn the led on

        }
        else if(command_value == "OFF")
        {
           gpio13_status = "OFF";//update the status
            digitalWrite(13,LOW); // turn the led off
        }
    }
    
    if(target_component == "GPIO0")
    {
        if(command_value == "ON")
        {

            gpio0_status = "ON"; //update the status
            digitalWrite(0,LOW); // turn the led on

        }
        else if(command_value == "OFF")
        {
           gpio0_status = "OFF";//update the status
           digitalWrite(0,HIGH); // turn the led off
        }    

    }

    if(target_component == "GPIO2")
    {
        if(command_value == "ON")
        {

            gpio2_status = "ON"; //update the status
            digitalWrite(2,LOW); // turn the led on

        }
        else if(command_value == "OFF")
        {
           gpio2_status = "OFF";//update the status
           digitalWrite(2,HIGH); // turn the led off
        }    
    }

    if(target_component == "neopixel")
    {
      Serial.println(command_value);
      neopixel_status = command_value;
      int firstCommaIndex = command_value.indexOf(",");
      int secondCommaIndex = command_value.indexOf(",", firstCommaIndex+1);
      int lastClaudatorIndex = command_value.indexOf("]");

      String red_ = command_value.substring(1, firstCommaIndex);
      red = red_.toInt();
      String green_ = command_value.substring(firstCommaIndex+1, secondCommaIndex);
      green = green_.toInt();
      String blue_ = command_value.substring(secondCommaIndex+1, lastClaudatorIndex);
      blue = blue_.toInt();
      pixels.setPixelColor(0, pixels.Color(red, green, blue));
      pixels.show();
      Serial.print(red); Serial.print(" ");
      Serial.print(green); Serial.print(" ");
      Serial.print(blue); Serial.println(" ");      
    }

    if(target_component == "pwm")
    {
      dimmer_status = command_value;
      dimmer = dimmer_status.toInt();
      Serial.println(dimmer);
      //To Do, use some gpio with pwm
    }

    ChipChop.updateStatus("GPIO13",gpio13_status); // Confirm the LED status change
    ChipChop.updateStatus("GPIO0",gpio0_status); // 
    ChipChop.updateStatus("GPIO2",gpio2_status); // 
    ChipChop.updateStatus("neopixel",neopixel_status); // 
    ChipChop.updateStatus("pwm",dimmer_status); //  

}


void setup()
{
    Serial.begin(BAUD_RATE);
    Wire.begin();

    WiFi.begin("mySSID", "myPASS");

    Serial.print("WiFi Connecting");
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    pinMode(16, OUTPUT);
    pinMode(13,OUTPUT); // declare the led pin as an output
    pinMode(0,OUTPUT); 
    pinMode(2,OUTPUT); 
    pinMode(12, INPUT);
    digitalWrite(16,HIGH);
    digitalWrite(13,LOW); // turn the led off
    digitalWrite(0,HIGH);
    digitalWrite(2,HIGH);

    pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
    pixels.clear(); // Set all pixel colors to 'off'
    pixels.setPixelColor(0, pixels.Color(255, 0, 0));
    pixels.show();
    delay(1000);
    pixels.setPixelColor(0, pixels.Color(0, 255, 0));
    pixels.show();
    delay(1000);
    pixels.setPixelColor(0, pixels.Color(0, 0, 255));
    pixels.show();
    delay(1000);
    pixels.setPixelColor(0, pixels.Color(0, 0, 0));
    pixels.show();
    delay(1000);

    pixels.show();   // Send the updated pixel colors to the hardware.

    //*** declare ChipChop utility callbacks, functions & settings ***//
    ChipChop.commandCallback(ChipChop_onCommandReceived);
    ChipChop.start(server_uri, uuid, device_id, auth_code);
    ChipChop.hearBeatInterval(10);
    ChipChop.debug(true); // enables log messages from the ChipChop library, set to false in production
   
    tasker.setInterval(blinkLed16, 1000);
    tasker.setInterval(update2server, 10000);
    tasker.setInterval(triggers2server, 100);
    tasker.setInterval(readTemperatureLM75, 15000);
}


void loop()
{
  ChipChop.run(); // This call has to be here in the main loop() and needs to run continously
  tasker.loop();//gestio de les tasques
}


//tasK 1
void blinkLed16()
{
  digitalWrite(16, !digitalRead(16));
}

//task 2
void update2server()
{
  if(WiFi.status() == WL_CONNECTED)
    { // we don't send anything unless the WiFi is connected

        ChipChop.updateStatus("GPIO13",gpio13_status); // Update the LED status continously
        ChipChop.updateStatus("GPIO0",gpio0_status);
        ChipChop.updateStatus("GPIO2",gpio2_status);
        ChipChop.updateStatus("upTime",int(millis()/1000)); // Update the LED status continously
        ChipChop.updateStatus("A0",int(analogRead(A0))); 
        ChipChop.updateStatus("GPIO12", gpio12_status); 
        ChipChop.updateStatus("tempLM75", float(t_LM75));
        ChipChop.updateStatus("neopixel",neopixel_status); // 
        ChipChop.updateStatus("pwm",dimmer_status); //   
    }
}

//task 3
void triggers2server()
{
  gpio12_now = digitalRead(12);
  if ( (gpio12_now == 1) && (gpio12_before == 0) )
  {
    gpio12_status = "ON";
    Serial.println("Dispara ON");
    if(WiFi.status() == WL_CONNECTED) ChipChop.triggerEvent("GPIO12", gpio12_status);//quick response
  }
  
  else if ( (gpio12_now == 0) && (gpio12_before == 1) )
  {
    gpio12_status = "OFF";
    Serial.println("Dispara OFF");
    //if(WiFi.status() == WL_CONNECTED) ChipChop.triggerEvent("GPIO12", gpio12_status);//quick response
  } 
  
  else 
  {
    if (gpio12_now == 1) gpio12_status = "ON";
    else gpio12_status = "OFF";
  }
  
  gpio12_before = gpio12_now; 
}

 //task 4
 void readTemperatureLM75()
 {
  t_LM75 = temperature.readTemperatureC();
  Serial.print("Temperature = ");
  Serial.print(t_LM75);
  Serial.println(" C");
 }
