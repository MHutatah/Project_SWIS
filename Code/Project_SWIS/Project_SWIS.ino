/*******************************************************************************
 *          This Program is Written by Mohammed Alghamdi (@MHutatah)           *
 *                    Computer Engineering B.S. student                        *
 *                            Taif University                                  *
 *                                                                             *
 *Description:                                                                 *
 *This Program is the hardware script to run an Arduino Uno WiFi Rev2, that is *
 *part of the Smart Water Irrigation System (SWIS), SWIS is the senior year    *
 *capstone graduation project for a group of Computer Engineering students at  *
 *Taif University (2022 Cohort).                                               *
 *                                                                             *
 *                                                                             *
 *All Rights are reserved to the Author.                                       *
 ******************************************************************************/

#include <SPI.h>
#include <SD.h>
#include <WiFiNINA.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <TimeLib.h>
#include <SR04.h>
#include <DS3231.h>
#include <Wire.h>
#include <NTPClient.h>
#include <Firebase_Arduino_WiFiNINA.h>
#include <ArduinoJson.h>

//FirebaseConfig 
#define tank_id 1234
#define tank_access_key 11223344
#define wifi_ssid ""
#define wifi_pass "" 
#define fire_host ""
#define fire_auth ""

//SensorsConfig
#define tds_meter A0
#define ph_meter A1
#define flow_rate 3
#define TRIG_PIN 2
#define ECHO_PIN 4
#define readingsNum 60
bool T = true;
bool F = false;



FirebaseData SWIS;
DS3231 clock1;
// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
SR04 sr04 = SR04(ECHO_PIN,TRIG_PIN);

//vars for different sensors readings
float distance[readingsNum];
float tds_meter_value[readingsNum];
float ph_meter_value[readingsNum];
float flow_rate_value[readingsNum];
unsigned long timestamp;            // Variable to save current epoch time
char buff[50];                      // Buffer for converting long int to String in void loop();
String database_path;               // Variable to store the final hierarchy of readings in a JSON string format


//Start of Arduino Uno WiFi Rev2 Built-in LED color change functions
void turnLEDpurple() {
  WiFi.setLEDs(60,0,60);
}
void turnLEDyellow() {
  WiFi.setLEDs(60,60,0);
}
void turnLEDred() {
  WiFi.setLEDs(60,0,0);
}
void turnLEDgreen() {
  WiFi.setLEDs(0,60,0);
}
void turnLEDoff(){
  WiFi.setLEDs(0,0,0);
}
// end of LED functions

//function to exit the program
void exitProgram(){
  Serial.println("Exiting Program!");
  exit(0);
}

//This function syncs the internal clock with an NTP time server
void syncWithNTP(){
  timeClient.update();
  time_t now = timeClient.getEpochTime();
  Serial.println("NTP Server Time: ");
  Serial.println(now);
  clock1.setEpoch(now);
}

//This function converts regular time format to UNIX epoch time format for better timestamps representation
time_t tmepoch_Convert(int YYYY, byte MM, byte DD, byte hh, byte mm, byte ss){
  tmElements_t tmSet;
  tmSet.Year = YYYY;
  tmSet.Month = MM;
  tmSet.Day = DD;
  tmSet.Hour = hh;
  tmSet.Minute = mm;
  tmSet.Second = ss;
  return makeTime(tmSet); 
}


//This Subroutine is to upload data when the interent connection is avalaible and everything is working as expected.
void onlineSubroutine(){
  Serial.print("Sending Data to server: ");
  Serial.println(String(database_path));
  if(Firebase.updateNode(SWIS, "/User_Data/"+String(tank_id), database_path)){
    Serial.print("Data sent: ");
    Serial.println(database_path);
  }
}

//This Subroutine is to log data to the MicroSD Card Module when internet is not avalaible.
void offlineSubroutine(){
  
}

