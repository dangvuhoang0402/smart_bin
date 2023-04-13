//HC-SR04
const int trigPin_2 = 5;
const int echoPin_2 = 18;
const int trigPin_1=12;
const int echoPin_1=14;
#define SOUND_SPEED 0.034
#define CM_TO_INCH 0.393701
long duration;
float distanceCm_1;
float distanceCm_2;
//MG90S
#include <ESP32Servo.h>
int servoPin =4 ;
Servo myservo;
int i=10; 
int State=0;
int cap=0;

//Location,Web
#include <Arduino.h>
#include <WiFi.h>
#include <WifiLocation.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#if __has_include("wificonfig.h")
#include "wificonfig.h"
#else
const char* googleApiKey = "YOUR_GEOLOCATION_KEY";
const char* ssid = "YOUR_WIFI_NAME";
const char* passwd = "YOUR_WIFI_PASS";
#endif
#define API_KEY "YOUR_FIREBASE_API_KEY"
#define DATABASE_URL "YOUR_FIREBASE_URL"
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
unsigned long sendDataPrevMillis = 0;
int count = 0;
bool signupOK = false;
WifiLocation location (googleApiKey);
location_t loc;

void setClock () {
    configTime (0, 0, "pool.ntp.org", "time.nist.gov");

    Serial.print ("Waiting for NTP time sync: ");
    time_t now = time (nullptr);
    while (now < 8 * 3600 * 2) {
        delay (500);
        Serial.print (".");
        now = time (nullptr);
    }
    struct tm timeinfo;
    gmtime_r (&now, &timeinfo);
    Serial.print ("\n");
    Serial.print ("Current time: ");
    Serial.print (asctime (&timeinfo));
}

void setup() {
  //setup servo
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  myservo.setPeriodHertz(50);
  myservo.attach(servoPin, 500, 2400);
  //setup hc_sr04
  pinMode(trigPin_1, OUTPUT);
  pinMode(echoPin_1, INPUT);
  pinMode(trigPin_2, OUTPUT);
  pinMode(echoPin_2, INPUT);
  Serial.begin(115200);
  //Wifi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, passwd);
  while (WiFi.status() != WL_CONNECTED) {
   Serial.print("Attempting to connect to WPA SSID: ");
   Serial.println(ssid);
   // wait 5 seconds for connection:
   Serial.print("Status = ");
   Serial.println(WiFi.status());
   delay(500);
  }
  Serial.println ("Connected");
  //Geolocation
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
  setClock ();
  //get location
  loc = location.getGeoFromWiFi();  
}
 
void loop() {
    //read hc_sr04
    digitalWrite(trigPin_1, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin_1, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin_1, LOW);
    duration = pulseIn(echoPin_1, HIGH);
    distanceCm_1 = duration * SOUND_SPEED/2;
    if (State==0)
    {
      digitalWrite(trigPin_2, LOW);
      delayMicroseconds(2);
      digitalWrite(trigPin_2, HIGH);
      delayMicroseconds(10);
      digitalWrite(trigPin_2, LOW);
      duration = pulseIn(echoPin_2, HIGH);
      distanceCm_2 = duration * SOUND_SPEED/2;
    }
    if (distanceCm_2>=20) cap=0;
    else if (distanceCm_2>=10) cap=50;
    else if (distanceCm_2>=5) cap=75;
    else cap=100;
    //servo
    if (distanceCm_1>=20){
      while (i>=10)
      {
        myservo.write(i);
        delay(100);
        i=i-10;
        State=0;
      }
      delay (1000);
    }
    else {
      while (i<=90)
      {
        myservo.write(i);
        delay(100);
        i=i+10;
        State=1;
      }
      delay(1000);
    }
  //Get location and upload to firebase
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 5000 || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();
    // Write Latitude
    if (Firebase.RTDB.setFloat(&fbdo, "test/Latitude", loc.lat)){
      Serial.println("Latitude PASSED:"+String(loc.lat,7));
    }
    else {
      Serial.println("Latitude FAILED");
    }
    //Write Longitude
    if (Firebase.RTDB.setFloat(&fbdo, "test/Longitude", loc.lon)){
      Serial.println("Longitude PASSED:"+String(loc.lon,7));
     }
     else {
      Serial.println("Longitude FAILED");
    }
    //Write Capacity
    if (Firebase.RTDB.setInt(&fbdo, "test/Capacity", cap)){
      Serial.println("Capacity PASSED:"+String(cap));
     }
     else {
      Serial.println("Capacity FAILED");
    }
    //Write state
    if (State==1)
    {
      if (Firebase.RTDB.setString(&fbdo, "test/state", "Đang sử dụng")){
    Serial.println("State PASSED:"+String(State));
    }
    else {
     Serial.println("State FAILED");
    }
    }
    else
    {
    if (Firebase.RTDB.setString(&fbdo, "test/state", "Không sử dụng")){
    Serial.println("State PASSED:"+String(State));
    }
    else {
     Serial.println("State FAILED");
    }
    }
    Serial.println(distanceCm_1);
  }
}
