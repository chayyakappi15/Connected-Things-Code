#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>
#include <DFRobotDFPlayerMini.h>
#include <AdafruitIO_WiFi.h>

#define BRIGHTNESS 50

Servo servo;
#define DFPlayer_RX 25         // Pin connected to RX on DFPlayer
#define DFPlayer_TX 26         // Pin connected to TX on DFPlayer
#define FSR_Sensor_Pin 34      // Pin connected to FSR sensor
DFRobotDFPlayerMini dfplayer;  // Object for controlling MP3 files
// Store pin number and no. pixels as variables
int numberOfPixels = 18;
int dataPin = 12;

int servoPin = 32;

// Create Adafruit_NeoPixels object
Adafruit_NeoPixel pixels(numberOfPixels, dataPin, NEO_GRB + NEO_KHZ800);

// AdafruitIO_WiFi io("chayyakappi", "key", "Wifi", "password");
AdafruitIO_WiFi io("chayyakappi", "key", "WiFi", "password");
//=======================================================
// **REPLACE THESE VARIABLES WITH YOUR DETAILS**
const char* ssid = "Wifi";
const char* password = "password";
//api request
const char* requestURL1 = "https://api.open-meteo.com/v1/forecast?latitude=55.9521&longitude=-3.1965&daily=daylight_duration,sunrise,sunset&current=temperature_2m,rain,is_day,wind_speed_10m";
const char* requestURL2 = "https://api.open-meteo.com/v1/forecast?latitude=25.5&longitude=51.25&daily=daylight_duration,sunrise,sunset&current=temperature_2m,rain,is_day,wind_speed_10m";
//adafruit io feeds connection
AdafruitIO_Feed* led = io.feed("led");
AdafruitIO_Feed* temperatureFeed = io.feed("temperatureFeed");
AdafruitIO_Feed* daylight = io.feed("daylight");

unsigned long timestamp = 0;

String timenow = "";
String sunrise = "";
String sunset = "";
float temperature;
int isRaining;
int isDay;
float windspeed;

bool firsttime = true;
unsigned long apiTimestamp = 0;
unsigned long locationSwitchTimestamp = 0; 
const long displayDuration = 30000; //

