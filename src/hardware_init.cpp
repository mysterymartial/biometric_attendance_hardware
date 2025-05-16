#include "hardware_init.h"

LiquidCrystal_I2C lcd(0x27, 16, 2); 
RTC_PCF8563 rtc;
HardwareSerial mySerial(1);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);
WiFiClientSecure wifiClient;
PubSubClient client(wifiClient);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "ng.pool.ntp.org", 3600, 60000);
char keys[4][4] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

char pin[4] = {'0', '0', '0', '0'}; 


bool wifiConnected = false;
bool mqttConnected = false;
int connectionAttempts = 0;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 200;

void initHardware() {
  Serial.begin(9600);
  EEPROM.begin(512);
  
  Wire.begin(SDA_PIN, SCL_PIN);
  
  initLCD();
  
  connectionAttempts = 0;
  initWiFiAndMQTT();
  
  initRTC();          
  
  initFingerprint();
  
  initKeypad();
}

void initLCD() {
  Serial.println("Starting LCD initialization...");
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);
  lcd.init();
  delay(100); 
  Serial.println("LCD initialized");
  lcd.backlight();
  Serial.println("LCD backlight on");
  lcd.setCursor(0, 0);
  lcd.print("Welcome to");
  lcd.setCursor(0, 1);
  lcd.print("Semicolon!");
  Serial.println("Text sent to LCD: Welcome to Semicolon!");
  delay(2000);
  lcd.clear();
}

bool fetchCurrentTime(DateTime &dt) {
  timeClient.begin();
  bool success = false;
  for (int i = 0; i < 5; i++) {
    if (timeClient.update()) {
      success = true;
      break;
    }
    Serial.println("Retrying NTP update...");
    delay(1000);
  }
  
  if (!success) {
    Serial.println("Failed to fetch time from NTP server after multiple attempts!");
    return false;
  }
  
  unsigned long epochTime = timeClient.getEpochTime();
  
  Serial.print("NTP time received: ");
  Serial.print(timeClient.getFormattedTime());
  Serial.print(" (Epoch: ");
  Serial.print(epochTime);
  Serial.println(")");
  
  dt = DateTime(epochTime);
  if (dt.year() < 2023 || dt.year() > 2024) {
    Serial.println("Warning: NTP returned an unusual year: " + String(dt.year()));
  }
  
  return true;
}


void initRTC() {
  if (!rtc.begin()) {
    Serial.println("RTC not found!");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("RTC Failed!");
    while (1);
  }
  
  uint8_t firstBootFlag = EEPROM.read(FIRST_BOOT_FLAG_ADDR);
  
  DateTime dt;
  if (fetchCurrentTime(dt)) {
    rtc.adjust(dt);
    
    if (firstBootFlag != 1) {
      EEPROM.write(FIRST_BOOT_FLAG_ADDR, 1);
      EEPROM.commit();
    }
    
    Serial.println("RTC set to current time via NTP");
    Serial.print("Current time set to: ");
    Serial.print(dt.year());
    Serial.print("-");
    Serial.print(dt.month());
    Serial.print("-");
    Serial.print(dt.day());
    Serial.print(" ");
    Serial.print(dt.hour());
    Serial.print(":");
    Serial.print(dt.minute());
    Serial.print(":");
    Serial.println(dt.second());
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("RTC Sync Success!");
    delay(1000);
  } else {
    if (firstBootFlag != 1) {
  rtc.adjust(DateTime(2023, 1, 1, 0, 0, 0)); 
      Serial.println("Failed to fetch NTP time, using default");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("RTC Default Set!");
      delay(1000);
    } else {
      Serial.println("Failed to update RTC from NTP, keeping current time");
    }
  }
}


void initFingerprint() {
  mySerial.begin(57600, SERIAL_8N1, RXD2, TXD2);
  if (!finger.verifyPassword()) {
    Serial.println("Fingerprint sensor not found!");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Finger Sensor Err!");
    while (1);
  }
  
  finger.setSecurityLevel(3);
  
  uint8_t params = finger.getParameters();
  Serial.print("Fingerprint sensor status: ");
  Serial.print("Capacity: "); Serial.print(finger.capacity);
  Serial.print(", Security level: "); Serial.print(finger.security_level);
  
  Serial.println("Fingerprint sensor initialized");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Finger Sensor OK!");
  delay(1000);
}


bool checkDuplicateFingerprint() {
  Serial.println("Checking for duplicate fingerprint...");
  
  uint8_t p = finger.fingerFastSearch();
  
  if (p == FINGERPRINT_OK) {
    Serial.print("Found duplicate fingerprint at ID #"); 
    Serial.println(finger.fingerID);
    Serial.print("With confidence of "); 
    Serial.println(finger.confidence);
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Duplicate found!");
    lcd.setCursor(0, 1);
    lcd.print("ID #"); lcd.print(finger.fingerID);
    delay(2000);
    return true;
  } 
  else if (p == FINGERPRINT_NOTFOUND) {
    Serial.println("No duplicate fingerprint found");
    return false;
  }
  else {
    Serial.print("Error in duplicate check: "); 
    Serial.println(p);
    return false; 
  }
}

