// ================================================
// IoT Household Inventory Management System
// Final Year Project - Babalola Emmanuel Timilehin
// ESP32-C6 - LCD + WiFi + Buzzer + Email Alert
// ================================================

#include <Wire.h>
#include <MFRC522.h>
#include <SPI.h>
#include <WiFi.h>
#include <ThingSpeak.h>
#include <HTTPClient.h>
#include <time.h>

// ===================== PIN DEFINITIONS =====================
#define RST_PIN      5
#define SS_PIN       4
#define BUZZER_PIN   15

// LCD I2C
#define LCD_ADDR     0x27
#define LCD_COLS     16
#define LCD_ROWS     4

// Confirmed row addresses for 16x4 LCD
const byte rowBase[4] = {0x00, 0x40, 0x10, 0x50};

// ===================== WIFI + THINGSPEAK =====================
const char* ssid          = "DESKTOP-2NUFHOG 9601";
const char* password      = "9848Q4:p";
unsigned long channelID   = 3399453;
const char* writeAPIKey   = "E2I66NZ3FJNJNJCY";

// ===================== GOOGLE APPS SCRIPT EMAIL =====================
const char* googleScriptURL = "https://script.google.com/macros/s/AKfycbwIQcSUd03cfSxGreu7UWh6P7DltgH-5Zk-UBoFlRfREa3SCsaYpgr_2Zvt_BW2SJU0/exec";  
// ← REPLACE WITH YOUR DEPLOYED WEB APP URL

WiFiClient client;

// ===================== OBJECTS =====================
MFRC522 rfid(SS_PIN, RST_PIN);

// ===================== ITEM DATABASE =====================
struct Item {
  String name;
  String prefix;
  float currentWeight;
  float lastRestockWeight;
  float minWeight;
  String rfidTag;
  int weightField;
  int stockField;
};

Item items[4] = {
  {"Rice",      "[R]", 0.0, 5000.0, 100.0, "73:92:83:FE", 1, 2},
  {"Beans",     "[B]", 0.0, 1200.0, 50.0,  "45:63:9F:28", 3, 4},
  {"Garri",     "[G]", 0.0, 3000.0, 80.0,  "2A:B1:FD:3E", 5, 6},
  {"Detergent", "[D]", 0.0, 250.0,  10.0,  "AA:BB:CC:DD", 7, 8},
};

// ===================== GLOBAL VARIABLES =====================
String currentItemName   = "No Item";
String lastScannedItem   = "None";
float currentWeight      = 0.0;
int stockPercentage      = 0;
bool itemSelected        = false;
int currentItemIndex     = -1;

unsigned long lastScanTime     = 0;
unsigned long lastActivityTime = 0;
unsigned long lastUploadTime   = 0;
bool lcdBacklightOn            = true;

const unsigned long INACTIVITY_TIMEOUT = 300000;   // 30 seconds

// ===================== FORWARD DECLARATIONS =====================
void lcdInit();
void lcdClear();
void lcdSetCursor(uint8_t col, uint8_t row);
void lcdPrint(const char* text);
void lcdPrint(String text);
void lcdPrint(int val);
void lcdPrint(float val, int decimals = 1);
void lcdWrite(uint8_t value);
void lcdSend(uint8_t value, uint8_t mode);
void lcdPulseEnable(uint8_t val);
void lcdI2CWrite(uint8_t val);
void setBacklight(bool on);

void loadingDot(int row, int col);
void showHomeScreen();
void showScanningScreen();
void showItemScreen();
void processRFID();
void connectWiFi();
void sendToThingSpeak();
void sendDailyUpload();
void checkLowStockAndNotify();
void beep(int times = 1);

// ===================== SETUP =====================
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\n=== SYSTEM BOOT ===");

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  Wire.setPins(21, 22);
  Wire.begin();
  Wire.setClock(100000);

  lcdInit();
  setBacklight(true);

  SPI.begin(18, 19, 23, SS_PIN);
  rfid.PCD_Init();

  connectWiFi();
  ThingSpeak.begin(client);

  configTime(3600, 0, "pool.ntp.org", "time.nist.gov");  // WAT timezone

  showHomeScreen();
  lastActivityTime = millis();
  Serial.println("=== SYSTEM READY ===");
  beep(2);
}

// ===================== LCD FUNCTIONS =====================
void lcdI2CWrite(uint8_t val) {
  Wire.beginTransmission(LCD_ADDR);
  Wire.write(val | (lcdBacklightOn ? 0x08 : 0));
  Wire.endTransmission();
  delayMicroseconds(50);
}

