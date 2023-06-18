#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebSrv.h>
#include "SPIFFS.h"
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Arduino_JSON.h>
#include <DHT.h>
#include <PubSubClient.h>
#include <Ultrasonic.h>
#include <HTTPClient.h>

#define GYRO_ADDRESS 0x68
// Network credentials
const char* ssid = "bintangrb";
const char* password = "marvel1234";

//MQTT Configuration
const char* mqttServer = "test.mosquitto.org";
const int mqttPort = 1883;
const char* mqttTopicReadings = "sensor/readings";
const char* mqttTopicRecording = "sensor/recording";
const char* mqttTopicQuery = "sensor/all_data";
const char* mqttTopicData = "sensor/table";

// API URL
const char* serverName = "https://192.168.28.73:5000/runs";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Create an Event Source on /events
AsyncEventSource events("/events");

// Json Variable to Hold Sensor Readings
JSONVar readings;

// Timer variables
unsigned long lastTimeDHT = 0;
unsigned long lastTimeGyro = 0;
unsigned long lastTimeTemperature = 0;
unsigned long lastTimeAcc = 0;
unsigned long lastTimeDistance = 0;
unsigned long lastTimePost = 0;
unsigned long lastTimeGyroCal = 0;
unsigned long lastTimeAccCal = 0;

// Delay variables
unsigned long dhtDelay = 500;
unsigned long gyroDelay = 10;
unsigned long temperatureDelay = 1000;
unsigned long accelerometerDelay = 200;
unsigned long distanceDelay = 1000;
unsigned long postDelay = 1000;
unsigned long gyroCalDelay = 10;
unsigned long accCalDelay = 10;

// DHT Sensor Configuration
#define DHT_PIN 4
#define DHT_TYPE DHT22

// Create a DHT sensor object
DHT dht(DHT_PIN, DHT_TYPE);
float temperature, humidity;

// Sensor Pin Configuration
const int trigPin = 27;
const int echoPin = 33;

// define sound speed in cm/uS
#define SOUND_SPEED 0.034

long duration;
float distanceCm;

// Create a MPU sensor object
Adafruit_MPU6050 mpu;

// Create gyro and accel variables
sensors_event_t a, g, temp;
float gyroX, gyroY, gyroZ;
float accX, accY, accZ;
float temperatureG;

// Gyroscope sensor deviation
float gyroXerror = 0.07;
float gyroYerror = 0.03;
float gyroZerror = 0.01;

// Global variables for gyro calibration
float gyroX_offset = 0.0;
float gyroY_offset = 0.0;
float gyroZ_offset = 0.0;
bool isGyroCalibrated = false;

// Global variables for accelerometer calibration
float accX_offset = 0.0;
float accY_offset = 0.0;
float accZ_offset = 0.0;
bool isAccCalibrated = false;

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

String sensorReadings;
bool recording = false;
bool recordingPrev = false;
int currentRun = 0;

// Init DHT Sensor
void initDHT() {
  dht.begin();
}

// Init MPU6050
void initMPU(){
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    delay(1000);
  }
  Serial.println("MPU6050 Found!");
}

// Initialize SPIFFS
void initSPIFFS() {
  if (!SPIFFS.begin()) {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  Serial.println("SPIFFS mounted successfully");
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

// Connect to MQTT broker
void connectToMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Connecting to MQTT broker...");
    if (mqttClient.connect("ESP8266Client")) {
      Serial.println("Connected to MQTT broker");
    } else {
      Serial.print("Failed to connect to MQTT broker. Retrying in 5 seconds...");
      delay(5000);
    }
  }
}

// Callback function for MQT subscription
void callback(char* topic, byte* payload, unsigned int length) {
  // Convert the received payload to a string
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  // Parse the received JSON data
  JSONVar stateReq = JSON.parse(message);
  // Check the topic of the payload
  if (strcmp(topic, "sensor/recording") == 0) {
    Serial.println(stateReq);
    int request = stateReq["state"];
    if (request == 1) {
      recording = true;
    } else if (request == 0) {
      recording = false;      
    }
  } else if (strcmp(topic, "sensor/all_data") == 0) {
    if(WiFi.status()== WL_CONNECTED){
      String table = httpGETRequest(serverName);
      Serial.println(message);
      Serial.println(table);
      if (mqttClient.publish(mqttTopicData, table.c_str())){
        Serial.println("published");
      }
    }
    else {
      Serial.println("WiFi Disconnected");
    }
  }
}