void initKeypad() {
  Serial.println("Initializing keypad with corrected mapping...");
  
  for (int i = 0; i < 4; i++) {
    pinMode(rowPins[i], INPUT);
    pinMode(colPins[i], INPUT);
  }
  
  delay(50); 
  
  for (int i = 0; i < 4; i++) {
    pinMode(colPins[i], OUTPUT);
    digitalWrite(colPins[i], HIGH);
  }
  
  for (int i = 0; i < 4; i++) {
    pinMode(rowPins[i], INPUT_PULLUP);
  }
  
  keys[0][0] = '1'; keys[0][1] = '2'; keys[0][2] = '3'; keys[0][3] = 'A';
  keys[1][0] = '4'; keys[1][1] = '5'; keys[1][2] = '6'; keys[1][3] = 'B';
  keys[2][0] = '7'; keys[2][1] = '8'; keys[2][2] = '9'; keys[2][3] = 'C';
  keys[3][0] = '*'; keys[3][1] = '0'; keys[3][2] = '#'; keys[3][3] = 'D';
  
  Serial.println("Keypad initialized with standard mapping");
  
  Serial.print("Row pins: ");
  for (int i = 0; i < 4; i++) {
    Serial.print(rowPins[i]);
    if (i < 3) Serial.print(", ");
  }
  Serial.println();
  
  Serial.print("Column pins: ");
  for (int i = 0; i < 4; i++) {
    Serial.print(colPins[i]);
    if (i < 3) Serial.print(", ");
  }
  Serial.println();
  
  Serial.println("Testing keypad initialization and mapping...");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Keypad test");
  lcd.setCursor(0, 1);
  lcd.print("Press any key");
  
  for (int col = 0; col < 4; col++) {
    digitalWrite(colPins[col], LOW);
    delayMicroseconds(500);
    
    Serial.print("Column ");
    Serial.print(col);
    Serial.print(" set LOW, row states: ");
    for (int row = 0; row < 4; row++) {
      int pinState = digitalRead(rowPins[row]);
      Serial.print(pinState);
      Serial.print(" ");
      
      if (pinState == LOW) {
        Serial.print(" (Key '");
        Serial.print(keys[row][col]);
        Serial.print("' detected) ");
      }
    }
    Serial.println();
    
    digitalWrite(colPins[col], HIGH);
    delayMicroseconds(500);
  }
  
  delay(1000);
}

char getKey() {
  if ((millis() - lastDebounceTime) < debounceDelay) {
    return '\0'; 
  }
  
  for (int c = 0; c < 4; c++) {
    digitalWrite(colPins[c], HIGH);
  }
  
  delayMicroseconds(500); 
  
  for (int col = 0; col < 4; col++) {
    digitalWrite(colPins[col], LOW);

    delayMicroseconds(1000);
    
    for (int row = 0; row < 4; row++) {
      
      int reading = digitalRead(rowPins[row]);
      delayMicroseconds(100);
      int reading2 = digitalRead(rowPins[row]);

      if (reading == LOW && reading2 == LOW) {
      
        digitalWrite(colPins[col], HIGH);
        
        char keyValue = keys[row][col];
        Serial.print("Key pressed at row ");
        Serial.print(row);
        Serial.print(", col ");
        Serial.print(col);
        Serial.print(", value: ");
        Serial.print(keyValue);
        Serial.print(", ASCII: ");
        Serial.println((int)keyValue);
        
        if (keyValue == 'k') {
            Serial.println("Correcting 'k' to '1'");
            keyValue = '1';
        } 
        else if (keyValue == 'i') {
            Serial.println("Correcting 'i' to '3'");
            keyValue = '3';
        }
        
        lastDebounceTime = millis();
        
        unsigned long pressStart = millis();
        while (digitalRead(rowPins[row]) == LOW) {
          
          if (millis() - pressStart > 1000) break;
          delay(10);
        }
        
        delay(50);
        
        return keyValue;
      }
    }
  
    digitalWrite(colPins[col], HIGH);
    delayMicroseconds(500); 
  }
  
  return '\0';
}


