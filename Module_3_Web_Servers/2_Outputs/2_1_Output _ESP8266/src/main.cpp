/*********
  Rui Santos
  Complete instructions at https://RandomNerdTutorials.com/build-web-servers-ebook/
  
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*********/

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>


// Replace with your network credentials
const char* ssid = "REPLACE_WITH_YOUR_SSID";
const char* password = "REPLACE_WITH_YOUR_PASSWORD";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Set LED GPIO
const int ledPin = 2;

// Stores LED state
String ledState;

// Initialize LitleFS
void initFS() {
  if (!LittleFS.begin()) {
    Serial.println("An error has occurred while mounting LitleFS");
  }
  Serial.println("LitleFS mounted successfully");
}

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}

// Replaces placeholder with LED state value
String processor(const String& var) {
  if(var == "STATE") {
    if(digitalRead(ledPin)) {
      ledState = "OFF";
    }
    else {
      ledState = "ON";
    }
    return ledState;
  }
  return String();
}

void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);
  initWiFi();
  initFS();
  
  // Set GPIO2 as an OUTPUT
  pinMode(2, OUTPUT);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.html", "text/html",false, processor);
  });

  server.serveStatic("/", LittleFS, "/");

    // Route to set GPIO state to HIGH (inverted logic for ESP8266)
  server.on("/on", HTTP_GET, [](AsyncWebServerRequest *request) {
    digitalWrite(ledPin, LOW);
    request->send(LittleFS, "/index.html", "text/html", false, processor);
  }); 

  // Route to set GPIO state to LOW (inverted logic for ESP8266)
  server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request) {
    digitalWrite(ledPin, HIGH);
    request->send(LittleFS, "/index.html", "text/html", false, processor);
  });

  server.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
}