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
#include "Firebase_Arduino_WiFiNINA.h"
#include "ArduinoJson.h"


//FirebaseConfig 
#define tank_id "tank001"
#define tank_access_key 11223344
#define wifi_ssid ""
#define wifi_pass "" 
#define fire_host ""
#define fire_auth ""    


//SensorsConfig
#define tds_meter A0
#define ph_meter A1
#define flow_rate A2
#define TRIG_PIN 2
#define ECHO_PIN 4
#define CHIP_SEL 10
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
bool syncWithNTP(){
  if(timeClient.update()){
    Serial.print("NTP:");
    Serial.println(timeClient.getEpochTime());
    time_t now = timeClient.getEpochTime();
    Serial.println("NTP Server Time: ");
    Serial.println(now);
    clock1.setEpoch(now);
    return T;
  }
  return F;
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
  if(SD.exists("LOG.TXT")){
    File LocalData;
    String BuffLine = ""; 
    int LastPosition=0;
  
    LocalData = SD.open("LOG.TXT");
    int totalBytes = LocalData.size();
  
    while (LocalData.available()){
      for(LastPosition=0; LastPosition<= totalBytes; LastPosition++){
        char caracter=LocalData.read();
        BuffLine=BuffLine+caracter;
        if(caracter==10 ){            //ASCII new line
          if(Firebase.updateNode(SWIS, "/User_Data/"+String(tank_id), BuffLine)){
            Serial.print("Local Data sent: ");
            Serial.println(BuffLine);
          }
          BuffLine="";
        }
      }
    }
    LocalData.close();
    SD.remove("LOG.TXT");
      
  }
  Serial.print("Sending Data to server: ");
  Serial.println(String(database_path));
  if(Firebase.updateNode(SWIS, "/User_Data/"+String(tank_id), database_path)){
    Serial.print("Data sent: ");
    Serial.println(database_path);
  }
}

//This Subroutine is to log data to the MicroSD Card Module when internet is not avalaible.
void offlineSubroutine(){
  File Local_Data;
  Local_Data = SD.open("LOG.TXT", FILE_WRITE);
  Local_Data.println(database_path);
  Local_Data.close();

  Serial.print("Local Data Saved: ");
  Serial.println(database_path);
}

void setup() {
  // establishing differnet communictation methods.
  Serial.begin(9600);
  Wire.begin();

  if (SD.begin(CHIP_SEL)){
    Serial.println("SD Card is ready to use!");
  }
  else{
    Serial.println("SD card failed to initialize!");
  }

  //start of WiFi and Internet connection script
  while(WiFi.status() == WL_NO_MODULE){
    Serial.println("Failed to connect to WiFi Module!");
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
    Serial.println("Syncing RTC Module with Server...");
    if(syncWithNTP()){
      Serial.println("Syncing Done!");  
    }
    else{
      Serial.println("Syncing Failed!"); 
    }
  //End of WiFi and Internet connection script

  //Establishing Connection to Firebase
  Serial.println("Connecting to Server!");
  Firebase.begin(fire_host, fire_auth, wifi_ssid, wifi_pass);
  int firebase_timeout = 10;
  while(!Firebase.setBool(SWIS, "/online", true) && firebase_timeout > 0){
    Serial.println("Connection to Firebase Database Failed!");
    turnLEDyellow();
    delay(2500);
    turnLEDoff();
    delay(2500);
    firebase_timeout-=1;
    WiFi.disconnect();                  // Added for the same library bug explained in the loop function
    WiFi.begin(wifi_ssid, wifi_pass);   // Added for the same library bug explained in the loop function
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

  int fr_pulse_begin;
  int fr_pulse_end;
  float fr_time = 0;
  float fr_freq = 0;
  float total_distance = 0.00;
  float total_tds_meter_value= 0.00;
  float total_ph_meter_value= 0.00;
  float total_flow_rate_value= 0.00;

  //flow rate calculation
  fr_pulse_begin = pulseIn(flow_rate, HIGH);
  fr_pulse_end = pulseIn(flow_rate, LOW);
  fr_time = fr_pulse_begin+fr_pulse_end;
  if(fr_time > 0){
    fr_freq = 1000000/fr_time;
  }
  total_flow_rate_value = fr_freq/7.5;

  //storing multiple reads of sensors inputs for smooth readings
  for(int i=0; i<readingsNum; i++) {
    if ((i+1)%10 == 0){
    Serial.print("collecting data, pass: ");
    Serial.println(i+1);
    }
    distance[i] = sr04.Distance();
    tds_meter_value[i] = analogRead(tds_meter);
    ph_meter_value[i] = analogRead(ph_meter);
  }
  Serial.println("Calculating Averages!");
  for(int i=0; i<readingsNum; i++){
    total_distance += distance[i];
    total_tds_meter_value += tds_meter_value[i];
    total_ph_meter_value += ph_meter_value[i];
  }

  //calling timestamp calculation function
  Serial.print("Calculating timestamp: ");
  timestamp = tmepoch_Convert(clock1.getYear(), clock1.getMonth(T), clock1.getDate()+1, clock1.getHour(F,T), clock1.getMinute(), clock1.getSecond());
  Serial.println(timestamp);

  //JSON document creation and serialization
  StaticJsonDocument<100> JsonDoc;
  String parent = String(ltoa(timestamp,buff,10));
  JsonObject jsonstring = JsonDoc.createNestedObject(parent);
  jsonstring["device_id"] = tank_id;
  jsonstring["timestamp"] = long(timestamp);
  jsonstring["distance"] = total_distance / readingsNum;
  jsonstring["ph_readings"] = -5.7 * (total_ph_meter_value*5.0/1024/readingsNum)+26;
  jsonstring["tds_readings"] = total_tds_meter_value / readingsNum;
  jsonstring["water_flow_rate"] = total_flow_rate_value;
  serializeJson(JsonDoc, database_path);
  
  //calling the appropiate subroutine based on the Internet Connection status
  /*
   * The code in this section is organized this way because of a bug in the WiFiNINA library, where the WiFi status will change
   * to status 6 then status 255 if repetaed connections to a socket is failed.
   * when the bug is fixed, the code can be organized in a regular manner of: if, else if, else
   * P.S. you will need to remove the assingment of (database_path) and (return;) from the first (if statement) as they will no longer be necessary.
   */
  Serial.println(WiFi.status());
  if (WiFi.status() == 3){
    Firebase.begin(fire_host, fire_auth, wifi_ssid, wifi_pass);
    if (Firebase.setBool(SWIS, "/online", true)){
      turnLEDgreen();
      onlineSubroutine();
      database_path = "";
      return;
    }
    else{
      turnLEDyellow();
      offlineSubroutine();
      database_path = "";
      return;
    }
  }
  WiFi.disconnect();
  WiFi.begin(wifi_ssid, wifi_pass);
  if (WiFi.begin(wifi_ssid, wifi_pass) == 3){
    Firebase.begin(fire_host, fire_auth, wifi_ssid, wifi_pass);
    if (Firebase.setBool(SWIS, "/online", true)){
      turnLEDgreen();
      onlineSubroutine();
    }
    else{
      turnLEDyellow();
      offlineSubroutine();
    }
  }
  else{
    turnLEDred();
    offlineSubroutine();
  }
  database_path = "";
  delay(1000);

}