void initWiFiAndMQTT() {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(1000);
  
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  
  Serial.println("\n=== WiFi Connection Attempt ===");
  Serial.print("Attempting to connect to SSID: ");
  Serial.println(WIFI_SSID);
  Serial.print("With password length: ");
  Serial.println(strlen(WIFI_PASSWORD));
  
  int numNetworks = WiFi.scanNetworks();
  bool networkFound = false;
  int networkSignal = 0;
  
  for (int i = 0; i < numNetworks; i++) {
    if (String(WiFi.SSID(i)) == String(WIFI_SSID)) {
      networkFound = true;
      networkSignal = WiFi.RSSI(i);
      Serial.print("Target network found with signal strength: ");
      Serial.print(networkSignal);
      Serial.println(" dBm");
      break;
    }
  }
  
  if (!networkFound) {
    Serial.println("WARNING: Target network not found in scan results!");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Network not");
    lcd.setCursor(0, 1);
    lcd.print("found!");
    delay(2000);
  }
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting to");
  lcd.setCursor(0, 1);
  lcd.print("WiFi (1/3)...");
  
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  
  WiFi.disconnect();
  delay(500);
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  unsigned long startTime = millis();
  const unsigned long timeout = 10000; 
  int dotCount = 0;
  
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < timeout) {
    delay(500);
    Serial.print(".");
    lcd.setCursor(14, 1);
    dotCount = (dotCount + 1) % 4;
    lcd.print("    "); 
    lcd.setCursor(14, 1);
    for (int i = 0; i < dotCount; i++) {
      lcd.print(".");
    }
  }
  

  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.println("\nConnected to WiFi using Method 1");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Connected!");
    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP().toString().substring(0, 16));
    delay(1000);
    
    testDNSResolution();
    
    setupMQTT();
    return;
  }
  
  if (networkFound) {
    Serial.println("\nTrying Method 2: Connection with specific parameters");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Connecting to");
    lcd.setCursor(0, 1);
    lcd.print("WiFi (2/3)...");
    
    WiFi.disconnect(true);
    delay(1000);
    
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    startTime = millis();
    const unsigned long timeout2 = 15000; 
    
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < timeout2) {
      delay(500);
      Serial.print("+");
      
      if ((millis() - startTime) % 3000 < 100) {
        Serial.print(" [Status: ");
        Serial.print(WiFi.status());
        Serial.println("]");
      }
      
      lcd.setCursor(14, 1);
      dotCount = (dotCount + 1) % 4;
      lcd.print("    ");
      lcd.setCursor(14, 1);
      for (int i = 0; i < dotCount; i++) {
        lcd.print(".");
      }
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      wifiConnected = true;
      Serial.println("\nConnected to WiFi using Method 2");
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());
      
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("WiFi Connected!");
      lcd.setCursor(0, 1);
      lcd.print(WiFi.localIP().toString().substring(0, 16));
      delay(1000);
      
      testDNSResolution();
      
      setupMQTT();
      return;
    }
  }
  
  Serial.println("\nTrying Method 3: Connection with static IP");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting to");
  lcd.setCursor(0, 1);
  lcd.print("WiFi (3/3)...");
  
  WiFi.disconnect(true);
  delay(1000);
  
  IPAddress staticIP(192, 168, 1, 200);  
  IPAddress gateway(192, 168, 1, 1);     
  IPAddress subnet(255, 255, 255, 0);
  IPAddress dns1(8, 8, 8, 8);            
  IPAddress dns2(8, 8, 4, 4);
  
  if (!WiFi.config(staticIP, gateway, subnet, dns1, dns2)) {
    Serial.println("Static IP configuration failed");
  }
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  startTime = millis();
  const unsigned long timeout3 = 20000; 
  
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < timeout3) {
    delay(500);
    Serial.print("*");
    
    if ((millis() - startTime) % 3000 < 100) {
      Serial.print(" [Status: ");
      Serial.print(WiFi.status());
      Serial.println("]");
    }
    
    lcd.setCursor(14, 1);
    dotCount = (dotCount + 1) % 4;
    lcd.print("    ");
    lcd.setCursor(14, 1);
    for (int i = 0; i < dotCount; i++) {
      lcd.print(".");
    }
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.println("\nConnected to WiFi using Method 3 (Static IP)");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Connected!");
    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP().toString().substring(0, 16));
    delay(1000);
    
    testDNSResolution();
    
    setupMQTT();
    return;
  }
  
  wifiConnected = false;
  Serial.println("\n=== All WiFi connection methods failed ===");
  Serial.println("Detailed diagnosis:");
  diagnosisWiFi();
  
  for (int i = 0; i < numNetworks; i++) {
    String ssid = WiFi.SSID(i);
    if (ssid == "Semicolon-Village" || ssid == "Semicolon-Village 2g") {
      Serial.print("\nTrying alternative network: ");
      Serial.println(ssid);
      
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Trying alt net:");
      lcd.setCursor(0, 1);
      lcd.print(ssid.substring(0, 16));
      
      WiFi.disconnect(true);
      delay(1000);
      WiFi.begin(ssid.c_str(), ""); 
      
      startTime = millis();
      const unsigned long altTimeout = 10000;
      
      while (WiFi.status() != WL_CONNECTED && millis() - startTime < altTimeout) {
        delay(500);
        Serial.print("~");
      }
      
      if (WiFi.status() == WL_CONNECTED) {
        wifiConnected = true;
        Serial.println("\nConnected to alternative network");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
        
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Alt WiFi OK!");
        lcd.setCursor(0, 1);
        lcd.print(WiFi.localIP().toString().substring(0, 16));
        delay(1000);
        
        testDNSResolution();
        
        setupMQTT();
        return;
      }
    }
  }
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Continuing in");
  lcd.setCursor(0, 1);
  lcd.print("offline mode...");
  delay(2000);
  
  Serial.println("\nContinuing in offline mode");
}

