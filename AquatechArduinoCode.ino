#include "DHT.h"
#include <MHZ19.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Servo.h>

#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

#define DHTPIN D1  //Connect to DHT11 or humidity sensor signal pin
#define DHTTYPE DHT11
#define Pump D6  //Connect to relay module signal pin for pump
#define ONE_WIRE_BUS D2 //Connect to outpin or signal pin of DS18B20 sensor
Servo s1;  

#define WIFI_SSID "wifi"
#define WIFI_PASSWORD "password"
#define API_KEY "api key"
#define DATABASE_URL "database url" 


FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
 
int pH_Value; 
float Voltage;
DHT dht(DHTPIN, DHTTYPE);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

unsigned long sendDataPrevMillis = 0;
bool signupOK = false;
bool pumpStatus = false;
bool feederStatus = false;



void setup(void) {
  pinMode(Pump, OUTPUT);
  pinMode(pH_Value, INPUT); 
  Serial.begin(9600);
  s1.attach(D3); //Connect to servo signal pin for feeder
  dht.begin();
  sensors.begin();
  digitalWrite(Pump, LOW);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
   while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(300);
  }
   Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  config.token_status_callback = tokenStatusCallback;
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

}


void loop(void) {
  
  if (Firebase.ready() && signupOK && (millis() -  sendDataPrevMillis > 1000 || sendDataPrevMillis == 0 )) {
    sendDataPrevMillis = millis();
    sensors.requestTemperatures();

    float h = dht.readHumidity();
    float t = dht.readTemperature();
    pH_Value = analogRead(A0); // Connect to PH LEVEL SENSOR signal pin
    Voltage = pH_Value * (5.0 / 1023.0); 
    
    if (Firebase.RTDB.setFloat(&fbdo, "SENSORS/1/waterTemp", sensors.getTempCByIndex(0))){
       Serial.print("Water Temperature: ");
       Serial.println(sensors.getTempCByIndex(0));
      
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }

    if (Firebase.RTDB.setFloat(&fbdo, "SENSORS/1/humidity", h)){
       Serial.print("Humidity: ");
       Serial.println(h);
      
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }

    if (Firebase.RTDB.setFloat(&fbdo, "SENSORS/1/temperature", t)){
       Serial.print("temperature: ");
       Serial.println(t);
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }

    if (Firebase.RTDB.setFloat(&fbdo, "SENSORS/1/phLevel", Voltage)){
       Serial.print("ph: ");
       Serial.println(Voltage);
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }

    Serial.print("");
    Serial.println("_______________________________________________");
    if (Firebase.RTDB.getBool(&fbdo, "Controls/1/pump")){
      if (fbdo.dataType() == "boolean"){
      pumpStatus = fbdo.boolData();
      Serial.println("Seccess: " + fbdo.dataPath() + ": " + pumpStatus + "(" + fbdo.dataType() + ")");
      digitalWrite(Pump, pumpStatus);
      }
      
    }
    else {
      Serial.println("FAILED: " + fbdo.errorReason());
    }

    if (Firebase.RTDB.getBool(&fbdo, "Controls/1/feeder")){
      if (fbdo.dataType() == "boolean"){
      feederStatus = fbdo.boolData();
      if (feederStatus == true){
        s1.write(180);
        Serial.println("Feeder is On");
      }else {
        s1.write(0);
        Serial.println("Feeder is Off");
      }
      }
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }

    Serial.print("");
    Serial.println("_______________________________________");

  }

}