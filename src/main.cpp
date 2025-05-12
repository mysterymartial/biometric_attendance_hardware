#include "hardware_init.h"

#include "hardware_init.h"

const int EEPROM_SIZE = 4;
bool pinSet = false;
char input[4] = {'\0'};
int inputIndex = 0;
unsigned long idDisplayStartTime = 0;
const unsigned long idDisplayDuration = 120000; // 2 minutes in milliseconds
bool idDisplayActive = false;
String currentFingerprintId = "";
unsigned long lastFingerprintReadTime = 0;
const unsigned long fingerprintCooldown = 5000; // 5 seconds cooldown between reads


void handleMenu();
char displayMainMenu();
void waitForBackKey();
bool isTimeAllowedForAttendance();

#ifndef UNIT_TEST
// Exclude during unit tests
void setup() {
  initHardware();
  
  for (int i = 0; i < 4; i++) {
    pin[i] = EEPROM.read(i);
    decryptPin(&pin[i]);
    if (pin[i] < '0' || pin[i] > '9') pin[i] = '0';
  }
  
  if (pin[0] != '0' || pin[1] != '0' || pin[2] != '0' || pin[3] != '0') {
    pinSet = true;
  }
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Welcome to");
  lcd.setCursor(0, 1);
  lcd.print("Semicolon!");
  delay(1000);
}

void loop() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Press any key...");
  
  char key = '\0';
  while (key == '\0') {
    key = getKey();
    delay(50);
  }
  
  handleMenu();
}
#endif  // UNIT_TEST

char displayMainMenu() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("1:Reg 2:Att 3:Res");
  lcd.setCursor(0, 1);
  lcd.print("7:Home");
  char choice = '\0';
  unsigned long menuStartTime = millis();
  
  while (choice == '\0') {
    choice = getKey();
    
    // If we got a key, validate it
    if (choice != '\0') {
      Serial.print("Menu key pressed: '");
      Serial.print(choice);
      Serial.println("'");
      
      // Check if it's one of our valid options using direct character comparison
      if (choice == '1') {
        Serial.println("Valid choice: 1 (Registration)");
      } else if (choice == '2') {
        Serial.println("Valid choice: 2 (Attendance)");
      } else if (choice == '3') {
        Serial.println("Valid choice: 3 (Reset)");
      } else if (choice == '7') {
        Serial.println("Valid choice: 7 (Home)");
      } else {
        // Invalid choice
        Serial.print("Invalid menu choice: '");
        Serial.print(choice);
        Serial.println("'");
        
        lcd.setCursor(0, 1);
        lcd.print("Invalid! Try again");
        delay(1000);
        lcd.setCursor(0, 1);
        lcd.print("7:Home            ");
        choice = '\0'; // Reset to continue loop
      }
    }
    
    // Timeout after 30 seconds of inactivity
    if (millis() - menuStartTime > 30000) {
      Serial.println("Menu timeout - returning to main screen");
      return '7'; // Return home on timeout
    }
    
    delay(50); // Small delay to prevent CPU hogging
  }
  
  // Debug output
  Serial.print("Final menu choice selected: '");
  Serial.print(choice);
  Serial.println("'");
  
  return choice;
}

void waitForBackKey() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Press 7 to go back");
  // Changed from 4 to 7
  char backKey = '\0';
  unsigned long startTime = millis();
  
  while (backKey != '7') {  // Changed from 4 to 7
    backKey = getKey();
    
    // If a key was pressed but it's not '7', show message
    if (backKey != '\0') {
      Serial.print("Back key pressed: ");
      Serial.println(backKey);
      
      if (backKey != '7') {  // Changed from 4 to 7
        lcd.setCursor(0, 1);
        lcd.print("Invalid! Press 7");  // Changed from 4 to 7
        delay(1000);
        lcd.setCursor(0, 1);
        lcd.print("                ");
      }
    }
    
    // Timeout after 30 seconds
    if (millis() - startTime > 30000) {
      Serial.println("Back key timeout - returning automatically");
      break;
    }
    
    delay(50); // Small delay to prevent CPU hogging
  }
}

