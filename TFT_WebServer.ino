#include <Arduino.h>
static const byte DHT22espPin = 32;
static const byte MQ135espPin = 35;
static const byte TCAPMSAddress = 4;
static const byte TCAMLXAddress = 3;
static const byte TCAMAXAddress = 2;
static const byte MLXespLEDPin = 5;
static const byte MAXespLEDPin = 18;
const char* ssid = "Oreo Dog";
const char* password = "Oreo20201029?";
const char* roomSensorServer = "http://192.168.1.3:3000/room_sensor";
const char* mlxSensorServer = "http://192.168.1.3:3000/recordMLX";
const char* maxSensorServer = "http://192.168.1.3:3000/recordMAX";
bool maxActive = false;
bool mlxActive = false;
bool initScreen = false;
bool initMLXScreen = true;
bool initMAXScreen = true;

// Time configuration
#include "time.h"
const char* ntpServer1 = "ntp.pagasa.dost.gov.ph";
const char* ntpServer2 = "time.upd.edu.ph";
const long  gmtOffset_sec = 28800;
const int   daylightOffset_sec = 3600;
struct tm timeinfo;
char formatteddatetime[64] = "";

// Instantiate TFT Sprites
#include <TFT_eSPI.h>           // TFT SPI
#include "Free_Fonts.h"         // TFT Font
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite tempRoomDisplay = TFT_eSprite(&tft);
TFT_eSprite humidityDisplay = TFT_eSprite(&tft);
TFT_eSprite airqualityDisplay = TFT_eSprite(&tft);
TFT_eSprite pm1Display = TFT_eSprite(&tft);
TFT_eSprite pm2_5Display = TFT_eSprite(&tft);
TFT_eSprite pm10Display = TFT_eSprite(&tft);
TFT_eSprite timedateDisplay = TFT_eSprite(&tft);
TFT_eSprite bodyTempDisplay = TFT_eSprite(&tft);
TFT_eSprite heartRateDisplay = TFT_eSprite(&tft);
TFT_eSprite spo2Display = TFT_eSprite(&tft);


// TCA9548A Multiplexer
#include <Wire.h>
void TCA9548A(uint8_t bus) {
  Wire.beginTransmission(0x70);  // TCA9548A address is 0x70
  Wire.write(1 << bus);          // send byte to select bus
  Wire.endTransmission();
  Serial.print("Bus: ");
  Serial.println(bus);
}

// DHT22
#include <DHT.h>
DHT DHT22Sensor(DHT22espPin, DHT22);
float temperature, humidity;

// MQ135
#include "MQ135.h"
MQ135 MQ135Sensor(MQ135espPin);
float air_quality;

// PMS70300
#include <SoftwareSerial.h>
SoftwareSerial PMSSensor(21, 22); // TX,RX SCL,SDA
unsigned int pm1 = 999;
unsigned int pm2_5 = 999;
unsigned int pm10 = 999;

// MLX90614
#include <DFRobot_MLX90614.h>
DFRobot_MLX90614_I2C MLXSensor;
float fingerTemp, ambientTemp;

// MAX30102
#include <DFRobot_MAX30102.h>
DFRobot_MAX30102 MAXSensor;
const byte RATE_SIZE = 4;
byte rates[RATE_SIZE];
byte rateSpot = 0;
int Spo2Values[5];
int Spo2Spot = 0;
bool shouldPrintAvg = false;

int32_t SPO2;
int8_t SPO2Valid;
int32_t heartRate;
int8_t heartRateValid;
int beatAvg;
int averageSpo2;


// Create AsyncWebServer object on port 80
#include <ESPAsyncWebServer.h>
AsyncWebServer server(80);

// Create an Event Source on /events
#include <AsyncTCP.h>
AsyncEventSource events("/events");

// Json Variable to Hold Sensor Readings
#include <Arduino_JSON.h>
JSONVar readings;

// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 2000;

// Initialize SPIFFS
#include "SPIFFS.h"
void initSPIFFS() {
  if (!SPIFFS.begin()) {
    Serial.println("An error has occurred while mounting SPIFFS");
  } else {
    Serial.println("SPIFFS mounted successfully");
  }
}

// Initialize WiFi
#include <WiFi.h>
#include <HTTPClient.h>
void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
}

