#include <Arduino.h>
static const byte DHT22espPin = 32;
static const byte MQ135espPin = 35;
static const byte TCAPMSAddress = 4;
static const byte TCAMLXAddress = 3;
static const byte TCAMAXAddress = 2;
static const byte MLXespLED = 99;
static const byte MAXespLEDPin = 99;

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
float objectTemp, ambientTemp;

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


void initializeTFTbackground(void){
  // Draw header
  tft.setTextDatum(MC_DATUM);                   // Set text reference coordinates to middle center
  tft.setFreeFont(FSS9);                        // Fonts for header
  tft.setTextColor(TFT_MAGENTA);
  tft.drawString("VITAL SIGNS AND AIR QUALITY MONITORING", 240, 20, GFXFF);
  tft.drawString("PALTOC HEALTH CENTER - SAMPALOC, MANILA", 240, 40, 2);

  tft.setTextColor(0x077F);

  // Draw sensors borders
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

void initializeSprites(void){
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

  timedateDisplay.createSprite(240, 25);
  timedateDisplay.setTextDatum(MC_DATUM);
}

void getPMSdata(void) {
  TCA9548A(TCAPMSAddress);
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
  TCA9548A(TCAMAXAddress);

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
} else {
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
  objectTemp = MLXSensor.getObjectTempCelsius();

  //Printing body and ambient temp
  Serial.print("Ambient celsius : "); Serial.print(ambientTemp); Serial.println(" °C");
  Serial.print("Object celsius : ");  Serial.print(objectTemp);  Serial.println(" °C");
}

void updateTFTdata(void){
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
 
//   timedateDisplay.fillRect(0, 0, 240, 25, TFT_BLACK);
//   timedateDisplay.drawString(F("January 29, 2024"), 120, 12, 2);
//   timedateDisplay.drawRect(0, 0, 240, 25, TFT_RED);
//   timedateDisplay.pushSprite(118, 285);
}

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

// ----------------------------------------------------------------- //
// Replace with your network credentials
const char* ssid = "Oreo Dog";
const char* password = "Oreo20201029?";
// ----------------------------------------------------------------- //

// Initialize WiFi
#include <WiFi.h>
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

String getSensorReadings(){
  readings["pm1"] = String(pm1);
  readings["pm2_5"] =  String(pm2_5);
  readings["pm10"] = String(pm10);
  
  readings["air_quality"] = String(air_quality);

  readings["dhttemp"] = String(temperature);
  readings["humidity"] = String(humidity);

  String jsonString = JSON.stringify(readings);
  return jsonString;
}

void setup(void) {
  Serial.begin(115200);
  Wire.end();             // TCA9548A
  Wire.begin();           // TCA9548A
  
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

  //  TCA9548A(TCAMAXAddress);
  //    while (!MAXSensor.begin()) {
  //      Serial.println("MAX30102 not detected.");
  //      delay(1000);
  //    }
  
  //    MAXSensor.sensorConfiguration(/*ledBrightness=*/50, /*sampleAverage=*/SAMPLEAVG_4, \
  //        /*ledMode=*/MODE_MULTILED, /*sampleRate=*/SAMPLERATE_100, \
  //        /*pulseWidth=*/PULSEWIDTH_411, /*adcRange=*/ADCRANGE_16384);

  // TFT LCD
  tft.init();
  tft.setRotation(1);                           // Rotates screen by 1 counter clockwise
  tft.fillScreen(TFT_BLACK);                    // Screen background
  initializeTFTbackground();
  initializeSprites();


  //------------------------------------------------------------------------------------------------//
  // Initialize Wifi + Web server
  initWiFi();
  initSPIFFS();

  // Web Server Root URL
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", "text/html");
  });

  server.serveStatic("/", SPIFFS, "/");

  // Request for the latest sensor readings
  server.on("/readings", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = getSensorReadings();
    request->send(200, "application/json", json);
    json = String();
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

  // Start server
  server.begin();
}

void loop() {
  humidity = DHT22Sensor.readHumidity();
  temperature = DHT22Sensor.readTemperature();
  air_quality = MQ135Sensor.getCorrectedPPM(temperature, humidity);

  TCA9548A(TCAMAXAddress);
  if (MAXSensor.begin()) {
    Serial.println("MAX30102 Detected.");
    MAXSensor.sensorConfiguration(/*ledBrightness=*/50, /*sampleAverage=*/SAMPLEAVG_4, \
        /*ledMode=*/MODE_MULTILED, /*sampleRate=*/SAMPLERATE_100, \
        /*pulseWidth=*/PULSEWIDTH_411, /*adcRange=*/ADCRANGE_16384);
  getMAXdata();
  }
  else {
    Serial.println("MAX30102 not detected.");
  }

  getMLXdata();
  getPMSdata();

  Serial.print("Temperature: ");
  Serial.println(temperature);

  Serial.print("Humidity: ");
  Serial.println(humidity);

  Serial.print("PPM: ");
  Serial.println(air_quality);

  updateTFTdata();

  if ((millis() - lastTime) > timerDelay) {
    // Send Events to the client with the Sensor Readings Every 10 seconds
    events.send("ping",NULL,millis());
    events.send(getSensorReadings().c_str(),"new_readings" ,millis());
    lastTime = millis();
  }

  Serial.println("[APP] Free memory: " + String(esp_get_free_heap_size()) + " bytes");
  Serial.println("==========================");
}