bool isTimeAllowedForAttendance() {
  DateTime now = rtc.now();
  
  // Debug output
  Serial.println("Checking if attendance is allowed at current time:");
  Serial.print("Current date/time: ");
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
  Serial.println(now.second());
  
  return isTimeAllowedForAttendanceWithTime(now);
}

void handleMenu() {
  char choice = displayMainMenu();
  String fpId = "";
  
  // Debug the initial choice
  Serial.print("Initial menu choice: '");
  Serial.print(choice);
  Serial.println("'");
  
  while (choice != '7') {
    // More debug output at the start of each loop
    Serial.print("Handling menu choice: '");
    Serial.print(choice);
    Serial.println("'");
    
    // Use if-else instead of switch for more reliable character comparison
    if (choice == '1') {
      Serial.println("Selected Registration (Option 1)");
      
      // Registration - doesn't require connectivity
      if (!pinSet) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Set New PIN: ");
        char newPin[4] = {'\0'};
        int newIndex = 0;
        
        while (newIndex < 4) {
          char key = getKey();
          if (key != '\0') {
            // Only accept digits
            if (isdigit(key)) {
              newPin[newIndex++] = key;
              lcd.setCursor(12 + newIndex - 1, 0);
              lcd.print("*");
            } else {
              // Show error for non-digit keys
              lcd.setCursor(0, 1);
              lcd.print("Use digits only!");
              delay(1000);
              lcd.setCursor(0, 1);
              lcd.print("                ");
            }
            delay(200); // Add delay to prevent double reads
          }
        }
        
        bool validInput = true;
        for (int i = 0; i < 4; i++) {
          if (newPin[i] < '0' || newPin[i] > '9') {
            validInput = false;
            break;
          }
        }
        
        if (validInput) {
          memcpy(pin, newPin, 4);
          savePinToEEPROM();
          pinSet = true;
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("PIN Set!");
          delay(1000);
        } else {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Wrong Format!");
          lcd.setCursor(0, 1);
          lcd.print("Use digits 0-9");
          delay(2000);
          waitForBackKey();
          choice = displayMainMenu();
          continue;
        }
      }
      
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Place Finger");
      
      // Enable auto-enrollment for registration
      fpId = getFingerprintString(true);
      
      if (fpId.length() > 0) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Finger ID: ");
        lcd.print(fpId.substring(4)); // Show just the ID number
        
        // Check if this was a new enrollment
        if (fpId.startsWith("FPID") && fpId.substring(4).toInt() > 0) {
          lcd.setCursor(0, 1);
          lcd.print("7:Exit/New Finger");
        }
        
        // Start the timer for 2-minute display
        unsigned long startTime = millis();
        bool exitEarly = false;
        
        // Loop for 2 minutes or until key 7 is pressed or new finger detected
        while (millis() - startTime < 120000 && !exitEarly) {
          // Check for key press
          char key = getKey();
          if (key == '7') {
            exitEarly = true;
          }
          
          // Check if a finger is on the sensor - using the function instead of direct access
          uint8_t p = getFingerprintID();
          if (p == FINGERPRINT_OK) {
            exitEarly = true;
          }
          
          delay(50); // Small delay to prevent CPU hogging
        }
      } else {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("No Fingerprint");
        lcd.setCursor(0, 1);
        lcd.print("Detected!");
        delay(2000);
        waitForBackKey();
      }
    }
    else if (choice == '2') {
      Serial.println("Selected Attendance (Option 2)");
      
      // Attendance - requires connectivity and time check
      if (!isTimeAllowedForAttendance()) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Can't mark attendance");
        lcd.setCursor(0, 1);
        lcd.print("by this time");
        delay(2000);
        waitForBackKey();
        choice = displayMainMenu();
        continue;
      }
      
      // Try to reconnect if needed for attendance marking
      if (!mqttConnected) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Connecting for");
        lcd.setCursor(0, 1);
        lcd.print("attendance...");
        delay(1000);
        
        reconnectMQTTIfNeeded();
        
        if (!mqttConnected) {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("No connection!");
          lcd.setCursor(0, 1);
          lcd.print("Try again later");
          delay(2000);
          waitForBackKey();
          choice = displayMainMenu();
          continue;
        }
      }
      
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Place Finger");
      lcd.setCursor(0, 1);
      lcd.print("for Attendance");
      
      // Don't auto-enroll for attendance
      fpId = getFingerprintString(false);
      
      if (fpId.length() > 0) {
        // Use the publishAttendanceData function from hardware_init.cpp
        publishAttendanceData(fpId.c_str(), "present");
        
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("ID: " + fpId.substring(4)); // Show just the ID number
        lcd.setCursor(0, 1);
        lcd.print("7:Exit/New Finger");
        
        // Start the timer for 2-minute display
        unsigned long startTime = millis();
        bool exitEarly = false;
        
        // Loop for 2 minutes or until key 7 is pressed or new finger detected
        while (millis() - startTime < 120000 && !exitEarly) {
          // Check for key press
          char key = getKey();
          if (key == '7') {
            exitEarly = true;
          }
          
          // Check if a finger is on the sensor - using the function instead of direct access
          uint8_t p = getFingerprintID();
          if (p == FINGERPRINT_OK) {
            exitEarly = true;
          }
          
          delay(50); // Small delay to prevent CPU hogging
        }
      } else {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("No Fingerprint");
        lcd.setCursor(0, 1);
        lcd.print("Detected!");
        delay(2000);
        waitForBackKey();
      }
    }
    else if (choice == '3') {
      Serial.println("Selected Leave (Option 3)");
      
      // Leave - requires connectivity
      if (!mqttConnected) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Connecting for");
        lcd.setCursor(0, 1);
        lcd.print("leave...");
        delay(1000);
        
        reconnectMQTTIfNeeded();
        
        if (!mqttConnected) {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("No connection!");
          lcd.setCursor(0, 1);
          lcd.print("Try again later");
          delay(2000);
          waitForBackKey();
          choice = displayMainMenu();
          continue;
        }
      }
      
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Place Finger");
      lcd.setCursor(0, 1);
      lcd.print("for Leave");
      
      // Don't auto-enroll for leave
      fpId = getFingerprintString(false);
      
      if (fpId.length() > 0) {
        // Use the publishAttendanceData function with "leave" status
        publishAttendanceData(fpId.c_str(), "leave");
        
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("ID: " + fpId.substring(4)); // Show just the ID number
        lcd.setCursor(0, 1);
        lcd.print("7:Exit/New Finger");
        
        // Start the timer for 2-minute display
        unsigned long startTime = millis();
        bool exitEarly = false;
        
        // Loop for 2 minutes or until key 7 is pressed or new finger detected
        while (millis() - startTime < 120000 && !exitEarly) {
          // Check for key press
          char key = getKey();
          if (key == '7') {
            exitEarly = true;
          }
          
          // Check if a finger is on the sensor - using the function instead of direct access
          uint8_t p = getFingerprintID();
          if (p == FINGERPRINT_OK) {
            exitEarly = true;
          }
          
          delay(50); // Small delay to prevent CPU hogging
        }
      } else {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("No Fingerprint");
        lcd.setCursor(0, 1);
        lcd.print("Detected!");
        delay(2000);
        waitForBackKey();
      }
    }
    else if (choice != '7') {
      // Invalid choice (not 1, 2, 3, or 7)
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Invalid choice!");
      lcd.setCursor(0, 1);
      lcd.print("Use 1, 2, 3 or 7");
      delay(2000);
    }
    
    // Get the next menu choice
    choice = displayMainMenu();
    
    // Debug the new choice
    Serial.print("New menu choice: '");
    Serial.print(choice);
    Serial.println("'");
  }
  
  Serial.println("Exiting menu (option 7 selected)");
}


