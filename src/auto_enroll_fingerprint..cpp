// #include <Wire.h>
// #include <LiquidCrystal_I2C.h>
// #include <Adafruit_Fingerprint.h>
// #include <HardwareSerial.h>
// #include <Arduino.h>

// #define SDA_PIN 21
// #define SCL_PIN 23
// #define RXD2 18
// #define TXD2 19

// // Initialize LCD (16x2, address 0x27)
// LiquidCrystal_I2C lcd(0x27, 16, 2);

// // Initialize Fingerprint sensor
// HardwareSerial mySerial(1);
// Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

// // Function prototypes
// String getFingerprintString();
// uint8_t autoEnrollFingerprint();

// void setup() {
//   // Start Serial for debugging
//   Serial.begin(9600);
//   while (!Serial);
//   delay(100);
//   Serial.println("\n\nAutomatic Fingerprint Attendance");

//   // Initialize I2C and LCD
//   Wire.begin(SDA_PIN, SCL_PIN);
//   Wire.setClock(100000); // Set I2C clock to 100kHz for stability
//   lcd.init();
//   lcd.backlight();
//   lcd.clear();
//   lcd.setCursor(0, 0);
//   lcd.print("Attendance Sys");

//   // Initialize Fingerprint sensor at 57600 baud
//   Serial.println("Initializing UART1 on pins RXD2: 18, TXD2: 19...");
//   delay(1000); // Increased delay for UART stabilization
//   mySerial.begin(57600, SERIAL_8N1, RXD2, TXD2); // Use confirmed 57600 baud
//   finger.begin(57600); // Explicitly initialize fingerprint sensor with 57600 baud
//   Serial.println("Checking for fingerprint sensor...");
//   if (finger.verifyPassword()) {
//     Serial.println("Found fingerprint sensor at 57600 baud!");
//     lcd.setCursor(0, 1);
//     lcd.print("Sensor OK!");
//   } else {
//     Serial.println("Did not find fingerprint sensor at 57600 baud :(");
//     Serial.println("Possible issues:");
//     Serial.println("- Check D+/D- wiring (sensor TX to RXD2:18, sensor RX to TXD2:19)");
//     Serial.println("- Try swapping D+/D- wires");
//     Serial.println("- Ensure power supply is stable (try VIN pin for 5V)");
//     Serial.println("- Test with alternative UART pins (e.g., UART2 on GPIO 16/17)");
//     Serial.println("- Disconnect other components to reduce interference");
//     lcd.setCursor(0, 1);
//     lcd.print("Sensor Err!");
//     while (1); // Halt if sensor not found
//   }
//   delay(1000);
//   lcd.clear();
// }

// void loop() {
//   // Display prompt on the first line
//   lcd.setCursor(0, 0);
//   lcd.print("Place Finger...");

//   // Capture fingerprint and convert to string
//   String fpId = getFingerprintString();
//   if (fpId.length() > 0) {
//     // Log fingerprint ID on the second line
//     lcd.setCursor(0, 1);
//     lcd.print(fpId);
//     lcd.print("      "); // Clear remaining characters
//     Serial.print("Fingerprint ID: ");
//     Serial.println(fpId);
//   } else {
//     // Check if this is a new finger to enroll
//     uint8_t p = finger.getImage();
//     if (p == FINGERPRINT_OK) {
//       p = finger.image2Tz();
//       if (p == FINGERPRINT_OK) {
//         finger.fingerID = 0;
//         finger.confidence = 0;
//         p = finger.fingerFastSearch();
//         if (p == FINGERPRINT_NOTFOUND) {
//           Serial.println("Unknown fingerprint detected! Enrolling...");
//           uint8_t newId = autoEnrollFingerprint();
//           if (newId > 0) {
//             fpId = "FPID" + String(newId);
//             lcd.setCursor(0, 1);
//             lcd.print(fpId);
//             lcd.print("      ");
//             Serial.print("New Fingerprint ID: ");
//             Serial.println(fpId);
//           } else {
//             lcd.setCursor(0, 1);
//             lcd.print("Enroll Failed");
//             Serial.println("Enrollment failed.");
//           }
//         } else {
//           lcd.setCursor(0, 1);
//           lcd.print("No Finger     ");
//           Serial.println("No fingerprint detected.");
//         }
//       } else {
//         lcd.setCursor(0, 1);
//         lcd.print("No Finger     ");
//         Serial.print("image2Tz failed with code: ");
//         Serial.println(p);
//       }
//     } else {
//       lcd.setCursor(0, 1);
//       lcd.print("No Finger     ");
//       Serial.print("getImage failed with code: ");
//       Serial.println(p);
//     }
//   }

