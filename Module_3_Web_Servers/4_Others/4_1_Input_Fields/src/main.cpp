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
#include "SPIFFS.h"
#include <Arduino_JSON.h>

// Replace with your network credentials
const char* ssid = "REPLACE_WITH_YOUR_SSID";
const char* password = "REPLACE_WITH_YOUR_PASSWORD";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Search for parameter in HTTP POST request
const char* PARAM_INPUT_1 = "input1";
const char* PARAM_INPUT_2 = "input2";

//Variables to save values from HTML form
String input1;
String input2;

// File paths to save input values permanently
const char* input1Path = "/input1.txt";
const char* input2Path = "/input2.txt";

JSONVar values;

// Initialize SPIFFS
void initSPIFFS() {
  if (!SPIFFS.begin(true)) {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  else {
    Serial.println("SPIFFS mounted successfully");
  }
}

// Read File from SPIFFS
String readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path);
  if(!file || file.isDirectory()){
    Serial.println("- failed to open file for reading");
    return String();
  }
  
  String fileContent;
  while(file.available()){
    fileContent = file.readStringUntil('\n');
    break;     
  }
  return fileContent;
}

// Write file to SPIFFS
void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, FILE_WRITE);
  if(!file){
    Serial.println("- failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("- file written");
  } else {
    Serial.println("- frite failed");
  }
}

// Initialize WiFi
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

String getCurrentInputValues(){
  values["textValue"] = input1;
  values["numberValue"] = input2;
  String jsonString = JSON.stringify(values);
  return jsonString;
}

void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);
  initWiFi();
  initSPIFFS();

  // Load values saved in SPIFFS
  input1 = readFile(SPIFFS, input1Path);
  input2 = readFile(SPIFFS, input2Path);

  // Web Server Root URL
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", "text/html");
  });
  
  server.serveStatic("/", SPIFFS, "/");

  server.on("/values", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = getCurrentInputValues();
    request->send(200, "application/json", json);
    json = String();
  });

  
  server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
    int params = request->params();
    for(int i=0;i<params;i++){
      AsyncWebParameter* p = request->getParam(i);
      if(p->isPost()){
        // HTTP POST input1 value
        if (p->name() == PARAM_INPUT_1) {
          input1 = p->value().c_str();
          Serial.print("Input 1 set to: ");
          Serial.println(input1);
          // Write file to save value
          writeFile(SPIFFS, input1Path, input1.c_str());
        }
        // HTTP POST input2 value
        if (p->name() == PARAM_INPUT_2) {
          input2 = p->value().c_str();
          Serial.print("Input 2 set to: ");
          Serial.println(input2);
          // Write file to save value
          writeFile(SPIFFS, input2Path, input2.c_str());
        }
        //Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
      }
    }
    request->send(SPIFFS, "/index.html", "text/html");
  });

  server.begin();
}

void loop() {
  /*Serial.println(readFile(SPIFFS, input1Path));
  Serial.println(readFile(SPIFFS, input2Path));
  delay(10000);*/
}