// #include <Wire.h>
// #include <LiquidCrystal_I2C.h>
// #include <Arduino.h>

// // Pin definitions
// const int rowPins[4] = {13, 12, 14, 27}; 
// const int colPins[4] = {26, 25, 33, 32}; 

// // Key mapping
// char keys[4][4] = {
//   {'1', '2', '3', 'A'},
//   {'4', '5', '6', 'B'},
//   {'7', '8', '9', 'C'},
//   {'*', '0', '#', 'D'}
// };

// // I2C pins
// #define SDA_PIN 21
// #define SCL_PIN 23

// // Initialize LCD (16x2, address 0x27)
// LiquidCrystal_I2C lcd(0x27, 16, 2);

// // For debouncing
// unsigned long lastDebounceTime = 0;
// const unsigned long debounceDelay = 200; // Increased debounce delay

// void setup() {
//   // Initialize Serial for debugging
//   Serial.begin(9600);
  
//   // Initialize I2C and LCD
//   Wire.begin(SDA_PIN, SCL_PIN);
//   Wire.setClock(100000);
//   lcd.init();
//   lcd.backlight();
//   lcd.setCursor(0, 0);
//   lcd.print("Keypad Test");
//   lcd.setCursor(0, 1);
//   lcd.print("Press keys...");
//   delay(2000);
//   lcd.clear();
  
