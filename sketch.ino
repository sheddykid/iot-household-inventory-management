// ================================================
// IoT Household Inventory Management System
// Final Year Project - Babalola Emmanuel Timilehin (RUN/CPE/20/8934)
// Using LCD + HX711 + MFRC522 + WiFi + ThingSpeak on ESP32
// ================================================

#include <HX711.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <MFRC522.h>
#include <SPI.h>
#include <WiFi.h>
#include <ThingSpeak.h>

// ===================== PIN DEFINITIONS =====================
#define LOADCELL_DT  26
#define LOADCELL_SCK 25
#define RST_PIN      5
#define SS_PIN       4

// ===================== WIFI + THINGSPEAK CONFIG =====================
const char* ssid        = "Wokwi-GUEST";
const char* password    = "";
unsigned long channelID = 3399453;
const char* writeAPIKey = "E2I66NZ3FJNJNJCY";

WiFiClient client;

// ===================== OBJECTS =====================
HX711 scale;
LiquidCrystal_I2C lcd(0x27, 20, 4);
MFRC522 rfid(SS_PIN, RST_PIN);

// ===================== ITEM DATABASE =====================
struct Item {
  String name;
  float initialWeight;
  float currentWeight;
  float decreaseRate;   // grams lost per scan (simulates usage)
  String rfidTag;
  int weightField;
  int stockField;
  int scanCount;        // tracks how many times scanned
};

Item items[4] = {
  //  Name         Full(g)  Current(g)  Per scan  Tag            Wf  Sf  Scans
  {"Rice",      5000.0,  4200.0,     250.0,    "01:02:03:04",  1,  2,  0},
  {"Beans",     1200.0,  900.0,      120.0,    "11:22:33:44",  3,  4,  0},
  {"Garri",     3000.0,  2500.0,     200.0,    "55:66:77:88",  5,  6,  0},
  {"Detergent", 250.0,   180.0,      25.0,     "AA:BB:CC:DD",  7,  8,  0},
};

String currentItemName  = "No Item";
String lastScannedItem  = "None";
float currentWeight     = 0.0;
float itemInitialWeight = 0.0;
int stockPercentage     = 0;
bool itemSelected       = false;
int currentItemIndex    = -1;
int currentWeightField  = 0;
int currentStockField   = 0;

unsigned long lastScanTime = 0;
bool lcdNeedsUpdate        = false;   // only redraw LCD when needed

const float CALIBRATION_FACTOR = -20500.0;

// ===================== FORWARD DECLARATIONS =====================
void showWelcomeScreen();
void showItemScreen();
void processRFID();
void connectWiFi();
void sendToThingSpeak();

// ===================== SETUP =====================
void setup() {
  Serial.begin(115200);

  scale.begin(LOADCELL_DT, LOADCELL_SCK);
  scale.set_scale(CALIBRATION_FACTOR);
  scale.tare();

  lcd.init();
  lcd.backlight();

  SPI.begin();
  rfid.PCD_Init();

  connectWiFi();
  ThingSpeak.begin(client);

  showWelcomeScreen();
  Serial.println("System Ready!");
}

// ===================== LOOP =====================
void loop() {
  // Only update LCD when something actually changed
  if (lcdNeedsUpdate) {
    lcdNeedsUpdate = false;
    if (itemSelected) {
      showItemScreen();
    } else {
      showWelcomeScreen();
    }
  }

  // Check for RFID scan
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    processRFID();
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }

  // Timeout after 10 seconds
  if (itemSelected && millis() - lastScanTime > 10000) {
    itemSelected     = false;
    currentItemIndex = -1;
    currentItemName  = "No Item";
    lcdNeedsUpdate   = true;
  }
}

// ===================== WIFI =====================
void connectWiFi() {
  Serial.print("Connecting to WiFi");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi...");

  WiFi.begin(ssid, password);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected!");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Connected!");
    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP());
    delay(2000);
  } else {
    Serial.println("\nWiFi Failed! Running offline.");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Failed!");
    lcd.setCursor(0, 1);
    lcd.print("Running offline");
    delay(2000);
  }
}