// Publish sensor readings to MQTT topic
void publishSensorReadings() {
  String json = getSensorReadings();
  mqttClient.publish(mqttTopicReadings, json.c_str());
  mqttClient.subscribe(mqttTopicRecording);
  mqttClient.subscribe(mqttTopicQuery);
}

// Get Sensor Readings and return JSON object
String getSensorReadings(){
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from DHT sensor!");
    return "{}";
  }

  readings["temperature"] = String(temperature);
  readings["humidity"] =  String(humidity);

  String jsonString = JSON.stringify(readings);
  return jsonString;
}

String getGyroReadings(){
  mpu.getEvent(&a, &g, &temp);

  // Calibrate gyro z-axis if not already done
  if (!isGyroCalibrated) {
    calibrateGyro();
  }
  
  float gyroX_temp = g.gyro.x - gyroX_offset;
  if(abs(gyroX_temp) > gyroXerror) {
    gyroX += gyroX_temp/50.00;
  }

  float gyroY_temp = g.gyro.y - gyroY_offset;
  if(abs(gyroY_temp) > gyroYerror) {
    gyroY += gyroY_temp/70.00;
  }

  float gyroZ_temp = g.gyro.z - gyroZ_offset;
  if(abs(gyroZ_temp) > gyroZerror) {
    gyroZ += gyroZ_temp/90.00;
  }

  readings["gyroX"] = String(gyroX);
  readings["gyroY"] = String(gyroY);
  readings["gyroZ"] = String(gyroZ);

  String jsonString = JSON.stringify(readings);
  return jsonString;
}

void calibrateGyro() {
  const int numSamples = 100;
  float sumX = 0.0;
  float sumY = 0.0;
  float sumZ = 0.0;

  // Collect samples for z-axis calibration
  for (int i = 0; i < numSamples; ) {
    if ((millis() - lastTimeGyroCal) > gyroCalDelay) {
    mpu.getEvent(&a, &g, &temp);
    sumX += g.gyro.x;
    sumY += g.gyro.y;
    sumZ += g.gyro.z;
    i++;
    lastTimeGyroCal = millis();
    }
  }

  // Calculate the average offset for z-axis
  gyroX_offset = sumX / numSamples;
  gyroY_offset = sumY / numSamples;
  gyroZ_offset = sumZ / numSamples;

  isGyroCalibrated = true;
}

String getAccReadings(){
  mpu.getEvent(&a, &g, &temp);

  // Calibrate accelerometer if not already done
  if (!isAccCalibrated) {
    calibrateAcc();
  }

  // Get current acceleration values
  accX = a.acceleration.x - accX_offset;
  accY = a.acceleration.y - accY_offset;
  accZ = a.acceleration.z - accZ_offset;

  readings["accX"] = String(accX);
  readings["accY"] = String(accY);
  readings["accZ"] = String(accZ);
  
  String accString = JSON.stringify (readings);
  return accString;
}

void calibrateAcc() {
  const int numSamples = 100;
  float sumX = 0.0;
  float sumY = 0.0;
  float sumZ = 0.0;

  // Collect samples for calibration
  for (int i = 0; i < numSamples; ) {
    if ((millis() - lastTimeAccCal) > accCalDelay) {
      mpu.getEvent(&a, &g, &temp);
      sumX += a.acceleration.x;
      sumY += a.acceleration.y;
      sumZ += a.acceleration.z;
      i++;
      lastTimeAccCal = millis();
    }
  }

  // Calculate the average offset for each axis
  accX_offset = sumX / numSamples;
  accY_offset = sumY / numSamples;
  accZ_offset = sumZ / numSamples;

  isAccCalibrated = true;
}

String getTemperature(){
  mpu.getEvent(&a, &g, &temp);
  temperatureG = temp.temperature;
  return String(temperatureG);
}

String getDistance(){
  // Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);
  
  // Calculate the distance
  distanceCm = duration * SOUND_SPEED/2;
  
  readings["distance"] = String(distanceCm);

  return String(distanceCm);
}