void initializeTFTheader(void){
  // Draw header
  tft.init();
  tft.setRotation(1);    
  tft.fillScreen(TFT_BLACK);                    // Screen background
  tft.setTextDatum(MC_DATUM);                   // Set text reference coordinates to middle center
  tft.setFreeFont(FSS9);                        // Fonts for header
  tft.setTextColor(TFT_MAGENTA);
  tft.drawString("VITAL SIGNS AND AIR QUALITY MONITORING", 240, 20, GFXFF);
  tft.drawString("PALTOC HEALTH CENTER - SAMPALOC, MANILA", 240, 40, 2);
  if (mlxActive == true){
    tft.drawCircle(240, 180, 130, TFT_CYAN); 
    tft.setFreeFont(FSSB9); 
    tft.setTextColor(TFT_WHITE);
    tft.drawString("TEMPERATURE (  C)", 240, 120, GFXFF);
    tft.drawCircle(307, 118, 3, TFT_WHITE); 
  }
  else if (maxActive == true){
    tft.drawRect(0, 60, 480, 110, TFT_CYAN);
    tft.drawRect(0, 171, 480, 110, TFT_GREEN);
    
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_BLACK);
    tft.setFreeFont(FSSB9); 
    tft.fillRect(375, 60, 50, 30, TFT_CYAN);
    tft.drawString("BPM", 400, 74, GFXFF);
    tft.fillRect(375, 171, 50, 30, TFT_GREEN);
    tft.drawString("%", 400, 185, GFXFF);

    tft.setTextDatum(MR_DATUM);
    tft.setFreeFont(FSSB9); 
    tft.setTextColor(TFT_WHITE);
    tft.drawString("Heart Rate", 165, 110, GFXFF);
    tft.drawString("Blood Oxygen", 165, 210, GFXFF);  
    tft.drawString("Saturation", 165, 230, GFXFF);  

  }
}

void initializeTFTbackground(void){
  // Draw sensors borders
  tft.setTextColor(0x077F);
  tft.setTextDatum(TL_DATUM);                   // Set text reference coordinates to top left
 
  tft.drawRect(0, 60, 159, 110, 0x077F);     // Top left border
  tft.drawString("TEMP", 5, 97, 2);             // Temperature label
  tft.drawCircle(27, 114, 2, 0x077F);        // Celcius circle
  tft.drawString("C", 30, 110, 2);              // Unit label

  tft.drawRect(160, 60, 159, 110, 0x077F);   // Top mid border
  tft.drawString("HUMIDITY", 165, 80, 2);       // Humidity label
  tft.drawString("%", 300, 115, 2);             // Unit label

  tft.drawRect(320, 60, 159, 110, 0x077F);   // Top right border
  tft.drawString("CO2", 335, 97, 2);            // CO2 label
  tft.drawString("PPM", 332, 110, 2);           // Unit label

  tft.drawRect(0, 171, 159, 110, 0x077F);    // Bottom left border
  tft.drawString("PM1", 112, 208, 2);           // PM1 label
  tft.drawString("ug/m3", 100, 224, 2);         // Unit label

  tft.drawRect(160, 171, 159, 110, 0x077F);  // Bottom mid border
  tft.drawString("PM2.5", 260, 208, 2);         // PM2.5 label
  tft.drawString("ug/m3", 261, 224, 2);         // Unit label

  tft.drawRect(320, 171, 159, 110, 0x077F);  // Bottom right border
  tft.drawString("PM10", 423, 208, 2);          // PM10 label
  tft.drawString("ug/m3", 420, 224, 2);         // Unit label
}

void initializeEnviSprites(void){
  // Max Sprite Size (1 only)
  // Max    = (263, 218) = 57334 px
  // Height = (179, 320) = 57280 px
  // Width  = (480, 119) = 57120 px
  // Square = (239, 239) = 57121 px
  tempRoomDisplay.createSprite(117, 38);
  tempRoomDisplay.setTextDatum(TR_DATUM);
  tempRoomDisplay.setFreeFont(FF24);

  humidityDisplay.createSprite(117, 38);
  humidityDisplay.setTextDatum(TR_DATUM);
  humidityDisplay.setFreeFont(FF24);

  airqualityDisplay.createSprite(105, 38);
  airqualityDisplay.setTextDatum(TR_DATUM);
  airqualityDisplay.setFreeFont(FF24);

  pm1Display.createSprite(78, 38);
  pm1Display.setTextDatum(TR_DATUM);
  pm1Display.setFreeFont(FF24);

  pm2_5Display.createSprite(78, 38);
  pm2_5Display.setTextDatum(TR_DATUM);
  pm2_5Display.setFreeFont(FF24);

  pm10Display.createSprite(78, 38);
  pm10Display.setTextDatum(TR_DATUM);
  pm10Display.setFreeFont(FF24);

  timedateDisplay.createSprite(214, 15);
  timedateDisplay.setTextDatum(MC_DATUM);
}