void testDNSResolution() {
  Serial.println("Testing DNS resolution...");
  
  IPAddress result;
  if (WiFi.hostByName("www.google.com", result)) {
    Serial.print("DNS resolution for google.com successful: ");
    Serial.println(result.toString());
  } else {
    Serial.println("DNS resolution for google.com failed!");
  }
  
  IPAddress mqttIP;
  String brokerHost = MQTT_BROKER;
  brokerHost.replace("ssl://", "");
  
  Serial.print("Attempting to resolve MQTT broker: ");
  Serial.println(brokerHost);
  
  if (WiFi.hostByName(brokerHost.c_str(), mqttIP)) {
    Serial.print("MQTT broker resolved to: ");
    Serial.println(mqttIP.toString());
  } else {
    Serial.println("Failed to resolve MQTT broker hostname!");
    Serial.println("Will try direct IP connection if available");
  }
}

void diagnosisWiFi() {
  Serial.println("--- WiFi Diagnosis ---");
  
  Serial.print("WiFi Status: ");
  switch (WiFi.status()) {
    case WL_CONNECTED: Serial.println("CONNECTED"); break;
    case WL_NO_SHIELD: Serial.println("NO SHIELD"); break;
    case WL_IDLE_STATUS: Serial.println("IDLE"); break;
    case WL_NO_SSID_AVAIL: Serial.println("NO SSID AVAILABLE"); break;
    case WL_SCAN_COMPLETED: Serial.println("SCAN COMPLETED"); break;
    case WL_CONNECT_FAILED: Serial.println("CONNECTION FAILED"); break;
    case WL_CONNECTION_LOST: Serial.println("CONNECTION LOST"); break;
    case WL_DISCONNECTED: Serial.println("DISCONNECTED"); break;
    default: Serial.println("UNKNOWN");
  }
  
  
  Serial.println("Attempting to scan networks...");
  int numNetworks = WiFi.scanNetworks();
  Serial.println("done");
  
  if (numNetworks == 0) {
    Serial.println("No networks found!");
  } else {
    Serial.print(numNetworks);
    Serial.println(" networks found:");
    
    for (int i = 0; i < numNetworks; i++) {
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(" dBm) ");
      
      switch (WiFi.encryptionType(i)) {
        case WIFI_AUTH_OPEN: Serial.println("Open"); break;
        case WIFI_AUTH_WEP: Serial.println("WEP"); break;
        case WIFI_AUTH_WPA_PSK: Serial.println("WPA/PSK"); break;
        case WIFI_AUTH_WPA2_PSK: Serial.println("WPA2/PSK"); break;
        case WIFI_AUTH_WPA_WPA2_PSK: Serial.println("WPA/WPA2/PSK"); break;
        default: Serial.println("Unknown");
      }
    }
  }
  
  bool foundTarget = false;
  for (int i = 0; i < numNetworks; i++) {
    if (String(WiFi.SSID(i)) == String(WIFI_SSID)) {
      foundTarget = true;
      break;
    }
  }
  
  if (!foundTarget) {
    Serial.print("'");
    Serial.print(WIFI_SSID);
    Serial.println("' network not found in scan results!");
  }
  
  IPAddress googleIP;
  Serial.println("Testing DNS resolution with google.com...");
  if (WiFi.hostByName("www.google.com", googleIP)) {
    Serial.print("DNS resolution successful: ");
    Serial.println(googleIP.toString());
  } else {
    Serial.println("DNS resolution failed!");
  }
  
  Serial.println("------------------------");
}


void maintainMQTTConnection() {
  if (!wifiConnected) {
  
    if (WiFi.status() != WL_CONNECTED) {
      reconnectWiFiIfNeeded();
    }
    return;
  }

  if (!client.connected()) {
    Serial.println("MQTT connection lost, attempting to reconnect...");
    String brokerHost = MQTT_BROKER;
    brokerHost.replace("ssl://", "");
    client.setServer(brokerHost.c_str(), MQTT_PORT);
    
    if (client.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
      mqttConnected = true;
      Serial.println("Reconnected to MQTT using hostname");
      client.subscribe("attendance/commands");
    } else {
      tryMQTTFallbackIPs();
    }
  }

  if (mqttConnected) {
    client.loop();
  }
}