String httpGETRequest(const char* serverName) {
  WiFiClient client;
  HTTPClient http;
  
  http.begin(client, serverName);
  
  // Send HTTP POST request
  int httpResponseCode = http.GET();
  
  String payload = "{}"; 
  
  if (httpResponseCode>0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();

  return payload;
}

void getLatestRun(){
  if(WiFi.status()== WL_CONNECTED){
              
    sensorReadings = httpGETRequest(serverName);
    Serial.println(sensorReadings);
    JSONVar myObject = JSON.parse(sensorReadings);

    if (JSON.typeof(myObject) == "undefined") {
      Serial.println("Parsing input failed!");
      return;
    }
  
    Serial.print("JSON object = ");
    Serial.println(myObject);
  
    for (int i = 0; i < myObject.length(); i++) {
      JSONVar value = myObject[i];
      if (int(value["run"]) > currentRun) {
        currentRun = int(value["run"]);        
      }
    }
  }
  else {
    Serial.println("WiFi Disconnected");
  }
}

void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);
  initDHT();
  initWiFi();
  initSPIFFS();
  initMPU();
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Set MQTT server and port
  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(callback);

  // Connect to MQTT broker
  connectToMQTT();

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

  // ReEquest for reset gyro
  server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request){
    gyroX=0;
    gyroY=0;
    gyroZ=0;
    request->send(200, "text/plain", "OK");
  });

  server.on("/resetX", HTTP_GET, [](AsyncWebServerRequest *request){
    gyroX=0;
    request->send(200, "text/plain", "OK");
  });

  server.on("/resetY", HTTP_GET, [](AsyncWebServerRequest *request){
    gyroY=0;
    request->send(200, "text/plain", "OK");
  });

  server.on("/resetZ", HTTP_GET, [](AsyncWebServerRequest *request){
    gyroZ=0;
    request->send(200, "text/plain", "OK");
  });

  //Request for start recording
  server.on("/start", HTTP_GET, [](AsyncWebServerRequest *request){
    recording = true;
    Serial.println("start");
    request->send(200, "text/plain", "OK");
  });

  //Request for stop recording
  server.on("/stop", HTTP_GET, [](AsyncWebServerRequest *request){
    recording = false;
    Serial.println("stop");
    request->send(200, "text/plain", "OK");
  });

  events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    // send event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    client->send("hello!", NULL, millis(), 500);
  });
  server.addHandler(&events);

  // Start server
  server.begin();
}

void loop() {
  // Reconnect to MQTT if connection is lost
  if (!mqttClient.connected()) {
    connectToMQTT();
  }
  mqttClient.loop();

  if (recordingPrev == false && recording == true) {
    getLatestRun();
    recordingPrev = true;
  }

  //Send an HTTP POST request every 10 minutes
  if ((millis() - lastTimePost) > postDelay && recording == true) {
    Serial.println("Data sent");
    //Check WiFi connection status
    if(WiFi.status()== WL_CONNECTED){
      WiFiClient client;
      HTTPClient http;
  
      http.begin(client, serverName);
      
      // Specify content-type header
      http.addHeader("Content-Type", "application/json");

      // Construct the JSON body
      String jsonBody = "{\"temperature\": "+String(temperature)+", \"humidity\": "+String(humidity)+", \"distance\": "+String(distanceCm)+", \"gyro\": "+String(gyroZ)+", \"run\": "+String(currentRun+1)+"}";
      Serial.println(jsonBody);
      // Send HTTP POST request
      int httpResponseCode = http.POST(jsonBody);
     
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
        
      http.end();
    }
    else {
      Serial.println("WiFi Disconnected");
    }
    lastTimePost = millis();
  }

  // Publish sensor readings to MQTT topic
  if ((millis() - lastTimeDHT) > dhtDelay) {
    events.send(getSensorReadings().c_str(),"new_readings",millis());
    publishSensorReadings();
    lastTimeDHT = millis();
  }

  if ((millis() - lastTimeGyro) > gyroDelay) {
    events.send(getGyroReadings().c_str(),"gyro_readings",millis());
    lastTimeGyro = millis();
  }

  if ((millis() - lastTimeAcc) > accelerometerDelay) {
    events.send(getAccReadings().c_str(),"accelerometer_readings",millis());
    lastTimeAcc = millis();
  }

  if ((millis() - lastTimeDistance) > distanceDelay) {
    events.send(getDistance().c_str(),"distance_reading",millis());
    lastTimeDistance = millis();
  }
}