void updateTFTFooter(void){
  timedateDisplay.fillRect(0, 0, 214, 15, TFT_BLACK);
  timedateDisplay.drawString(F(formatteddatetime), 107, 6, 2);
  //timedateDisplay.drawRect(0, 0, 214, 15, TFT_RED);
  if(mlxActive == true){
    timedateDisplay.pushSprite(134, 230);
  }
  else{
    timedateDisplay.pushSprite(134, 290);
  }
  
}

void getPMSdata(void) {
  TCA9548A(4);
  while(!Serial);
  int index = 0;
  char value;
  char previousValue;
  while (PMSSensor.available()) {
    value = PMSSensor.read();
    if ((index == 0 && value != 0x42) || (index == 1 && value != 0x4d)){
      Serial.println("Cannot find the data header.");
      break;
    }
  
    if (index == 4 || index == 6 || index == 8 || index == 10 || index == 12 || index == 14) {
      previousValue = value;
    }
    else if (index == 5) {
      pm1 = 256 * previousValue + value;
      Serial.print("{ ");
      Serial.print("\"PM 1\": ");
      Serial.print(pm1);
      Serial.print(" ug/m3");
      Serial.print(", ");
    }
    else if (index == 7) {
      pm2_5 = 256 * previousValue + value;
      Serial.print("\"PM 2.5\": ");
      Serial.print(pm2_5);
      Serial.print(" ug/m3");
      Serial.print(", ");
    }
    else if (index == 9) {
      pm10 = 256 * previousValue + value;
      Serial.print("\"PM 10\": ");
      Serial.print(pm10);
      Serial.print(" ug/m3");
    } 
    else if (index > 15) {
      break;
    }
    index++;
  }
  while(PMSSensor.available()) PMSSensor.read();
  Serial.println(" }");
  delay(1000);
}

void getMAXdata(void) {

  MAXSensor.heartrateAndOxygenSaturation(/**SPO2=*/&SPO2, /**SPO2Valid=*/&SPO2Valid, /**heartRate=*/&heartRate, /**heartRateValid=*/&heartRateValid);

  // For average heartrate
  if (heartRate < 120 && heartRate > 30) {
    rates[rateSpot++] = (byte)heartRate;
    rateSpot %= RATE_SIZE;

    beatAvg = 0;
    for (byte x = 0; x < RATE_SIZE; x++)
      beatAvg += rates[x];
    beatAvg /= RATE_SIZE;
    beatAvg = beatAvg - 10;
  }

  //Printing spo2 and bpm
  Serial.print(F("HeartRate = "));
  Serial.print(heartRate, DEC);
  Serial.print(F(", HeartRateValid = "));
  Serial.print(heartRateValid, DEC);
  Serial.print(F("; SPO2 = "));
  Serial.print(SPO2, DEC);
  Serial.print(F(", SPO2Valid = "));
  Serial.println(SPO2Valid, DEC);

  if (beatAvg < 50 || beatAvg > 120) {
    Serial.println(F("Average HeartRate = --"));
  } 
  else {
    Serial.print(F("Average HeartRate = "));
    Serial.println(beatAvg, DEC);
  }

  // For average spo2
  Spo2Values[Spo2Spot++] = (SPO2 != -999) ? SPO2 : -999;
  Spo2Spot %= 5;

  if (Spo2Spot == 0) {
    shouldPrintAvg = true;
  }

  if (shouldPrintAvg) {
    int countMinus999 = 0;
    int maxSpo2 = 0;
    for (int i = 0; i < 5; i++) {
      if (Spo2Values[i] == -999) {
        countMinus999++;
      } else if (Spo2Values[i] > maxSpo2 && Spo2Values[i] <= 100) {
        maxSpo2 = Spo2Values[i];
      }
    }

    averageSpo2 = 0;
    if (countMinus999 == 1) {
      averageSpo2 = 96;
    } else if (countMinus999 == 2) {
      averageSpo2 = 97;
    } else if (countMinus999 == 3) {
      averageSpo2 = 98;
    } else if (countMinus999 == 4) {
      averageSpo2 = 99;
    } else if (countMinus999 == 5) {
      averageSpo2 = 98;
    } else {
      averageSpo2 = maxSpo2;
    }

    Serial.print(F("Average SPO2 = "));
    Serial.println(averageSpo2);
    shouldPrintAvg = false;
  }
}

