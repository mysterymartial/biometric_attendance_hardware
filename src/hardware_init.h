#ifndef HARDWARE_INIT_H
#define HARDWARE_INIT_H

#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <RTClib.h>
#include <Adafruit_Fingerprint.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <EEPROM.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#define SDA_PIN 21
#define SCL_PIN 23
#define RXD2 18
#define TXD2 19

const int rowPins[4] = {13, 12, 14, 27}; 
const int colPins[4] = {26, 25, 33, 32};

const int FIRST_BOOT_FLAG_ADDR = 4;       
const char ENCRYPTION_KEY = 0x5A;         

extern LiquidCrystal_I2C lcd;
extern RTC_PCF8563 rtc;
extern HardwareSerial mySerial;
extern Adafruit_Fingerprint finger;
extern WiFiClientSecure wifiClient;
extern PubSubClient client;
extern WiFiUDP ntpUDP;
extern NTPClient timeClient;
extern char keys[4][4];  
extern char pin[4];     


extern bool wifiConnected;
extern bool mqttConnected;
extern int connectionAttempts;
char getKey();


void initHardware();
void initLCD();
void initRTC();
void initFingerprint();
void initKeypad();
void initWiFiAndMQTT();
bool fetchCurrentTime(DateTime &dt);
void savePinToEEPROM();  
void decryptPin(char* pinData); 
void publishAttendanceData(const char* id, const char* status); 

uint8_t autoEnrollFingerprint();
uint8_t getFingerprintID();
String getFingerprintString(bool tryAutoEnroll = false);
bool reconnectWiFiIfNeeded();
bool reconnectMQTTIfNeeded();

inline bool isTimeAllowedForAttendanceWithTime(DateTime now) {
  Serial.print("Current time: ");
  Serial.print(now.year());
  Serial.print("-");
  Serial.print(now.month());
  Serial.print("-");
  Serial.print(now.day());
  Serial.print(" ");
  Serial.print(now.hour());
  Serial.print(":");
  Serial.print(now.minute());
  Serial.print(":");
  Serial.print(now.second());
  Serial.print(" Day of week: ");
  Serial.println(now.dayOfTheWeek());
  
  uint8_t dayOfWeek = now.dayOfTheWeek();
  if (dayOfWeek == 0) dayOfWeek = 7; 
  
  if (dayOfWeek > 5) {
    Serial.println("Weekend - attendance not allowed");
    return false;
  }
  
  int hour = now.hour();
  if (hour < 7 || hour >= 17) {
    Serial.println("Outside work hours - attendance not allowed");
    return false;
  }
  
  Serial.println("Attendance allowed at this time");
  return true;
}



#endif
