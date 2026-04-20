#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>
#include <DFRobotDFPlayerMini.h>
 
Servo servo;
 
// Store pin number and no. pixels as variables
int numberOfPixels = 32;
int dataPin = 32;
 
// Create Adafruit_NeoPixels object
Adafruit_NeoPixel pixels(numberOfPixels, dataPin, NEO_GRB + NEO_KHZ800);
uint32_t red = pixels.Color(255, 0, 0);
uint32_t blue = pixels.Color(0, 0, 255);
uint32_t purple = pixels.Color(255, 0, 255);
uint32_t bluepurple = pixels.Color(50, 0, 255);
uint32_t redpurple = pixels.Color(255, 0, 50);
uint32_t moreredpurple = pixels.Color(255, 0, 20);
 
#define DFPlayer_RX 18  // Pin connected to RX on DFPlayer
#define DFPlayer_TX 5  // Pin connected to TX on DFPlayer
 
DFRobotDFPlayerMini dfplayer;  // Object for controlling MP3 files
 
//=======================================================
// **REPLACE THESE VARIABLES WITH YOUR DETAILS**
const char* ssid = "Pixel_9781";
const char* password = "KarmenKass";
//=======================================================
 
// API Request URL
const char* requestURL = "https://api.open-meteo.com/v1/forecast?latitude=55.9521&longitude=-3.1965&daily=daylight_duration,sunrise,sunset&current=temperature_2m,rain,is_day,wind_speed_10m";
 
// Function definitions
bool connectToWiFi(const char* _ssid, const char* _password);
 
//=======================================================
// SETUP
 
void setup() {
  Serial.begin(115200);
 
  // Connect to WiFi
  // Do not continue if not connected
  while (!connectToWiFi(ssid, password)) {}
 
  // Start software serial communication - for DFPlayer
  Serial1.begin(9600, SERIAL_8N1, DFPlayer_RX, DFPlayer_TX);
 
  // Initialise DFPlayer
  if (!dfplayer.begin(Serial1, true, true)) {
    Serial.println("Error: Failed to initialise DFPlayer.");
    while (true) {}  // Do not continue if init fails
  }
  // Set volume value (0-30).
  dfplayer.volume(15);
  dfplayer.play(1);  //Play the first mp3
 
  // Attach pin 12 to servo object
  servo.attach(12);
  pixels.begin();
}