void setupMQTT() {
  if (!wifiConnected) {
    Serial.println("Cannot set up MQTT: WiFi not connected");
    return;
  }
  wifiClient.setInsecure(); 
  
  wifiClient.setTimeout(15000); 

  String brokerHost = MQTT_BROKER;
  brokerHost.replace("ssl://", "");
  
  Serial.print("Connecting to MQTT broker: ");
  Serial.println(brokerHost);
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting MQTT");
  
  client.setServer(brokerHost.c_str(), MQTT_PORT);
  
  Serial.println("Attempting MQTT connection...");
  if (client.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
    mqttConnected = true;
    Serial.println("Connected to MQTT broker!");
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("MQTT Connected!");
    delay(1000);
    client.subscribe("attendance/commands");
    Serial.println("Subscribed to MQTT topics");
  } else {
    Serial.print("MQTT connection failed, rc=");
    Serial.println(client.state());
    tryMQTTFallbackIPs();
  }
}

void tryMQTTFallbackIPs() {
  
  const char* MQTT_FALLBACK_IPS[] = {
    "46.137.47.218",
    "54.73.92.158",
    "52.31.149.80"
  };
  
  bool connected = false;
  for (int i = 0; i < 3 && !connected; i++) {
    Serial.print("Trying direct IP connection to MQTT broker: ");
    Serial.println(MQTT_FALLBACK_IPS[i]);
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("MQTT IP try ");
    lcd.print(i+1);
    lcd.setCursor(0, 1);
    lcd.print(MQTT_FALLBACK_IPS[i]);
    
    client.setServer(MQTT_FALLBACK_IPS[i], MQTT_PORT);
    if (client.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
      connected = true;
      mqttConnected = true;
      Serial.println("Connected to MQTT using direct IP!");
      
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("MQTT Connected!");
      lcd.setCursor(0, 1);
      lcd.print("via IP ");
      lcd.print(i+1);
      delay(1000);
      
      client.subscribe("attendance/commands");
      Serial.println("Subscribed to MQTT topics");
      break;
    } else {
      Serial.print("Failed with IP ");
      Serial.print(MQTT_FALLBACK_IPS[i]);
      Serial.print(", rc=");
      Serial.println(client.state());
      delay(1000);
    }
  }
  
  if (!connected) {
    Serial.println("All MQTT connection attempts failed");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("MQTT Failed!");
    lcd.setCursor(0, 1);
    lcd.print("Check credentials");
    delay(1000);
  }
}

bool publishToMQTT(const char* topic, const char* payload) {
  if (!mqttConnected) {
    Serial.println("Cannot publish: MQTT not connected");
    return false;
  }
  
  bool success = client.publish(topic, payload);
  if (success) {
    Serial.print("Published to ");
    Serial.print(topic);
    Serial.print(": ");
    Serial.println(payload);
  } else {
    Serial.print("Failed to publish to ");
    Serial.println(topic);
  }
  
  return success;
}


bool reconnectMQTT() {
  if (mqttConnected && client.connected()) {
    return true; 
  }
  
  if (!wifiConnected) {
    Serial.println("Cannot reconnect to MQTT: WiFi not connected");
    return false;
  }
  
  Serial.println("Attempting to reconnect to MQTT...");
  
  if (client.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
    mqttConnected = true;
    Serial.println("Reconnected to MQTT broker");
    
    client.subscribe("attendance/commands");
    return true;
  } else {
    Serial.print("Failed to reconnect to MQTT, rc=");
    Serial.println(client.state());
    mqttConnected = false;
    return false;
  }
}


bool reconnectWiFiIfNeeded() {
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    return true;
  }
  
  Serial.println("WiFi disconnected. Attempting to reconnect...");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Reconnecting");
  lcd.setCursor(0, 1);
  lcd.print("to WiFi...");
  
  WiFi.disconnect(true);
  delay(1000);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  unsigned long startTime = millis();
  const unsigned long timeout = 10000; 
  
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < timeout) {
    delay(500);
    Serial.print(".");
    lcd.setCursor(14, 1);
    lcd.print(".");
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.println("Reconnected to WiFi");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Reconnected!");
    delay(1000);
    return true;
  } else {
    wifiConnected = false;
    Serial.println("WiFi reconnection failed!");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Reconnect");
    lcd.setCursor(0, 1);
    lcd.print("Failed!");
    delay(1000);
    return false;
  }
}

