/*********
  Rui Santos
  Complete instructions at https://RandomNerdTutorials.com/build-web-servers-ebook/
  
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*********/

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

// Replace with desired access point credentials
const char* ssid     = "ESP32-Access-Point";
const char* password = "123456789";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>ESP Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="icon" href="data:,">
  <style>
    html {
      font-family: Arial;
      text-align: center;
    }
    body {
      max-width: 400px;
      margin:0px auto;
    }
  </style>
</head>
<body>
  <h1>Hello World!</h1>
  <p>Congratulations!<br>This is your first Web Server with the ESP.</p>
</body>
</html>
)rawliteral";

void initAP() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
}

void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);
  initAP();
  
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", index_html);
  });

  // Start server
  server.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
}