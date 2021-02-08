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
#include "LittleFS.h"
#include <Arduino_JSON.h>
#include <Adafruit_BME280.h>
#include <Adafruit_Sensor.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// Replace with your network credentials
const char* ssid = "REPLACE_WITH_YOUR_SSID";
const char* password = "REPLACE_WITH_YOUR_PASSWORD";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Create an Event Source on /events
AsyncEventSource events("/events");

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

// Json Variable to Hold Sensor Readings
JSONVar readings;

// File name where readings will be saved
const char* dataPath = "/data.txt";

// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 1800000;

// Create a sensor object
Adafruit_BME280 bme;         // BME280 connect to ESP32 I2C (GPIO 21 = SDA, GPIO 22 = SCL)

// Init BME280
void initBME(){
  if (!bme.begin(0x76)) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }
}

// Function that gets current epoch time
unsigned long getTime() {
  timeClient.update();
  unsigned long now = timeClient.getEpochTime();
  return now;
}

// Get Sensor Readings and return JSON object
String getSensorReadings(){
  readings["time"] = String(getTime());
  readings["temperature"] = String(bme.readTemperature());
  readings["humidity"] =  String(bme.readHumidity());
  readings["pressure"] = String(bme.readPressure()/100.0F);
  String jsonString = JSON.stringify(readings);
  return jsonString;
}

// Initialize LittleFS
void initFS() {
  if (!LittleFS.begin()) {
    Serial.println("An error has occurred while mounting LittleFS");
  }
  Serial.println("LittleFS mounted successfully");
}

// Read file from LittleFS
String readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path, "r");
  if(!file || file.isDirectory()){
    Serial.println("- failed to open file for reading");
    return String();
  }
  
  String fileContent;
  while(file.available()){
    fileContent += file.readStringUntil('\n');
    break;     
  }
  file.close();
  return fileContent;
}

// Append data to file in LittleFS
void appendFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Appending to file: %s\r\n", path);

  File file = fs.open(path, "a");
  if(!file){
    Serial.println("- failed to open file for appending");
    return;
  }
  if(file.print(message)){
    Serial.println("- message appended");
  } else {
    Serial.println("- append failed");
  }
  file.close();
}

// Delete File
void deleteFile(fs::FS &fs, const char * path){
  Serial.printf("Deleting file: %s\r\n", path);
  if(fs.remove(path)){
    Serial.println("- file deleted");
  } else {
    Serial.println("- delete failed");
  }
}

// Get file size
int getFileSize(fs::FS &fs, const char * path){
  File file = fs.open(path, "r");

  if(!file){
    Serial.println("Failed to open file for checking size");
    return 0;
  }
  Serial.print("File size: ");
  Serial.println(file.size());

  return file.size();
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

void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);
  Serial.print("ok");
  initBME();
  initWiFi();
  initFS();

  // Initialize a NTPClient to get time
  timeClient.begin();
  // Set offset time in seconds to adjust for your timezone, for example:
  // GMT +1 = 3600
  // GMT +8 = 28800
  // GMT -1 = -3600
  // GMT 0 = 0
  timeClient.setTimeOffset(0);

  // Create a data.txt file
  File file = LittleFS.open(dataPath, "r");
  if(!file) {
    Serial.println("File doens't exist");
    Serial.println("Creating file...");
    // Prepare readings to add to the file
    String message = getSensorReadings() + ",";
    // Apend data to file to create it
    appendFile(LittleFS, dataPath, message.c_str());
  }
  else {
    Serial.println("File already exists");  
  }
  file.close();

  // Web Server Root URL
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/index.html", "text/html");
  });

  server.serveStatic("/", LittleFS, "/");

  // Request for the latest sensor readings
  server.on("/readings", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/data.txt", "text/txt");
  });

  // Request for raw data
  server.on("/view-data", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/data.txt", "text/txt");
  });

  // Request to delete data.txt file
  server.on("/delete-data", HTTP_GET, [](AsyncWebServerRequest *request){
    deleteFile(LittleFS, dataPath);
    request->send(200, "text/plain", "data.txt has been deleted.");
  });

  events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    // send event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    client->send("hello!", NULL, millis(), 10000);
  });
  server.addHandler(&events);

  // Start server
  server.begin();

  events.send(getSensorReadings().c_str(),"new_readings", millis());

}

void loop() {
  if ((millis() - lastTime) > timerDelay) {
    // Send Events to the client with the Sensor Readings Every 30 minutes
    events.send("ping",NULL,millis());
    events.send(getSensorReadings().c_str(),"new_readings" ,millis());
    String message = getSensorReadings() + ",";

    if ((getFileSize(LittleFS, dataPath))>= 3400){
      Serial.print("Too many data points, deleting file...");
      // Uncomment the next two lines if you don't want to delete the data file automatically.
      // It won't log more data into the file
      deleteFile(LittleFS, dataPath);
      appendFile(LittleFS, "/data.txt", message.c_str());
    }
    else{
      // Append new readings to the file
      appendFile(LittleFS, "/data.txt", message.c_str());
    }

    lastTime = millis();

    Serial.print(readFile(LittleFS, dataPath));
  }
}