void loop() {
 
  // // Get JSON response
  String payload;  // this will store the entire response
  makeAPIRequest(payload);
 
  // //=======================================================
  // // 2. DESERIALIZE RESPONSE
 
  StaticJsonDocument<2048> doc;
 
  DeserializationError error = deserializeJson(doc, payload);
 
  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
 
    static unsigned long timestamp = millis();
 
    if (millis() - timestamp > 3000) {
      timestamp = millis();
      dfplayer.next();  // Play next file every 3 second
    }
  }
 
  // //=======================================================
  // // 3. EXTRACT DATA
 
  JsonObject current = doc["current"];
  String time = current["time"];
  float temperature = current["temperature_2m"];
  int isRaining = current["rain"];
  int isDay = current["is_day"];
  float windspeed = current["wind_speed_10m"];
 
  JsonObject daily = doc["daily"];
 
  JsonArray daily_sunrise = daily["sunrise"];
  String sunrise = daily_sunrise[0];
 
  JsonArray daily_sunset = daily["sunset"];
  String sunset = daily_sunset[0];
 
  time = time.substring(11, 16);
  sunrise = sunrise.substring(11, 16);
  sunset = sunset.substring(11, 16);
 
  // //=======================================================
  // // 4. USE/PRINT DATA
 
  // Print data to Serial Monitor
  Serial.println("Time: " + String(time));
  Serial.println("Temp.: " + String(temperature));
  Serial.println("Is rain? " + String(isRaining));
  Serial.println("Is day? " + String(isDay));
  Serial.println("Windspeed: " + String(windspeed));
 
  Serial.println("Sunrise: " + String(sunrise));
  Serial.println("Sunset: " + String(sunset));
 
  if (isRaining == true) {
    dfplayer.play(2);
  }
 
  if (temperature < 10) {
    // do something if hotter than 10 degrees
    pixels.fill(pixels.Color(blue, 0, 0), 0, 32);
    pixels.setBrightness(252);
    pixels.show();
  }
 
  if (temperature > 10) {
    // do something if hotter than 10 degrees
    pixels.fill(pixels.Color(red, 0, 0), 0, 32);
    pixels.setBrightness(252);
    pixels.show();
  }
 
  if (temperature < 20) {
    // do something if hotter than 10 degrees
    pixels.fill(pixels.Color(blue, 0, 0), 0, 32);
    pixels.setBrightness(252);
    pixels.show();
  }
 
  // //=======================================================
 
  // Calculate current time in mins
  String timeHrsStr = time.substring(0, 2);
  String timeMinsStr = time.substring(3, 5);
 
  int timeHrs = timeHrsStr.toInt();
  int timeMins = timeMinsStr.toInt();
 
  int timeTotal = (timeHrs * 60) + timeMins;
 
  // Calculate sunrise time in mins
  String sunriseHrsStr = sunrise.substring(0, 2);
  String sunriseMinsStr = sunrise.substring(3, 5);
 
  int sunriseHrs = sunriseHrsStr.toInt();
  int sunriseMins = sunriseMinsStr.toInt();
 
  int sunriseTotal = (sunriseHrs * 60) + sunriseMins;
 
  // Calculate sunset time in mins
  String sunsetHrsStr = sunset.substring(0, 2);
  String sunsetMinsStr = sunset.substring(3, 5);
 
  int sunsetHrs = sunsetHrsStr.toInt();
  int sunsetMins = sunsetMinsStr.toInt();
 
  int sunsetTotal = (sunsetHrs * 60) + sunsetMins;
 
  Serial.println("Time in mins: " + String(timeTotal));
  Serial.println("Sunrise in mins: " + String(sunriseTotal));
  Serial.println("Sunset in mins: " + String(sunsetTotal));
 
  int middayTotal = (sunriseTotal + sunsetTotal) * 0.5;
  // //=======================================================
 
  int percentageThroughDay = map(timeTotal, sunriseTotal, sunsetTotal, 0, 100);
  int angleOfMotor = map(timeTotal, sunriseTotal, sunsetTotal, 0, 180);
 
  Serial.println("% through daylight: " + String(percentageThroughDay));
  Serial.println("Motor Angle: " + String(angleOfMotor));
 
  if ((percentageThroughDay > 10) && (percentageThroughDay < 20)) {
    servo.write(18);
  }
  if ((percentageThroughDay > 20) && (percentageThroughDay < 30)) {
    servo.write(36);
  }
 
  if ((percentageThroughDay > 30) && (percentageThroughDay < 40)) {
    servo.write(54);
  }
 
  if ((percentageThroughDay > 40) && (percentageThroughDay < 50)) {
    servo.write(72);
  }
 
  if ((percentageThroughDay > 50) && (percentageThroughDay < 60)) {
    servo.write(90);
  }
 
  if ((percentageThroughDay > 60) && (percentageThroughDay < 70)) {
    servo.write(108);
  }
 
  if ((percentageThroughDay > 70) && (percentageThroughDay < 80)) {
    servo.write(136);
  }
 
  if ((percentageThroughDay > 80) && (percentageThroughDay < 90)) {
    servo.write(154);
  }
 
  if ((percentageThroughDay > 90) && (percentageThroughDay < 100)) {
    servo.write(172);
  }
  //=======================================================
 
  // Wait 1 min before making next request
  delay(60000);
}
 
//============================================================
// Make API Request
 
void makeAPIRequest(String& _payload) {
  // Start HTTP client
  HTTPClient client;
  client.begin(requestURL);
 
  // Get status
  int status = client.GET();
 
  if (status == 0) {
    Serial.println("Connection failed");
    while (true) {}
  }
 
  // Retrieve JSON response
  _payload = client.getString();
  client.end();  // End HTTP connection
}
 
//============================================================
// Connect to WiFi
 
bool connectToWiFi(const char* _ssid, const char* _password) {
  Serial.println("Connecting to:");
  Serial.println(_ssid);
 
  // Start connection
  WiFi.mode(WIFI_STA);
  WiFi.begin(_ssid, _password);
  unsigned long startAttemptTime = millis();
 
  // Wait until connected, or timed out
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 30000) {
    Serial.print(".");
    delay(400);
  }
 
  Serial.println();
 
  // Print connection status
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Err: Failed to connect");
    delay(2000);
    return false;
  } else {
    Serial.println("Connected");
    delay(2000);
    return true;
  }
}