void lcdPulseEnable(uint8_t val) {
  lcdI2CWrite(val | 0x04);
  delayMicroseconds(150);
  lcdI2CWrite(val & ~0x04);
  delayMicroseconds(150);
}

void lcdSend(uint8_t value, uint8_t mode) {
  uint8_t high = value & 0xF0;
  uint8_t low  = (value << 4) & 0xF0;
  lcdI2CWrite(high | mode);
  lcdPulseEnable(high | mode);
  lcdI2CWrite(low | mode);
  lcdPulseEnable(low | mode);
}

void lcdCommand(uint8_t value) { lcdSend(value, 0); }
void lcdWrite(uint8_t value)   { lcdSend(value, 0x01); }

void lcdInit() {
  delay(50);
  lcdI2CWrite(0);
  delay(50);

  lcdI2CWrite(0x30); lcdPulseEnable(0x30); delay(5);
  lcdI2CWrite(0x30); lcdPulseEnable(0x30); delay(5);
  lcdI2CWrite(0x30); lcdPulseEnable(0x30); delay(2);
  lcdI2CWrite(0x20); lcdPulseEnable(0x20); delay(2);

  lcdCommand(0x28); // 4-bit, 2 lines
  lcdCommand(0x0C); // Display on, cursor off
  lcdCommand(0x06); // Entry mode
  lcdClear();
}

void lcdClear() {
  lcdCommand(0x01);
  delay(3);
}

void lcdSetCursor(uint8_t col, uint8_t row) {
  if (row > 3) row = 3;
  if (col > 15) col = 15;
  lcdCommand(0x80 | (col + rowBase[row]));
}

void lcdPrint(const char* text) {
  while (*text) lcdWrite(*text++);
}

void lcdPrint(String text) {
  for (char c : text) lcdWrite(c);
}

void lcdPrint(int val) {
  char buf[12];
  itoa(val, buf, 10);
  lcdPrint(buf);
}

void lcdPrint(float val, int decimals) {
  char buf[16];
  dtostrf(val, 6, decimals, buf);
  lcdPrint(buf);
}

void setBacklight(bool on) {
  lcdBacklightOn = on;
  lcdI2CWrite(0);
}

// ===================== LOADING ANIMATION =====================
void loadingDot(int row, int col) {
  lcdSetCursor(col, row);
  for (int i = 0; i < 3; i++) {
    lcdPrint(".");
    delay(800);
  }
  lcdSetCursor(col, row);
  lcdPrint("   ");
}

// ===================== BEEP =====================
void beep(int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(80);
    digitalWrite(BUZZER_PIN, LOW);
    delay(120);
  }
}

// ===================== SCREENS =====================
void showHomeScreen() {
  lcdClear();
  lcdSetCursor(3, 0); lcdPrint("Home Page");
  lcdSetCursor(0, 1); lcdPrint("Last: "); lcdPrint(lastScannedItem);
  lcdSetCursor(0, 3); lcdPrint("Scan RFID Tag");
}

void showScanningScreen() {
  lcdClear();
  lcdSetCursor(3, 0); lcdPrint("Scanning");
  loadingDot(0, 11);
}

void showItemScreen() {
  lcdClear();
  lcdSetCursor(0, 0);
  lcdPrint(items[currentItemIndex].prefix);
  lcdPrint(" ");
  lcdPrint(currentItemName);

  lcdSetCursor(0, 1);
  lcdPrint("Wt: ");
  lcdPrint(currentWeight, 1);
  lcdPrint("g  Stk: ");
  lcdPrint(stockPercentage);
  lcdPrint("%");

  lcdSetCursor(0, 2);
  if (stockPercentage < 20) {
    lcdPrint("!! LOW - RESTOCK!");
  } else if (stockPercentage < 50) {
    lcdPrint("Moderate Stock");
  } else {
    lcdPrint("Stock OK");
  }
}