// ===================== THINGSPEAK UPLOAD =====================
void sendToThingSpeak() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected, skipping upload.");
    return;
  }

  ThingSpeak.setField(currentWeightField, currentWeight);
  ThingSpeak.setField(currentStockField,  stockPercentage);

  int result = ThingSpeak.writeFields(channelID, writeAPIKey);

  if (result == 200) {
    Serial.println("ThingSpeak upload OK");
  } else {
    Serial.print("ThingSpeak error: ");
    Serial.println(result);
  }
}

// ===================== RFID PROCESSING =====================
void processRFID() {
  String tag = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (i > 0) tag += ":";
    if (rfid.uid.uidByte[i] < 0x10) tag += "0";
    tag += String(rfid.uid.uidByte[i], HEX);
  }
  tag.toUpperCase();

  Serial.print("Scanned Tag: ");
  Serial.println(tag);

  bool found = false;
  for (int i = 0; i < 4; i++) {
    if (tag == items[i].rfidTag) {
      currentItemIndex   = i;
      currentItemName    = items[i].name;
      itemInitialWeight  = items[i].initialWeight;
      lastScannedItem    = items[i].name;
      currentWeightField = items[i].weightField;
      currentStockField  = items[i].stockField;
      itemSelected       = true;
      lastScanTime       = millis();
      found              = true;

      items[i].scanCount++;

      // Every 3 scans = restock, otherwise decrease
      if (items[i].scanCount % 3 == 0) {
        items[i].currentWeight = items[i].initialWeight;
        Serial.println("RESTOCKED: " + items[i].name);
      } else {
        items[i].currentWeight -= items[i].decreaseRate;
        if (items[i].currentWeight < 0) items[i].currentWeight = 0;
        Serial.println("Usage recorded: " + items[i].name);
      }

      currentWeight = items[i].currentWeight;

      // Calculate stock percentage
      stockPercentage = (currentWeight / itemInitialWeight) * 100;
      if (stockPercentage > 100) stockPercentage = 100;
      if (stockPercentage < 0)   stockPercentage = 0;

      Serial.println("Weight: " + String(currentWeight) + "g | Stock: " + String(stockPercentage) + "%");
      break;
    }
  }

  if (!found) {
    currentItemName  = "Unknown Item";
    currentItemIndex = -1;
    itemSelected     = true;
    lastScanTime     = millis();
    Serial.println("Unknown Item - no upload");
    lcdNeedsUpdate = true;
    return;
  }

  sendToThingSpeak();
  lcdNeedsUpdate = true;
}

// ===================== LCD DISPLAY =====================
void showWelcomeScreen() {
  lcd.clear();
  lcd.setCursor(6, 0);
  lcd.print("WELCOME");
  lcd.setCursor(0, 1);
  lcd.print("Last: " + lastScannedItem);
  lcd.setCursor(0, 2);
  lcd.print("--------------------");
  lcd.setCursor(0, 3);
  lcd.print("Scan RFID Tag...");
}

void showItemScreen() {
  lcd.clear();

  // Row 0: Item name
  lcd.setCursor(0, 0);
  lcd.print("Item: ");
  lcd.print(currentItemName);

  // Row 1: Divider
  lcd.setCursor(0, 1);
  lcd.print("--------------------");

  // Row 2: Weight
  lcd.setCursor(0, 2);
  lcd.print("Wt: ");
  lcd.print(currentWeight, 1);
  lcd.print("g  Stk:");
  lcd.print(stockPercentage);
  lcd.print("%");

  // Row 3: Status
  lcd.setCursor(0, 3);
  if (items[currentItemIndex].scanCount % 3 == 0) {
    lcd.print("** RESTOCKED! **    ");
  } else if (stockPercentage < 20) {
    lcd.print("!! LOW - RESTOCK !! ");
  } else if (stockPercentage < 50) {
    lcd.print("Moderate Stock      ");
  } else {
    lcd.print("Stock OK            ");
  }
}
