/*************************************************************************
Title:    WiFi-Enabled Searchlight Signal Controller
Authors:  Nathan D. Holmes <maverick@drgw.net>
File:     $Id: $
License:  GNU General Public License v3

LICENSE:
    Copyright (C) 2020 Nathan D. Holmes (maverick@drgw.net)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

*************************************************************************/

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <Wire.h>
#include <SPI.h>
extern "C" {
  #include "user_interface.h"
}

#define LIGHT_ENABLE_PIN    15
#define COIL_A_PIN          12
#define COIL_B_PIN          13

#define BLUE_LED              2


const char* ssid     = "YOUR-WIRELESS-NETWORK";
const char* password = "YOUR-WIRELESS-PASSWORD";
const char* nodeName = "backyard-h2";

ESP8266WebServer server(80);
WiFiClient espClient;

uint8_t relay_coilA = 0;
uint8_t relay_coilB = 0;
uint8_t relay_lamp = 0;

void wifiSetup()
{
  WiFi.mode(WIFI_STA); // Set to station mode - disable broadcasting AP SSID offers

  // Connect to WiFi network
  wifi_station_set_hostname((char*)nodeName);
  WiFi.begin(ssid, password);
  Serial.print("\n\r \n\rWorking to connect");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
}

typedef enum
{
  ASPECT_OFF = 0,
  ASPECT_RED = 1,
  ASPECT_YELLOW = 2,
  ASPECT_GREEN = 3,
  ASPECT_FL_RED = 4,
  ASPECT_FL_YELLOW = 5,
  ASPECT_FL_GREEN = 6
  
} SignalAspect;

SignalAspect signalAspect = ASPECT_OFF;

uint8_t setAspect(String aspect)
{
  uint8_t error = 0;
  
  if (aspect == "red")
    signalAspect = ASPECT_RED;
  else if (aspect == "yellow")
    signalAspect = ASPECT_YELLOW;
  else if (aspect == "green")
    signalAspect = ASPECT_GREEN;
  else if (aspect == "flred")
    signalAspect = ASPECT_FL_RED;
  else if (aspect == "flyellow")
    signalAspect = ASPECT_FL_YELLOW;
  else if (aspect == "flgreen")
    signalAspect = ASPECT_FL_GREEN;
  else if (aspect == "off")
    signalAspect = ASPECT_OFF;
  else
    error = 1;

  return error;
}

void handle_root() 
{
  for (int i = 0; i < server.args(); i++) 
    if (server.argName(i) == "aspect")
      setAspect(server.arg(i));
  
  String html = "<html><title>Signal Controller " + String(nodeName) + "</title><body>";
  html += "<h3>Signal Controller " + String(nodeName) + "</h3><br />";
  html += "Current aspect [";
  switch(signalAspect)
  {
    case ASPECT_OFF:
      html += "OFF";
      break;
    case ASPECT_RED:
      html += "RED";
      break;
    case ASPECT_YELLOW:
      html += "YELLOW";
      break;
    case ASPECT_GREEN:
      html += "GREEN";
      break;
    case ASPECT_FL_RED:
      html += "FLASHING RED";
      break;
    case ASPECT_FL_YELLOW:
      html += "FLASHING YELLOW";
      break;
    case ASPECT_FL_GREEN:
      html += "FLASHING GREEN";
      break;
  }

  html += "]<br /><br /><form action='/' method='get'>";

  html += "<input type='radio' name='aspect' value='off'";
  if (ASPECT_OFF == signalAspect)
    html += " checked";
  html += ">Off</input><br />\n";
  
  html += "<input type='radio' name='aspect' value='red'";
  if (ASPECT_RED == signalAspect)
    html += " checked";
  html += ">Red</input><br />\n";
  
  html += "<input type='radio' name='aspect' value='yellow'";
  if (ASPECT_YELLOW == signalAspect)
    html += " checked";
  html += ">Yellow</input><br />\n";
  
  html += "<input type='radio' name='aspect' value='green'";
  if (ASPECT_GREEN == signalAspect)
    html += " checked";
  html += ">Green</input><br />\n";
  
  html += "<input type='radio' name='aspect' value='flred'";
  if (ASPECT_FL_RED == signalAspect)
    html += " checked";
  html += ">Flashing Red</input><br />\n";

  html += "<input type='radio' name='aspect' value='flyellow'";
  if (ASPECT_FL_YELLOW == signalAspect)
    html += " checked";
  html += ">Flashing Yellow</input><br />\n";
  
  
  html += "<input type='radio' name='aspect' value='flgreen'";
  if (ASPECT_FL_GREEN == signalAspect)
    html += " checked";
  html += ">Flashing Green</input><br />\n";
  html += "<input type='submit' value='Change Aspect'></input></form></body></html>\n";
  
  server.send(200, "text/html", html);
  delay(100);
}