// ===================== RFID =====================
void processRFID() {
  String tag = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (i > 0) tag += ":";
    if (rfid.uid.uidByte[i] < 0x10) tag += "0";
    tag += String(rfid.uid.uidByte[i], HEX);
  }
  tag.toUpperCase();

  showScanningScreen();
  beep(1);

  bool found = false;
  for (int i = 0; i < 4; i++) {
    if (tag == items[i].rfidTag) {
      currentItemIndex = i;
      currentItemName  = items[i].name;
      lastScannedItem  = items[i].name;
      itemSelected     = true;
      lastScanTime     = millis();
      lastActivityTime = millis();

      currentWeight = items[i].currentWeight;   // Replace with HX711 later
      stockPercentage = (items[i].lastRestockWeight > 0) ?
                        (int)(currentWeight / items[i].lastRestockWeight * 100) : 0;
      stockPercentage = constrain(stockPercentage, 0, 100);

      showItemScreen();
      sendToThingSpeak();
      checkLowStockAndNotify();
      found = true;
      break;
    }
  }

  if (!found) {
    lcdClear();
    lcdSetCursor(0, 1); lcdPrint("Unknown Tag!");
    delay(1500);
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

// ===================== THINGSPEAK =====================
void sendToThingSpeak() {
  if (WiFi.status() != WL_CONNECTED) return;
  
  ThingSpeak.setField(items[currentItemIndex].weightField, currentWeight);
  ThingSpeak.setField(items[currentItemIndex].stockField, stockPercentage);
  ThingSpeak.writeFields(channelID, writeAPIKey);
}

void sendDailyUpload() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return;

  if (timeinfo.tm_hour == 23 && timeinfo.tm_min == 0) {
    if (millis() - lastUploadTime > 60000) {
      Serial.println("=== Daily 11PM Upload ===");
      lcdClear();
      lcdSetCursor(1, 1); lcdPrint("Daily Upload...");

      for (int i = 0; i < 4; i++) {
        float pct = (items[i].lastRestockWeight > 0) ?
                    (items[i].currentWeight / items[i].lastRestockWeight * 100.0) : 0;
        pct = constrain(pct, 0, 100);

        ThingSpeak.setField(items[i].weightField, items[i].currentWeight);
        ThingSpeak.setField(items[i].stockField, (int)pct);
        ThingSpeak.writeFields(channelID, writeAPIKey);
        delay(17000);
      }
      lastUploadTime = millis();
      showHomeScreen();
    }
  }
}

// ===================== LOW STOCK EMAIL =====================
void checkLowStockAndNotify() {
  if (stockPercentage < 20 && currentItemIndex != -1) {
    Serial.println("🚨 LOW STOCK ALERT! Sending email...");

    beep(3);

    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin(googleScriptURL);
      http.addHeader("Content-Type", "application/json");

      String payload = "{\"item\":\"" + currentItemName + 
                      "\",\"stock\":" + String(stockPercentage) + 
                      ",\"weight\":" + String(currentWeight, 1) + "}";

      int httpResponseCode = http.POST(payload);

      if (httpResponseCode > 0) {
        Serial.println("✅ Email Alert Sent Successfully!");
        lcdSetCursor(0, 3);
        lcdPrint("Alert Sent!");
      } else {
        Serial.print("❌ Error: ");
        Serial.println(httpResponseCode);
        lcdSetCursor(0, 3);
        lcdPrint("Alert Failed");
      }
      http.end();
    } else {
      Serial.println("WiFi disconnected");
    }
    delay(2500);
  }
}

// ===================== WIFI =====================
void connectWiFi() {
  lcdClear();
  lcdSetCursor(0, 0); lcdPrint("Connecting WiFi...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    attempts++;
    if (attempts % 5 == 0) {
      lcdSetCursor(0, 1); lcdPrint("Attempt: "); lcdPrint(attempts);
    }
  }

  if (WiFi.status() == WL_CONNECTED) {
    lcdClear();
    lcdSetCursor(0, 0); lcdPrint("WiFi Connected!");
    lcdSetCursor(0, 1); lcdPrint(WiFi.localIP().toString());
    delay(2000);
  } else {
    lcdClear();
    lcdSetCursor(0, 0); lcdPrint("WiFi Failed!");
    lcdSetCursor(0, 1); lcdPrint("Offline Mode");
    delay(2000);
  }
}

// ===================== LOOP =====================
void loop() {
  // Backlight timeout
  if (lcdBacklightOn && millis() - lastActivityTime > INACTIVITY_TIMEOUT) {
    setBacklight(false);
  }

  // RFID Scan
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    setBacklight(true);
    lastActivityTime = millis();
    processRFID();
  }

  // Daily upload check
  sendDailyUpload();

  // Return to home screen
  if (itemSelected && millis() - lastScanTime > 15000) {
    itemSelected = false;
    currentItemIndex = -1;
    showHomeScreen();
  }

  delay(50);
}