bool reconnectMQTTIfNeeded() {
  if (!wifiConnected) {
    if (!reconnectWiFiIfNeeded()) {
      return false;
    }
  }
  
  if (client.connected()) {
    mqttConnected = true;
    return true;
  }
  
  Serial.println("MQTT disconnected. Attempting to reconnect...");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Reconnecting");
  lcd.setCursor(0, 1);
  lcd.print("to MQTT...");
  
  wifiClient.setInsecure();
  client.setServer(MQTT_BROKER, MQTT_PORT);
  
  if (client.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
    mqttConnected = true;
    Serial.println("Reconnected to MQTT");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("MQTT Reconnected!");
    delay(1000);
    return true;
  } else {
    mqttConnected = false;
    Serial.println("MQTT reconnection failed! State: " + String(client.state()));
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("MQTT Reconnect");
    lcd.setCursor(0, 1);
    lcd.print("Failed!");
    delay(1000);
    return false;
  }
}

void savePinToEEPROM() {
  char encryptedPin[4];
  memcpy(encryptedPin, pin, 4);
  for (int i = 0; i < 4; i++) {
    encryptedPin[i] ^= ENCRYPTION_KEY;
  }
  for (int i = 0; i < 4; i++) {
    EEPROM.write(i, encryptedPin[i]);
  }
  EEPROM.commit();
}

void decryptPin(char* pinData) {
  for (int i = 0; i < 4; i++) {
    pinData[i] ^= ENCRYPTION_KEY;
  }
}

uint8_t autoEnrollFingerprint(uint8_t id) {
  uint8_t p = -1;
  Serial.println("Waiting for valid finger to enroll...");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Place finger");
  lcd.setCursor(0, 1);
  lcd.print("to enroll...");
  
  unsigned long startTime = millis();
  while (p != FINGERPRINT_OK && millis() - startTime < 15000) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      if ((millis() - startTime) % 1000 < 100) {
        lcd.setCursor(15, 1);
        static int dots = 0;
        lcd.print(dots == 0 ? " " : dots == 1 ? "." : dots == 2 ? ".." : "...");
        dots = (dots + 1) % 4;
      }
      delay(100);
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      break;
    default:
      Serial.println("Unknown error");
      break;
    }
  }
  
  if (p != FINGERPRINT_OK) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Timeout!");
    lcd.setCursor(0, 1);
    lcd.print("No finger detected");
    delay(2000);
    return p;
  }
  
  p = finger.image2Tz(1);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Image too messy");
      lcd.setCursor(0, 1);
      lcd.print("Try again");
      delay(2000);
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Communication");
      lcd.setCursor(0, 1);
      lcd.print("error");
      delay(2000);
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("No features");
      lcd.setCursor(0, 1);
      lcd.print("found");
      delay(2000);
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Invalid image");
      delay(2000);
      return p;
    default:
      Serial.println("Unknown error");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Unknown error");
      delay(2000);
      return p;
  }
  
  if (checkDuplicateFingerprint()) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Finger already");
    lcd.setCursor(0, 1);
    lcd.print("registered!");
    delay(2000);
    return FINGERPRINT_ENROLLMISMATCH; 
  }
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Remove finger");
  Serial.println("Remove finger");
  delay(2000);
  
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }
  
  p = -1;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Place same");
  lcd.setCursor(0, 1);
  lcd.print("finger again");
  Serial.println("Place same finger again");
  
  startTime = millis();
  while (p != FINGERPRINT_OK && millis() - startTime < 15000) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      if ((millis() - startTime) % 1000 < 100) {
        lcd.setCursor(15, 1);
        static int dots = 0;
        lcd.print(dots == 0 ? " " : dots == 1 ? "." : dots == 2 ? ".." : "...");
        dots = (dots + 1) % 4;
      }
      delay(100);
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      break;
    default:
      Serial.println("Unknown error");
      break;
    }
  }
  
  if (p != FINGERPRINT_OK) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Timeout!");
    lcd.setCursor(0, 1);
    lcd.print("Enrollment failed");
    delay(2000);
    return p;
  }
  
  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Image too messy");
      delay(2000);
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Communication");
      lcd.setCursor(0, 1);
      lcd.print("error");
      delay(2000);
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("No features");
      lcd.setCursor(0, 1);
      lcd.print("found");
      delay(2000);
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Invalid image");
      delay(2000);
      return p;
    default:
      Serial.println("Unknown error");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Unknown error");
      delay(2000);
      return p;
  }
  
  Serial.print("Creating model for ID #");
  Serial.println(id);
  
  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    Serial.println("Prints matched!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Communication");
    lcd.setCursor(0, 1);
    lcd.print("error");
    delay(2000);
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    Serial.println("Fingerprints did not match");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Fingerprints");
    lcd.setCursor(0, 1);
    lcd.print("didn't match!");
    delay(2000);
    return p;
  } else {
    Serial.println("Unknown error");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Unknown error");
    delay(2000);
    return p;
  }

  if (finger.confidence < 100) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Low quality");
    lcd.setCursor(0, 1);
    lcd.print("Try again");
    Serial.print("Low confidence score: "); Serial.println(finger.confidence);
    delay(2000);
    return FINGERPRINT_FEATUREFAIL;
  }
  
  Serial.print("Storing model for ID #");
  Serial.println(id);
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Fingerprint");
    lcd.setCursor(0, 1);
    lcd.print("Enrolled as ID #");
    lcd.print(id);
    Serial.println("Stored!");
    delay(2000);
    return p;
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Communication");
    lcd.setCursor(0, 1);
    lcd.print("error");
    delay(2000);
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not store in that location");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Storage location");
    lcd.setCursor(0, 1);
    lcd.print("error");
    delay(2000);
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Flash memory");
    lcd.setCursor(0, 1);
    lcd.print("error");
    delay(2000);
    return p;
  } else {
    Serial.println("Unknown error");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Unknown error");
    delay(2000);
    return p;
  }
}