void handleSetLights() 
{ 
  String message = "";
  uint8_t error = 0; 
  char buffer[256];
  message = "Ok\n";

  for (int i = 0; i < server.args(); i++) 
  {
    if (server.argName(i) == "aspect")
    {
      String aspect = server.arg(i);
      error = setAspect(aspect);
      if (!error)
        snprintf(buffer, sizeof(buffer), "set aspect to [%s]", aspect.c_str());
      else
        snprintf(buffer, sizeof(buffer), "unknown aspect [%s]", aspect.c_str());
    }
  }


  message += buffer;
  server.send(error?400:200, "text/plain", message);          //Returns the HTTP response
}


void setup(void)
{
  // You can open the Arduino IDE Serial Monitor window to see what the code is doing
  
  Serial1.begin(9600);  // Serial connection from ESP-01 via 3.3v console cable
  wifiSetup(); 
  Serial1.print("");
  Serial1.print("US&S H2 Signal Control Node");

  
  server.on("/", handle_root);
  server.on("/setAspect", handleSetLights);
 
  server.begin();

  Serial1.print("HTTP server started");
  pinMode(BLUE_LED, OUTPUT);
  
  pinMode(LIGHT_ENABLE_PIN, OUTPUT);
  pinMode(COIL_A_PIN, OUTPUT);
  pinMode(COIL_B_PIN, OUTPUT);

  digitalWrite(LIGHT_ENABLE_PIN, 1);
  digitalWrite(COIL_A_PIN, 1);
  digitalWrite(COIL_B_PIN, 1);
}

#define DECISEC_LIMIT 14

void loop(void)
{
  static uint8_t decisecs = 0;
  delay(100);
  server.handleClient();
  decisecs++;
  if (decisecs == (DECISEC_LIMIT/2) || decisecs == DECISEC_LIMIT)
  {
    uint8_t blinky = (decisecs==7)?0:1;
    digitalWrite(BLUE_LED, blinky);

    if (decisecs == DECISEC_LIMIT)
      decisecs = 0;
    
    switch(signalAspect)
    {
      case ASPECT_OFF:
        digitalWrite(LIGHT_ENABLE_PIN, 0);
        digitalWrite(COIL_A_PIN, 1);
        digitalWrite(COIL_B_PIN, 1);
        break;

      case ASPECT_RED:
        digitalWrite(LIGHT_ENABLE_PIN, 1);
        digitalWrite(COIL_A_PIN, 1);
        digitalWrite(COIL_B_PIN, 1);
        break;
      case ASPECT_YELLOW:
        digitalWrite(LIGHT_ENABLE_PIN, 1);
        digitalWrite(COIL_A_PIN, 0);
        digitalWrite(COIL_B_PIN, 1);
        break;
      case ASPECT_GREEN:
        digitalWrite(LIGHT_ENABLE_PIN, 1);
        digitalWrite(COIL_A_PIN, 1);
        digitalWrite(COIL_B_PIN, 0);
        break;
      case ASPECT_FL_RED:
        digitalWrite(LIGHT_ENABLE_PIN, blinky);
        digitalWrite(COIL_A_PIN, 1);
        digitalWrite(COIL_B_PIN, 1);
        break;
      case ASPECT_FL_YELLOW:
        digitalWrite(LIGHT_ENABLE_PIN, blinky);
        digitalWrite(COIL_A_PIN, 0);
        digitalWrite(COIL_B_PIN, 1);
        break;
      case ASPECT_FL_GREEN:
        digitalWrite(LIGHT_ENABLE_PIN, blinky);
        digitalWrite(COIL_A_PIN, 1);
        digitalWrite(COIL_B_PIN, 0);
        break;
    }
  }
  
}   