//   delay(2000); // Wait 2 seconds before next update
//   lcd.clear();
// }

// // Function to get fingerprint ID as a string with debug
// String getFingerprintString() {
//   Serial.println("Attempting to get fingerprint image...");
//   uint8_t p = finger.getImage();
//   if (p != FINGERPRINT_OK) {
//     Serial.print("getImage failed with code: ");
//     Serial.println(p);
//     return "";
//   }
//   Serial.println("Image captured successfully.");

//   Serial.println("Converting image to template...");
//   p = finger.image2Tz();
//   if (p != FINGERPRINT_OK) {
//     Serial.print("image2Tz failed with code: ");
//     Serial.println(p);
//     return "";
//   }
//   Serial.println("Template conversion successful.");

//   Serial.println("Searching for fingerprint...");
//   finger.fingerID = 0;
//   finger.confidence = 0;
//   p = finger.fingerFastSearch(); // Using fingerFastSearch as confirmed by you
//   if (p == FINGERPRINT_OK) {
//     Serial.print("Found fingerprint ID: ");
//     Serial.println(finger.fingerID);
//     return "FPID" + String(finger.fingerID);
//   } else {
//     Serial.print("fingerFastSearch failed with code: ");
//     Serial.println(p);
//     return "";
//   }
// }

// // Function to automatically enroll a new fingerprint
// uint8_t autoEnrollFingerprint() {
//   uint8_t p = -1;
//   Serial.println("Waiting for valid finger to enroll...");
//   while (p != FINGERPRINT_OK) {
//     p = finger.getImage();
//     if (p == FINGERPRINT_IMAGEFAIL) {
//       Serial.println("Imaging error");
//       return 0;
//     }
//   }
//   Serial.println("Image taken");

//   p = finger.image2Tz(1);
//   if (p != FINGERPRINT_OK) {
//     Serial.print("image2Tz failed with code: ");
//     Serial.println(p);
//     return 0;
//   }
//   Serial.println("Image converted");

//   Serial.println("Remove finger");
//   delay(2000);
//   while (finger.getImage() == FINGERPRINT_OK);

//   Serial.println("Place same finger again");
//   p = -1;
//   while (p != FINGERPRINT_OK) {
//     p = finger.getImage();
//     if (p == FINGERPRINT_IMAGEFAIL) {
//       Serial.println("Imaging error");
//       return 0;
//     }
//   }
//   Serial.println("Image taken");

//   p = finger.image2Tz(2);
//   if (p != FINGERPRINT_OK) {
//     Serial.print("image2Tz failed with code: ");
//     Serial.println(p);
//     return 0;
//   }
//   Serial.println("Image converted");

//   Serial.println("Creating model...");
//   p = finger.createModel();
//   if (p != FINGERPRINT_OK) {
//     Serial.print("createModel failed with code: ");
//     Serial.println(p);
//     return 0;
//   }
//   Serial.println("Prints matched!");

//   // Find the next available ID
//   uint8_t newId = 1;
//   while (finger.loadModel(newId) == FINGERPRINT_OK) {
//     newId++;
//     if (newId > 127) {
//       Serial.println("No available ID slots!");
//       return 0;
//     }
//   }

//   Serial.print("Storing model as ID #");
//   Serial.println(newId);
//   p = finger.storeModel(newId);
//   if (p == FINGERPRINT_OK) {
//     Serial.println("Stored!");
//     return newId;
//   } else {
//     Serial.print("storeModel failed with code: ");
//     Serial.println(p);
//     return 0;
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