void setup() {
  // establishing differnet communictation methods.
  Serial.begin(9600);
  Wire.begin();


  //start of WiFi and Internet connection script
  while(WiFi.status() == WL_NO_MODULE){
    Serial.println("Failed to connect to WiFI Module!");
    turnLEDpurple();
    exitProgram();
    }
  
  int wifi_timeout = 5;
  while (WiFi.begin(wifi_ssid, wifi_pass) != 3 && wifi_timeout > 0){
    Serial.print("Connection to WiFi Network (");
    Serial.print(wifi_ssid);
    Serial.println(") Failed!");
    turnLEDred();
    delay(5000);
    turnLEDoff();
    wifi_timeout-=1;
    }
  if (wifi_timeout <= 0){
    turnLEDred();
    return;
   }
  else{
    Serial.print("Connected to WiFi Network (");
    Serial.print(wifi_ssid);
    Serial.println(") Successfully!");
   }
    //This last block is to sync the RTC module with Online NTP time server
   if (clock1.getYear() - 30 < 0){
    Serial.println("Syncing RTC Module with Server...");
    syncWithNTP();
    Serial.println("Syncing Done!");
   }
  //End of WiFi and Internet connection script

  //Establishing Connection to Firebase
  Serial.println("Connecting to Server!");
  Firebase.begin(fire_host, fire_auth, wifi_ssid, wifi_pass);
  int firebase_timeout = 5;
  while(!Firebase.setBool(SWIS, "/online", true) && firebase_timeout > 0){
    Serial.println("Connection to Firebase Database Failed!");
    turnLEDyellow();
    delay(2500);
    turnLEDoff();
    delay(2500);
    firebase_timeout-=1;
  }
  if (firebase_timeout <= 0){
    turnLEDyellow();
    return;
   }
  else{
    Serial.println("Connected to Firebase Database Successfully!");
    turnLEDgreen();
   }
}

void loop() {
  
  float total_distance = 0.00;
  float total_tds_meter_value= 0.00;
  float total_ph_meter_value= 0.00;
  float total_flow_rate_value= 0.00;
  
  for(int i=0; i<readingsNum; i++) {
    Serial.print("collecting data, pass: ");
    Serial.println(i);
    distance[i] = sr04.Distance();
    Serial.println(distance[i]);
    tds_meter_value[i] = analogRead(tds_meter);
    Serial.println(tds_meter_value[i]);
    ph_meter_value[i] = analogRead(ph_meter);
    Serial.println(ph_meter_value[i]);
    flow_rate_value[i] = flow_rate;
    Serial.println(flow_rate_value[i]);
  }
  Serial.println("Calculating Averages!");
  for(int i=0; i<readingsNum; i++){
    total_distance += distance[i];
    total_tds_meter_value += tds_meter_value[i];
    total_ph_meter_value += ph_meter_value[i];
    total_flow_rate_value += flow_rate_value[i];
  }

  //calling timestamp calculation function
  Serial.print("Calculating timestamp: ");
  timestamp = tmepoch_Convert(clock1.getYear(), clock1.getMonth(T), clock1.getDate(), clock1.getHour(F,T), clock1.getMinute(), clock1.getSecond());
  Serial.println(timestamp);

  //JSON document creation and serialization
  StaticJsonDocument<60> JsonDoc;
  String parent = String(ltoa(timestamp,buff,10));
  JsonObject jsonstring = JsonDoc.createNestedObject(parent);
  jsonstring["Distance"] = total_distance / readingsNum;
  jsonstring["TDS"] = total_tds_meter_value / readingsNum;
  jsonstring["PH"] = total_ph_meter_value / readingsNum;
  jsonstring["Flowrate"] = total_flow_rate_value / readingsNum;
  serializeJson(JsonDoc, database_path);
  
  //calling the appropiate subroutine based on the Internet Connection status
  if (WiFi.status() == 3 && Firebase.setBool(SWIS, "/online", true)){
    onlineSubroutine();
  }
  else{
    offlineSubroutine();
  }
  database_path = "";
  delay(5000);

}