//   // Initialize keypad pins with proper pull-up
//   for (int i = 0; i < 4; i++) {
//     pinMode(rowPins[i], INPUT_PULLUP);
//     pinMode(colPins[i], OUTPUT);
//     digitalWrite(colPins[i], HIGH);
//   }
  
//   // Print debug info
//   Serial.println("Keypad Test Started");
//   Serial.println("-------------------");
// }

// char getKey() {
//   // Check if enough time has passed since last key press
//   if ((millis() - lastDebounceTime) < debounceDelay) {
//     return '\0'; // Return no key if debounce period hasn't passed
//   }
  
//   // Scan the keypad with improved method
//   for (int col = 0; col < 4; col++) {
//     // Set current column to LOW
//     digitalWrite(colPins[col], LOW);
    
//     // Small delay to stabilize
//     delayMicroseconds(500);
    
//     // Check each row
//     for (int row = 0; row < 4; row++) {
//       if (digitalRead(rowPins[row]) == LOW) {
//         // Key is pressed, set column back to HIGH
//         digitalWrite(colPins[col], HIGH);
        
//         // Update debounce time
//         lastDebounceTime = millis();
        
//         // Wait for key release with timeout
//         unsigned long pressStart = millis();
//         while (digitalRead(rowPins[row]) == LOW) {
//           // Break if key is held too long (prevent hanging)
//           if (millis() - pressStart > 1000) break;
//           delay(10);
//         }
        
//         // Return the key
//         return keys[row][col];
//       }
//     }
    
//     // Set column back to HIGH before moving to next column
//     digitalWrite(colPins[col], HIGH);
//   }
  
//   // No key was pressed
//   return '\0';
// }

// void loop() {
//   char key = getKey();
  
//   if (key != '\0') {
//     // Display on LCD
//     lcd.clear();
//     lcd.setCursor(0, 0);
//     lcd.print("Key: ");
//     lcd.print(key);
    
//     // Display on Serial - just the key value
//     Serial.println(key);
    
//     // Debug info to check if key is a digit
//     lcd.setCursor(0, 1);
//     if (isdigit(key)) {
//       lcd.print("Is a digit: Yes");
//     } else {
//       lcd.print("Is a digit: No");
//     }
//   }
// }

// #include <Wire.h>
// #include <LiquidCrystal_I2C.h>
// #include <Arduino.h>

// // Pin definitions
// const int rowPins[4] = {13, 12, 14, 27}; 
// const int colPins[4] = {26, 25, 33, 32}; 

// // Key mapping
// char keys[4][4] = {
//   {'1', '2', '3', 'A'},
//   {'4', '5', '6', 'B'},
//   {'7', '8', '9', 'C'},
//   {'*', '0', '#', 'D'}
// };

