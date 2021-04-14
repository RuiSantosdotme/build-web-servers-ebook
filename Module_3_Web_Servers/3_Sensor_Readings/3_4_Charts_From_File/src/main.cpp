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
#include <Adafruit_BME280.h>
#include <Adafruit_Sensor.h>
#include "time.h"
#include <WiFiUdp.h>


// Replace with your network credentials
const char* ssid = "REPLACE_WITH_YOUR_SSID";
const char* password = "REPLACE_WITH_YOUR_PASSWORD";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Create an Event Source on /events
AsyncEventSource events("/events");

// NTP server to request epoch time
const char* ntpServer = "pool.ntp.org";

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
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    //Serial.println("Failed to obtain time");
    return(0);
  }
  time(&now);
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

// Initialize SPIFFS
void initSPIFFS() {
  if (!SPIFFS.begin()) {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  Serial.println("SPIFFS mounted successfully");
}

// Read file from SPIFFS
String readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path);
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

// Append data to file in SPIFFS
void appendFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Appending to file: %s\r\n", path);

  File file = fs.open(path, FILE_APPEND);
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
  File file = fs.open(path);

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
  initSPIFFS();

  // Create a data.txt file
  bool fileexists = SPIFFS.exists(dataPath);
  Serial.print(fileexists);
  if(!fileexists) {
    Serial.println("File doesn't exist");
    Serial.println("Creating file...");
    // Prepare readings to add to the file
    String message = getSensorReadings() + ",";
    // Apend data to file to create it
    appendFile(SPIFFS, dataPath, message.c_str());
  }
  else {
    Serial.println("File already exists");
  }

  // Web Server Root URL
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", "text/html");
  });

  server.serveStatic("/", SPIFFS, "/");

  // Request for the latest sensor readings
  server.on("/readings", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/data.txt", "text/txt");
  });

  // Request for the latest sensor readings
  server.on("/view-data", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/data.txt", "text/txt");
  });

  // Request for the latest sensor readings
  server.on("/delete-data", HTTP_GET, [](AsyncWebServerRequest *request){
    deleteFile(SPIFFS, dataPath);
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

  configTime(0, 0, ntpServer);

  // Start server
  server.begin();

  events.send(getSensorReadings().c_str(),"new_readings" ,millis());
}

void loop() {
if ((millis() - lastTime) > timerDelay) {

    // Send Events to the client with the Sensor Readings
    events.send("ping",NULL,millis());
    events.send(getSensorReadings().c_str(),"new_readings" ,millis());

    String message = getSensorReadings() + ",";
    if ((getFileSize(SPIFFS, dataPath))>= 3400){
      Serial.print("Too many data points, deleting file...");
      // Comment the next two lines if you don't want to delete the data file automatically.
      // It won't log more data into the file
      deleteFile(SPIFFS, dataPath);
      appendFile(SPIFFS, "/data.txt", message.c_str());
    }
    else{
      // Append new readings to the file
      appendFile(SPIFFS, "/data.txt", message.c_str());
    }

    lastTime = millis();

    Serial.print(readFile(SPIFFS, dataPath));

  }
}