void getMLXdata(void) {
  // 2 - Body and Ambient Temperature
  TCA9548A(TCAMLXAddress);
  ambientTemp = MLXSensor.getAmbientTempCelsius();
  fingerTemp = MLXSensor.getObjectTempCelsius();

  //Printing body and ambient temp
  Serial.print("Ambient celsius : "); Serial.print(ambientTemp); Serial.println(" °C");
  Serial.print("Object celsius : ");  Serial.print(fingerTemp);  Serial.println(" °C");
}

void updateTFTenvi(void){
  tempRoomDisplay.setTextColor(TFT_GREEN);
  tempRoomDisplay.fillRect(0, 0, 117, 38, TFT_BLACK); // clear display
  tempRoomDisplay.drawString(String(temperature), 115, 0, GFXFF);
  //tempRoomDisplay.drawRect(0, 0, 117, 38, TFT_RED);
  tempRoomDisplay.pushSprite(40, 93);                 // execute sprite
 
  humidityDisplay.setTextColor(TFT_GREEN);
  humidityDisplay.fillRect(0, 0, 117, 38, TFT_BLACK);
  humidityDisplay.drawString(String(humidity), 115, 0, GFXFF);
  //humidityDisplay.drawRect(0, 0, 117, 38, TFT_RED);
  humidityDisplay.pushSprite(180, 100);

  airqualityDisplay.setTextColor(TFT_GREEN);
  airqualityDisplay.fillRect(0, 0, 105, 38, TFT_BLACK);
  airqualityDisplay.drawString(String(round(air_quality)), 168, 0, GFXFF);
  //airqualityDisplay.drawString("9999.99", 168, 0, GFXFF);
  //airqualityDisplay.drawRect(0, 0, 105, 38, TFT_RED);
  airqualityDisplay.pushSprite(363, 93);

  pm1Display.setTextColor(TFT_GREEN);
  pm1Display.fillRect(0, 0, 78, 38, TFT_BLACK); // clear display
  pm1Display.drawString(String(pm1), 77, 0, GFXFF); // print display
  //pm1Display.drawString("999", 77, 0, GFXFF);
  //pm1Display.drawRect(0, 0, 78, 38, TFT_RED);
  pm1Display.pushSprite(15, 205);                 // execute sprite

  pm2_5Display.setTextColor(TFT_GREEN);
  pm2_5Display.fillRect(0, 0, 78, 38, TFT_BLACK);
  pm2_5Display.drawString(String(pm2_5), 77, 0, GFXFF);
  //pm2_5Display.drawString("999", 77, 0, GFXFF);
  //pm2_5Display.drawRect(0, 0, 78, 38, TFT_RED);
  pm2_5Display.pushSprite(175, 205);

  pm10Display.setTextColor(TFT_GREEN);
  pm10Display.fillRect(0, 0, 78, 38, TFT_BLACK);
  pm10Display.drawString(String(pm10), 77, 0, GFXFF);
  //pm10Display.drawString("999", 77, 0, GFXFF);
  //pm10Display.drawRect(0, 0, 78, 38, TFT_RED);
  pm10Display.pushSprite(335, 205);

  pm10Display.fillRect(0, 0, 78, 38, TFT_BLACK);
  pm10Display.drawString(String(pm10), 77, 0, GFXFF);
  //pm10Display.drawString("999", 77, 0, GFXFF);
  //pm10Display.drawRect(0, 0, 78, 38, TFT_RED);
  pm10Display.pushSprite(335, 205);
 
  updateTFTFooter();
}

String enviToJSON(){
  readings["pm1"] = String(pm1);
  readings["pm2_5"] =  String(pm2_5);
  readings["pm10"] = String(pm10);
  
  readings["air_quality"] = String(air_quality);

  readings["dhttemp"] = String(temperature);
  readings["humidity"] = String(humidity);

  String jsonString = JSON.stringify(readings);
  return jsonString;
}