// // I2C pins
// #define SDA_PIN 21
// #define SCL_PIN 23

// // Initialize LCD (16x2, address 0x27)
// LiquidCrystal_I2C lcd(0x27, 16, 2);

// // For debouncing
// unsigned long lastDebounceTime = 0;
// const unsigned long debounceDelay = 200; // Increased debounce delay

// void setup() {
//   // Initialize Serial for debugging
//   Serial.begin(9600);
  
//   // Initialize I2C and LCD
//   Wire.begin(SDA_PIN, SCL_PIN);
//   Wire.setClock(100000);
//   lcd.init();
//   lcd.backlight();
//   lcd.setCursor(0, 0);
//   lcd.print("Keypad Test");
//   lcd.setCursor(0, 1);
//   lcd.print("Press keys...");
  
//   // Initialize keypad pins with proper pull-up
//   for (int i = 0; i < 4; i++) {
//     pinMode(rowPins[i], INPUT_PULLUP);
//     pinMode(colPins[i], OUTPUT);
//     digitalWrite(colPins[i], HIGH);
//   }
  
//   // Print debug info
//   Serial.println("\n\nKeypad Test Started");
//   Serial.println("-------------------");
//   Serial.println("This test will show ASCII values and character comparisons");
//   delay(2000);
//   lcd.clear();
// }

// char getKey() {
//   // Check if enough time has passed since last key press
//   if ((millis() - lastDebounceTime) < debounceDelay) {
//     return '\0'; // Return no key if debounce period hasn't passed
//   }
  
//   // Scan the keypad with improved method
//   for (int col = 0; col < 4; col++) {
//     // Set current column to LOW
//     digitalWrite(colPins[col], LOW);
    
//     // Small delay to stabilize
//     delayMicroseconds(500);
    
//     // Check each row
//     for (int row = 0; row < 4; row++) {
//       if (digitalRead(rowPins[row]) == LOW) {
//         // Key is pressed, set column back to HIGH
//         digitalWrite(colPins[col], HIGH);
        
//         // Update debounce time
//         lastDebounceTime = millis();
        
//         // Wait for key release with timeout
//         unsigned long pressStart = millis();
//         while (digitalRead(rowPins[row]) == LOW) {
//           // Break if key is held too long (prevent hanging)
//           if (millis() - pressStart > 1000) break;
//           delay(10);
//         }
        
//         // Return the key
//         return keys[row][col];
//       }
//     }
    
//     // Set column back to HIGH before moving to next column
//     digitalWrite(colPins[col], HIGH);
//   }
  
//   // No key was pressed
//   return '\0';
// }

// void loop() {
//   char key = getKey();
  
//   if (key != '\0') {
//     // Display on LCD
//     lcd.clear();
//     lcd.setCursor(0, 0);
//     lcd.print("Key: ");
//     lcd.print(key);
    
//     // Display detailed info on Serial
//     Serial.println("\n--- Key Press Details ---");
//     Serial.print("Key pressed: '");
//     Serial.print(key);
//     Serial.println("'");
    
//     // Show ASCII value
//     Serial.print("ASCII value: ");
//     Serial.println((int)key);
    
//     // Test character comparisons
//     Serial.println("\nCharacter comparison tests:");
    
//     // Test direct comparison
//     Serial.print("key == '1': ");
//     Serial.println(key == '1' ? "TRUE" : "FALSE");
    
//     Serial.print("key == '2': ");
//     Serial.println(key == '2' ? "TRUE" : "FALSE");
    
//     Serial.print("key == '3': ");
//     Serial.println(key == '3' ? "TRUE" : "FALSE");
    
//     // Test isdigit function
//     Serial.print("isdigit(key): ");
//     Serial.println(isdigit(key) ? "TRUE" : "FALSE");
    
//     // Display on LCD
//     lcd.setCursor(0, 1);
//     if (isdigit(key)) {
//       lcd.print("ASCII: ");
//       lcd.print((int)key);
//     } else {
//       lcd.print("Not a digit");
//     }
//   }
// }