uint8_t getFingerprintID() {
  uint8_t p = finger.getImage();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      return FINGERPRINT_NOFINGER;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }
  
  p = finger.image2Tz();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }
  
  p = finger.fingerSearch();
    
  if (p == FINGERPRINT_OK) {
    Serial.println("Found a print match!");
    
    if (finger.confidence < 50) {
      Serial.print("Match found but confidence too low: ");
      Serial.println(finger.confidence);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Low confidence");
      lcd.setCursor(0, 1);
      lcd.print("Try again");
      delay(1500);
      return FINGERPRINT_NOTFOUND;
    }
    
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_NOTFOUND) {
    Serial.println("Did not find a match");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }
  
  Serial.print("Found ID #"); Serial.print(finger.fingerID);  
  Serial.print(" with confidence of "); Serial.println(finger.confidence);
  
  return finger.fingerID;
}


bool checkPinFromEEPROM() {
  char storedPin[4];
  for (int i = 0; i < 4; i++) {
    storedPin[i] = EEPROM.read(i);
  }
  decryptPin(storedPin);
  
  for (int i = 0; i < 4; i++) {
    if (pin[i] != storedPin[i]) {
      return false;
    }
  }
  return true;
}

void publishAttendanceData(const char* id, const char* status) {
  if (!wifiConnected || !mqttConnected) {
    if (!reconnectMQTTIfNeeded()) {
      Serial.println("Cannot publish: no connection");
      return;
    }
  }
  
  DateTime now = rtc.now();
  
  char timeStr[20];
  char timeOnlyStr[9];
  
  // Format as YYYY-MM-DDTHH:MM:SS for date
  sprintf(timeStr, "%04d-%02d-%02dT%02d:%02d:%02d",
          now.year(), now.month(), now.day(),
          now.hour(), now.minute(), now.second());
  
  // Format as HH:MM:SS for time
  sprintf(timeOnlyStr, "%02d:%02d:%02d",
          now.hour(), now.minute(), now.second());
  
  // Create payload using String concatenation instead of JSON library
  String payload = "{\"fingerprintId\":\"" + String(id) +
                  "\",\"date\":\"" + String(timeStr) +
                  "\",\"time\":\"" + String(timeOnlyStr) +
                  "\",\"status\":\"" + String(status) +
                  "\",\"topic\":\"" + MQTT_TOPIC + "\"}";
  
  Serial.print("Sending payload: ");
  Serial.println(payload);
  
  if (client.publish(MQTT_TOPIC, payload.c_str())) {
    Serial.println("Published attendance data successfully");
  } else {
    Serial.println("Failed to publish attendance data");
  }
}

void displayDateTime() {
  DateTime now = rtc.now();
  
  lcd.clear();
  lcd.setCursor(0, 0);
  
  // Format: DD/MM/YYYY
  lcd.print(now.day() < 10 ? "0" : "");
  lcd.print(now.day());
  lcd.print("/");
  lcd.print(now.month() < 10 ? "0" : "");
  lcd.print(now.month());
  lcd.print("/");
  lcd.print(now.year());
  
  lcd.setCursor(0, 1);
  
  // Format: HH:MM:SS
  lcd.print(now.hour() < 10 ? "0" : "");
  lcd.print(now.hour());
  lcd.print(":");
  lcd.print(now.minute() < 10 ? "0" : "");
  lcd.print(now.minute());
  lcd.print(":");
  lcd.print(now.second() < 10 ? "0" : "");
  lcd.print(now.second());
}

void displayWelcomeScreen() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Attendance System");
  lcd.setCursor(0, 1);
  lcd.print("Ready...");
}