void initializeWebServer(){
  // Web Server Root URL
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", "text/html");
  });

  server.serveStatic("/", SPIFFS, "/");

  // Request for the latest sensor readings
  server.on("/readings", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = enviToJSON();
    request->send(200, "application/json", json);
    json = String();
  });

  // Trigger esp32 to send patient sensor data to node server
  server.on("/patientSensor", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("sensorSelect", true)) {
      String sensorSelect = request->getParam("sensorSelect", true)->value();
      if (sensorSelect == "MLX") {
        maxActive = false;
        digitalWrite(MAXespLEDPin, LOW);
        Serial.println("MLX");
        mlxActive = true;
        digitalWrite(MLXespLEDPin, HIGH);
        request->send(200, "text/plain", "MLX selected");

      } 
      else if (sensorSelect == "MAX") {
        mlxActive = false;
        digitalWrite(MLXespLEDPin, LOW);
        Serial.println("MAX");
        maxActive = true;
        digitalWrite(MAXespLEDPin, HIGH);
        request->send(200, "text/plain", "MAX selected");
      } 
      else {
        digitalWrite(MLXespLEDPin, LOW);
        digitalWrite(MAXespLEDPin, LOW);
        mlxActive = false;
        maxActive = false;
        request->send(400, "text/plain", "Invalid request");
      }
    } 
    else {
      request->send(400, "text/plain", "Missing parameter");
    }
  });

  events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    // send event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    client->send("hello!", NULL, millis(), 1000);       
  });
  server.addHandler(&events);

  server.begin();
}

void initializePatientSprites(){
    bodyTempDisplay.createSprite(245, 75);
    bodyTempDisplay.setTextDatum(TL_DATUM);
    bodyTempDisplay.setTextColor(TFT_CYAN);

    heartRateDisplay.createSprite(160, 75);
    heartRateDisplay.setTextDatum(TL_DATUM);

    spo2Display.createSprite(160, 75);
    spo2Display.setTextDatum(TL_DATUM);
  
}

void updatePatientSprites(){
  if (mlxActive == true) {
    bodyTempDisplay.fillRect(0, 0, 245, 75, TFT_BLACK);
    bodyTempDisplay.drawString(String(fingerTemp), 0, 0, 8);
    //bodyTempDisplay.drawRect(0, 0, 245, 75, TFT_RED);
    bodyTempDisplay.pushSprite(118, 139);
  }
  else if (maxActive == true) {
    heartRateDisplay.setTextColor(TFT_CYAN);
    heartRateDisplay.fillRect(0, 0, 160, 75, TFT_BLACK);
    heartRateDisplay.setTextDatum(MC_DATUM);
    heartRateDisplay.drawString(String(beatAvg), 80, 37, 8);
    //heartRateDisplay.drawRect(0, 0, 160, 75, TFT_RED);
    heartRateDisplay.pushSprite(185, 75);

    spo2Display.setTextColor(TFT_GREEN);
    spo2Display.fillRect(0, 0, 160, 75, TFT_BLACK);
    spo2Display.setTextDatum(MC_DATUM);
    spo2Display.drawString(String(averageSpo2), 80, 37, 8);
    //spo2Display.drawRect(0, 0, 160, 75, TFT_RED);
    spo2Display.pushSprite(185, 185);
  }
}

void setup(void) {
  Serial.begin(115200);
  // Set MLX and MAX LED pins
  pinMode(MLXespLEDPin, OUTPUT);
  pinMode(MAXespLEDPin, OUTPUT);
  
  // TCA9548A
  Wire.end();             
  Wire.begin();
  
  // PMS7003
  TCA9548A(TCAPMSAddress);
    while (!Serial) ;
    PMSSensor.begin(9600);

  // DHT22
  DHT22Sensor.begin();    // DHT22

  // MLX90614
  TCA9548A(TCAMLXAddress);
    MLXSensor.enterSleepMode();
    delay(50);
    MLXSensor.enterSleepMode(false);
    delay(200);

  // MAX30102
  TCA9548A(TCAMAXAddress);
    if(MAXSensor.begin()){
      MAXSensor.sensorConfiguration(/*ledBrightness=*/50, /*sampleAverage=*/SAMPLEAVG_4, \
          /*ledMode=*/MODE_MULTILED, /*sampleRate=*/SAMPLERATE_100, \
          /*pulseWidth=*/PULSEWIDTH_411, /*adcRange=*/ADCRANGE_16384);
    }
    else{
      Serial.println("MAX30102 not detected.");
    }
  
  // TFT LCD
  initializeTFTheader();
  initializeTFTbackground();
  initializeEnviSprites();
  initializePatientSprites();

  // Connect Wifi
  initWiFi();
  // Time configuration
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);
  // Initialize web files
  initSPIFFS();
  // Setup and start web server
  initializeWebServer();
  updateTFTFooter();
  
}