// Function definitions
bool connectToWiFi(const char* _ssid, const char* _password);
bool useSecondLocation = false;
//SETUP
void setup() {
  // Start serial communication
  Serial.begin(115200);
  while (!Serial)
    ;

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
  servo.attach(servoPin);
  pixels.begin();
  // connect to io.adafruit.com
  Serial.print("Connecting to Adafruit IO");
  io.connect();

  // wait for a connection
  while (io.status() < AIO_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  // we are connected
  Serial.println();
  Serial.println(io.statusText());
}
bool wasRaining = false;
String globalPayload;
//=================================================================

void loop() {
  io.run();

int analogReading = analogRead(FSR_Sensor_Pin);
  Serial.print("The force sensor value = ");
  Serial.print(analogReading); // print the raw analog reading

static bool lastPressed = false;
bool pressed = analogReading > 1000; // adjust threshold

if (pressed && !lastPressed) {
  delay(50); // Simple debounce
  useSecondLocation = true; // Switch to location 2
  locationSwitchTimestamp = millis(); // Start the 5-minute clock
  timestamp = 0; // Only force refresh ONCE at the moment of the press
  Serial.println("Switching to Location 2 for 30 seconds");
}

// Check if the 30 seconds are up to switch back to Location 1
if (useSecondLocation && (millis() - locationSwitchTimestamp > displayDuration)) {
  useSecondLocation = false;
  timestamp = 0; // Force refresh once to get Location 1 data
  Serial.println("Back to Location 1");
}

lastPressed = pressed;


 
  // 1. NON-BLOCKING API TIMER (Check every 60 seconds)
  if (millis() - timestamp > 60000 || timestamp == 0 || firsttime == true) {
  
  if (useSecondLocation) {
  makeAPIRequestTwo(globalPayload);
} else {
  makeAPIRequest(globalPayload);
}
  
    deserialise();

    moveMotor();

    if (isRaining == 1 && !wasRaining) {
      fadeToTrack(2, 5);  // Track 2, Volume 15
      wasRaining = true;
    } else if (isRaining == 0 && wasRaining) {
      fadeToTrack(3, 5);  // Track 7, Volume 15
      wasRaining = false;
    }

    if (firsttime == true && timestamp > 0) {
      delay(1000);
      firsttime = false;
    }

    timestamp = millis();
  }
  

  int pulse = (sin(millis() / 1000.0) * 127.5) + 127.5;

  if (temperature > 20) {
    // FADE RED TO YELLOW
    // Red stays at 255, Blue stays at 0, Green fades 0 -> 255
    for (int i = 0; i < numberOfPixels; i++) {
      pixels.setPixelColor(i, pixels.Color(255, pulse, 0));
    }
  }
  if (temperature < 20) {
    // FADE BLUE TO WHITE
    // Blue stays at 255, Red and Green fade 0 -> 255
    for (int i = 0; i < numberOfPixels; i++) {
      pixels.setPixelColor(i, pixels.Color(pulse, pulse, 255));
    }
  }
  pixels.setBrightness(BRIGHTNESS);
  pixels.show();
  Serial.println(pulse);
  delay(15);
}

void moveMotor() {
  // //=======================================================
  // Calculate current time in mins
  String timeHrsStr = timenow.substring(0, 2);
  String timeMinsStr = timenow.substring(3, 5);
  int timeHrs = timeHrsStr.toInt();
  int timeMins = timeMinsStr.toInt();
  int timeTotal = (timeHrs * 60) + timeMins;
  // Calculate sunrise timenow in mins
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


  if ((percentageThroughDay > 10) && (percentageThroughDay <= 20)) {
    servo.write(18);
  }
  if ((percentageThroughDay > 20) && (percentageThroughDay <= 30)) {
    servo.write(36);
  }
  if ((percentageThroughDay > 30) && (percentageThroughDay <= 40)) {
    servo.write(54);
  }
  if ((percentageThroughDay > 40) && (percentageThroughDay <= 50)) {
    servo.write(72);
  }
  if ((percentageThroughDay > 50) && (percentageThroughDay <= 60)) {
    servo.write(90);
  }
  if ((percentageThroughDay > 60) && (percentageThroughDay <= 70)) {
    servo.write(108);
  }
  if ((percentageThroughDay > 70) && (percentageThroughDay <= 80)) {
    servo.write(136);
  }
  if ((percentageThroughDay > 80) && (percentageThroughDay <= 90)) {
    servo.write(154);
  }
  if ((percentageThroughDay > 90) && (percentageThroughDay <= 100)) {
    servo.write(172);
  }

  else {
    servo.write(0);
  }
  // save fahrenheit (or celsius) to Adafruit IO
  temperatureFeed->save(temperature);
  //save daylight (percentage) to adafruit io
  daylight->save(percentageThroughDay);
  // wait 5 seconds (5000 milliseconds == 5 seconds)
;
}

void deserialise() {

  // 2. DESERIALIZE DATA
  // We do this every loop to keep variables updated, but only if payload isn't empty
  StaticJsonDocument<2048> doc;
  DeserializationError error = deserializeJson(doc, globalPayload);
  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }

  JsonObject current = doc["current"];
  timenow = String(current["time"]);
  temperature = current["temperature_2m"];
  isRaining = current["rain"];
  isDay = current["is_day"];
  windspeed = current["wind_speed_10m"];

  JsonObject daily = doc["daily"];
  JsonArray daily_sunrise = daily["sunrise"];
  sunrise = String(daily_sunrise[0]);
  JsonArray daily_sunset = daily["sunset"];
  sunset = String(daily_sunset[0]);
  timenow = timenow.substring(11, 16);
  sunrise = sunrise.substring(11, 16);
  sunset = sunset.substring(11, 16);

  // //=======================================================
  // // 4. USE/PRINT DATA
  // Print data to Serial Monitor
  Serial.println("Time: " + String(timenow));
  Serial.println("Temp.: " + String(temperature));
  Serial.println("Is rain? " + String(isRaining));
  Serial.println("Is day? " + String(isDay));
  Serial.println("Windspeed: " + String(windspeed));
  Serial.println("Sunrise: " + String(sunrise));
  Serial.println("Sunset: " + String(sunset));
}

//=================================================================

void fadeToTrack(int newTrack, int targetVolume) {
  Serial.print("Fading to track: ");
  Serial.println(newTrack);

  // Fade Out
  for (int v = targetVolume; v >= 0; v--) {
    dfplayer.volume(v);
    delay(30);  // Adjust delay for speed of fade (total ~0.5 seconds)
  }

  dfplayer.play(newTrack);
  delay(100);  // Short pause for the hardware to start the file

  // Fade In
  for (int v = 0; v <= targetVolume; v++) {
    dfplayer.volume(v);
    delay(30);
  }
}
//============================================================
// Make API Request
void makeAPIRequest(String& _payload) {
  // Start HTTP client
  HTTPClient client;
  client.begin(requestURL1);
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
void makeAPIRequestTwo(String& _payload) {
  // Start HTTP client
  HTTPClient client;
  client.begin(requestURL2);
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