String getFingerprintString(bool tryAutoEnroll) {
  int attempts = 0;
  const int MAX_ATTEMPTS = 3;
  
  while (attempts < MAX_ATTEMPTS) {
    uint8_t p = finger.getImage();
    
    if (p != FINGERPRINT_OK) {
      if (p == FINGERPRINT_NOFINGER) {
        if (tryAutoEnroll) {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Place finger");
          lcd.setCursor(0, 1);
          lcd.print("firmly on sensor");
        
          unsigned long startTime = millis();
          while (millis() - startTime < 10000) {
            p = finger.getImage();
            if (p == FINGERPRINT_OK) break;
            
            if ((millis() - startTime) % 1000 < 100) {
              lcd.setCursor(15, 1);
              static int dots = 0;
              lcd.print(dots == 0 ? " " : dots == 1 ? "." : dots == 2 ? ".." : "...");
              dots = (dots + 1) % 4;
            }
            
            delay(100);
          }
          
          if (p != FINGERPRINT_OK) {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Timeout!");
            lcd.setCursor(0, 1);
            lcd.print("No finger detected");
            delay(2000);
            return "";
          }
        } else {
          return "";
        }
      } else {
        Serial.println("Error getting fingerprint image");
        attempts++;
        if (attempts >= MAX_ATTEMPTS) {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Too many errors");
          lcd.setCursor(0, 1);
          lcd.print("Please try again");
          delay(2000);
          return "";
        }
        continue; 
      }
    }
    
    p = finger.image2Tz();
    if (p != FINGERPRINT_OK) {
      Serial.print("Error converting fingerprint image: ");
      Serial.println(p);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Poor image");
      lcd.setCursor(0, 1);
      lcd.print("Place firmly");
      delay(1500);
      
      attempts++;
      if (attempts >= MAX_ATTEMPTS) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Too many errors");
        lcd.setCursor(0, 1);
        lcd.print("Please try again");
        delay(2000);
        return "";
      }
      continue; 
    }
    
    p = finger.fingerSearch();
    if (p == FINGERPRINT_OK) {
      if (finger.confidence < 50) {
        Serial.print("Match found but confidence too low: ");
        Serial.println(finger.confidence);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Low confidence");
        lcd.setCursor(0, 1);
        lcd.print("Try again");
        delay(1500);
        
        attempts++;
        if (attempts >= MAX_ATTEMPTS) {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Too many errors");
          lcd.setCursor(0, 1);
          lcd.print("Please try again");
          delay(2000);
          return "";
        }
        continue; 
      }
      String fpId = "FPID" + String(finger.fingerID);
      Serial.print("Found fingerprint with ID: ");
      Serial.print(fpId);
      Serial.print(" (confidence: ");
      Serial.print(finger.confidence);
      Serial.println(")");
      return fpId;
    } else if (p == FINGERPRINT_NOTFOUND) {
      Serial.println("Fingerprint not found in database");
    
      if (tryAutoEnroll) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("New fingerprint");
        lcd.setCursor(0, 1);
        lcd.print("Enrolling...");
        delay(1000);
        
        uint8_t nextId = 1;
        while (finger.loadModel(nextId) == FINGERPRINT_OK) {
          nextId++;
          if (nextId >= 128) { 
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Database full!");
            delay(2000);
            return "";
          }
        }
        
        uint8_t result = autoEnrollFingerprint(nextId);
        if (result == FINGERPRINT_OK) {
          String newId = "FPID" + String(nextId);
          return newId;
        } else {
          return ""; 
        }
      }
      
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Fingerprint not");
      lcd.setCursor(0, 1);
      lcd.print("recognized");
      delay(2000);
      return "";
    } else {
      Serial.print("Error searching for fingerprint: ");
      Serial.println(p);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Sensor error");
      lcd.setCursor(0, 1);
      lcd.print("Try again");
      delay(2000);
      
      attempts++;
      if (attempts >= MAX_ATTEMPTS) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Too many errors");
        lcd.setCursor(0, 1);
        lcd.print("Please try again");
        delay(2000);
        return "";
      }
      continue; 
    }
  }
  
  return "";
}

uint8_t deleteFingerprint(uint8_t id) {
  uint8_t p = -1;
  
  p = finger.deleteModel(id);
  
  if (p == FINGERPRINT_OK) {
    Serial.print("Deleted fingerprint ID #");
    Serial.println(id);
    return p;
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not delete from that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return p;
  } else {
    Serial.print("Unknown error: 0x"); Serial.println(p, HEX);
    return p;
  }
}


uint8_t emptyDatabase() {
  uint8_t p = finger.emptyDatabase();
  
  if (p == FINGERPRINT_OK) {
    Serial.println("Database emptied!");
    return p;
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_DBCLEARFAIL) {
    Serial.println("Could not clear database");
    return p;
  } else {
    Serial.print("Unknown error: 0x"); Serial.println(p, HEX);
    return p;
  }
}