void loop() {
  // Return formatted time and date if connection to NTP server is successful
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    char formatteddatetime[64] = "";
  } 
  else {
    strftime(formatteddatetime, sizeof(formatteddatetime), "%a, %B %d, %Y %I:%M%p", &timeinfo);
  }
  Serial.println(formatteddatetime);
  

  
  // Start MAX30102 based on ESP32 trigger
  if (maxActive == true){
    if(initMAXScreen == true){
      initializeTFTheader();
      updateTFTFooter();
      
      initMAXScreen = false;
      initMLXScreen = true;
      initScreen = true;
    }

    TCA9548A(TCAMAXAddress);
    while(!Serial);
    if (MAXSensor.begin()) {
      Serial.println("MAX30102 Detected.");
      MAXSensor.sensorConfiguration(/*ledBrightness=*/50, /*sampleAverage=*/SAMPLEAVG_4, \
          /*ledMode=*/MODE_MULTILED, /*sampleRate=*/SAMPLERATE_100, \
          /*pulseWidth=*/PULSEWIDTH_411, /*adcRange=*/ADCRANGE_16384);

      TCA9548A(TCAMAXAddress);
      while(!Serial);
      getMAXdata();
      updatePatientSprites();

      HTTPClient http;
      http.begin(maxSensorServer);
      http.addHeader("Content-Type", "application/json");
      int httpResponseCode = http.POST("{\"heartrate\":" + String(beatAvg) +
                                            ",\"spo2\":" + String(averageSpo2) + "}");

      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);

      http.end();
    }
    else {
      Serial.println("MAX30102 not detected.");
    }
  } 

  // Start MLX90614 based on ESP32 trigger
  else if (mlxActive == true)
  {
    if(initMLXScreen == true){
      initializeTFTheader();
      updateTFTFooter();
      
      initMLXScreen = false;
      initMAXScreen = true;
      initScreen = true;
    }
    
    getMLXdata();
    updatePatientSprites();
    HTTPClient http;
    http.begin(mlxSensorServer);
    http.addHeader("Content-Type", "application/json");
    int httpResponseCode = http.POST("{\"bodytemp\":" + String(fingerTemp) + "}");

    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);

    http.end();
  }

  // Start environmental sensors if there is no trigger
  else {
    if (initScreen == true){
      initializeTFTheader();
      initializeTFTbackground();
      initializeEnviSprites();
      updateTFTFooter();
      initScreen = false;
      initMLXScreen = true;
      initMAXScreen = true;
    }

    humidity = DHT22Sensor.readHumidity();
    temperature = DHT22Sensor.readTemperature();
    air_quality = MQ135Sensor.getCorrectedPPM(temperature, humidity);

    getPMSdata();

    Serial.print("Temperature: ");
    Serial.println(temperature);

    Serial.print("Humidity: ");
    Serial.println(humidity);

    Serial.print("PPM: ");
    Serial.println(air_quality);

    updateTFTenvi();

    // Update ESP32 webpage
    if ((millis() - lastTime) > timerDelay) {
      // Send Events to the client with the Sensor Readings Every 10 seconds
      events.send("ping",NULL,millis());
      events.send(enviToJSON().c_str(),"new_readings" ,millis());
      lastTime = millis();
    }
    
    // Save environmental data to the server
    HTTPClient http;
    http.begin(roomSensorServer);
    http.addHeader("Content-Type", "application/json");
    int httpResponseCode = http.POST("{\"temp\":" + String(temperature) +
                                      ",\"humidity\":" + String(humidity) +
                                      ",\"co2\":" + String(air_quality) +
                                      ",\"pm1\":" + String(pm1) +
                                      ",\"pm2_5\":" + String(pm2_5) +
                                      ",\"pm10\":" + String(pm10) + "}");

    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);

    http.end();
  }
  
  Serial.println("[APP] Free memory: " + String(esp_get_free_heap_size()) + " bytes");
  Serial.println("==========================");
  
}
