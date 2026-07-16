// ================================================
// Final Year Project -
// Babalola Emmanuel Timilehin
// ESP32-C6 - LCD + WiFi + Buzzer + Email Alert + HX711
// ================================================

#include <Wire.h>
#include <MFRC522.h>
#include <SPI.h>
#include <WiFi.h>
#include <ThingSpeak.h>
#include <HTTPClient.h>
#include <time.h>
#include <HX711.h>

// ===================== PIN DEFINITIONS =====================
#define RST_PIN      5
#define SS_PIN       4
#define BUZZER_PIN   15

// HX711 load cell (Garri only - single Wheatstone bridge)
#define LOADCELL_DT  6
#define LOADCELL_SCK 7

// LCD I2C
#define LCD_ADDR     0x27
#define LCD_COLS     16
#define LCD_ROWS     4

// Confirmed row addresses for 16x4 LCD
const byte rowBase[4] = {0x00, 0x40, 0x10, 0x50};

// ===================== WIFI + THINGSPEAK =====================
const char* ssid          = "Babalola Timilehin";
const char* password      = "Demilade123..";
unsigned long channelID   = 3399453;
const char* writeAPIKey   = "E2I66NZ3FJNJNJCY";

// ===================== GOOGLE APPS SCRIPT EMAIL =====================
const char* googleScriptURL = "https://script.google.com/macros/s/AKfycbwIQcSUd03cfSxGreu7UWh6P7DltgH-5Zk-UBoFlRfREa3SCsaYpgr_2Zvt_BW2SJU0/exec";
// ← REPLACE WITH YOUR DEPLOYED WEB APP URL

WiFiClient client;

// ===================== OBJECTS =====================
MFRC522 rfid(SS_PIN, RST_PIN);
HX711 scale;

// ===================== HX711 CALIBRATION (Garri only) =====================
const long zero_offset = -334740;
const float calibration_factor = 8161.93;

// ===================== ITEM DATABASE =====================
struct Item {
  String name;
  String prefix;
  float currentWeight;
  float lastRestockWeight;
  float lastKnownWeight;   // previous reading - used to detect a genuine restock jump
  float minWeight;
  String rfidTag;
  int weightField;
  int stockField;
  bool hasBeenScanned;     // false until this item's first-ever scan since boot
};

// All weights are in kilograms.
// lastRestockWeight is the "100% capacity" reference for stock % calculations.
// Default is 15.0 kg for all items; it only changes for an item once that
// item registers a genuine restock (weight jump >= RESTOCK_THRESHOLD_KG).
Item items[4] = {
  {"Garri",      "[G]", 0.0, 15.0, 0.0, 0.1,  "73:92:83:FE", 1, 2, false},
  {"Beans",     "[B]", 0.0, 15.0, 0.0, 0.05, "45:63:9F:28", 3, 4, false},
  {"Rice",     "[R]", 0.0, 15.0, 0.0, 0.08, "2A:B1:FD:3E", 5, 6, false},
  {"Detergent", "[D]", 0.0, 15.0, 0.0, 0.01, "AA:BB:CC:DD", 7, 8, false},
};

// A weight increase must be at least this much versus the last known
// reading for an item to be treated as a restock event (as opposed to
// sensor noise or a slight nudge). Only then does the item's capacity
// reference (lastRestockWeight) get reset to the new weight.
const float RESTOCK_THRESHOLD_KG = 10.0;

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

// ---- Background WiFi state ----
bool wifiConnected              = false;  // last known connection state
bool onHomeScreen               = false;  // true only while home screen is displayed
unsigned long lastWifiCheckTime = 0;
const unsigned long WIFI_CHECK_INTERVAL = 1000;  // poll every 1s, non-blocking

const unsigned long INACTIVITY_TIMEOUT  = 300000; // 30 seconds - backlight timeout
const unsigned long HOME_SCREEN_TIMEOUT = 20000;  // 20 seconds - return to home screen
const unsigned long UNKNOWN_TAG_DISPLAY = 5000;   // 5 seconds - unknown tag message duration

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
void sendToThingSpeak();
void sendDailyUpload();
void checkLowStockAndNotify();
void beep(int times = 1);
float getStableWeight(uint8_t samples, float bigJumpThresholdKg, uint8_t maxPasses);

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

  // ---- Stage 1: "System Booting" screen, 4 seconds ----
  lcdClear();
  lcdSetCursor(1, 1); lcdPrint("System Booting");
  Serial.println("[BOOT] Stage 1/2: System Booting screen (4s)...");
  delay(4000);

  // ---- Stage 2: "Initializing" screen with dot animation, ~7 seconds ----
  lcdClear();
  lcdSetCursor(1, 1); lcdPrint("Initializing");
  Serial.println("[BOOT] Stage 2/2: Initializing hardware (7s)...");

  unsigned long initStart = millis();

  SPI.begin(18, 19, 23, SS_PIN);
  rfid.PCD_Init();
  Serial.println("[INIT] RFID (MFRC522) initialized.");

  // NOTE: no tare() here on purpose - offset is a fixed known value from
  // prior calibration, not a runtime zero taken at boot.
  scale.begin(LOADCELL_DT, LOADCELL_SCK);
  scale.set_scale(calibration_factor);
  scale.set_offset(zero_offset);
  Serial.println("[INIT] HX711 load cell initialized.");

  ThingSpeak.begin(client);
  client.setTimeout(3000); 
  
  // cap connect/read attempts at 3s - without this,
  // a WiFi link that's associated but has no real
  // internet path (router up, WAN down) can block
  // writeFields() for tens of seconds with the LCD
  // frozen, since these calls run synchronously
  // inside processRFID()/sendDailyUpload().
  
  Serial.println("[INIT] ThingSpeak client initialized (3s connect timeout).");

  configTime(3600, 0, "pool.ntp.org", "time.nist.gov");  // WAT timezone
  Serial.println("[INIT] NTP time configured.");

  // Fill out the remaining time of the 7-second window with the dot
  // animation, so the screen stays alive even though init itself is fast.
  while (millis() - initStart < 7000) {
    loadingDot(1, 13);
  }

  // ---- Home screen - WiFi has NOT started connecting yet ----
  showHomeScreen();
  lastActivityTime = millis();
  Serial.println("=== SYSTEM READY - home screen shown ===");
  beep(2);

  // ---- Start WiFi connection now, in the background ----
  // WiFi.begin() is non-blocking - it kicks off the connection attempt
  // and returns immediately. Actual connection status is polled inside
  // loop() so it never holds up booting.
  Serial.println("[WIFI] Starting background connection attempt...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
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

// ===================== HX711 WEIGHT READ (Garri only) =====================
// Takes `samples` readings and returns their plain average - no strict
// tolerance rejection. Real load cells jitter by tens of grams between
// reads under normal conditions; treating that as "unstable" (as the
// previous version did, with a 1.5g tolerance on a 20kg load) rejected
// every valid reading and fell back to a stale value.
//
// Stabilization retries only kick in if the spread among those first 10
// readings exceeds `bigJumpThresholdKg` - which isn't normal sensor noise,
// it means the weight actually changed mid-read (e.g. a hand still on the
// container, or a restock in progress). In that case it keeps retrying
// fresh batches of `samples` readings until one settles under the
// threshold, or gives up after `maxPasses` and keeps the last known value.
//
// Every read is bounded by wait_ready_timeout() so an unresponsive HX711
// can never cause a blocking call.
float getStableWeight(uint8_t samples, float bigJumpThresholdKg, uint8_t maxPasses) {
  float readings[samples];
  uint8_t validCount = 0;
  float sum = 0;

  for (uint8_t i = 0; i < samples; i++) {
    if (!scale.wait_ready_timeout(200)) {
      continue; // this sample timed out - skip it, don't block
    }
    float r = scale.get_units(1);  // already in kilograms
    readings[validCount] = r;
    sum += r;
    validCount++;
    delay(40);
  }

  if (validCount == 0) {
    Serial.println("[WEIGHT] HX711 unresponsive - keeping last known Garri weight");
    return items[0].currentWeight;
  }

  float avg = sum / validCount;
  float maxDev = 0;
  for (uint8_t i = 0; i < validCount; i++) {
    float dev = fabs(readings[i] - avg);
    if (dev > maxDev) maxDev = dev;
  }

  Serial.print("[WEIGHT] Average of "); Serial.print(validCount);
  Serial.print(" reads: "); Serial.print(avg, 4);
  Serial.print(" kg, maxDev="); Serial.print(maxDev, 4);
  Serial.println(" kg");

  if (maxDev <= bigJumpThresholdKg) {
    return avg;
  }

  // Spread exceeded the big-jump threshold - the weight was actually
  // changing mid-read, not just noisy. Switch to retrying fresh batches
  // until one settles.
  Serial.println("[WEIGHT] Large mid-read jump detected - retrying until it settles...");

  for (uint8_t pass = 0; pass < maxPasses; pass++) {
    float readings2[samples];
    uint8_t validCount2 = 0;
    float sum2 = 0;

    for (uint8_t i = 0; i < samples; i++) {
      if (!scale.wait_ready_timeout(200)) {
        continue;
      }
      float r = scale.get_units(1);
      readings2[validCount2] = r;
      sum2 += r;
      validCount2++;
      delay(40);
    }

    if (validCount2 < (samples / 2)) {
      Serial.println("[WEIGHT] Retry pass had too few valid samples, trying again...");
      continue;
    }

    float avg2 = sum2 / validCount2;
    float maxDev2 = 0;
    for (uint8_t i = 0; i < validCount2; i++) {
      float dev = fabs(readings2[i] - avg2);
      if (dev > maxDev2) maxDev2 = dev;
    }

    Serial.print("[WEIGHT] Retry pass "); Serial.print(pass + 1);
    Serial.print(": avg="); Serial.print(avg2, 4);
    Serial.print(" kg, maxDev="); Serial.print(maxDev2, 4);
    Serial.println(" kg");

    if (maxDev2 <= bigJumpThresholdKg) {
      Serial.print("[WEIGHT] Settled at "); Serial.print(avg2, 4);
      Serial.println(" kg");
      return avg2;
    }
  }

  Serial.println("[WEIGHT] WARNING: never settled after large jump - keeping last known Garri weight");
  return items[0].currentWeight;
}

// ===================== SCREENS =====================
void showHomeScreen() {
  onHomeScreen = true;
  lcdClear();
  lcdSetCursor(3, 0); lcdPrint("Home Page");
  lcdSetCursor(0, 1); lcdPrint("Last: "); lcdPrint(lastScannedItem);
  lcdSetCursor(0, 2); lcdPrint("Scan RFID Tag");
  // Row 3 intentionally left empty.

  // Draw current WiFi status immediately rather than waiting for the
  // next 1-second poll cycle in loop().
  lcdSetCursor(15, 0);
  lcdPrint(wifiConnected ? "W" : "X");
}

void showScanningScreen() {
  onHomeScreen = false;
  lcdClear();
  lcdSetCursor(3, 0); lcdPrint("Scanning");
  loadingDot(0, 11);
}

void showItemScreen() {
  onHomeScreen = false;
  lcdClear();
  lcdSetCursor(0, 0);
  lcdPrint(items[currentItemIndex].prefix);
  lcdPrint(" ");
  lcdPrint(currentItemName);

  lcdSetCursor(0, 1);
  lcdPrint("Wt: ");
  lcdPrint(currentWeight, 1);
  lcdPrint("kg Stk: ");
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

  Serial.println();
  Serial.print("[RFID] Card detected, UID: ");
  Serial.println(tag);

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

      Serial.print("[RFID] Matched item: ");
      Serial.println(items[i].name);

      if (i == 0) {
        // Garri - the only item physically on the load cell.
        Serial.println("[WEIGHT] Reading Garri load cell...");
        lcdSetCursor(0, 2); lcdPrint("Reading wt");
        loadingDot(2, 10);
        float stableWeight = getStableWeight(10, 35.0, 15);  // 35 kg = gross mid-read jump threshold, not sensor noise tolerance
        items[i].currentWeight = stableWeight;
      } else {
        // Beans / Rice / Detergent - no load cell attached, always 0.0 kg
        items[i].currentWeight = 0.0;
        Serial.println("[WEIGHT] Non-Garri item - fixed at 0.0 kg");
      }

      // ---- Restock detection (threshold-gated) ----
      // A restock is only recognized when the weight increases by at least
      // RESTOCK_THRESHOLD_KG versus the last known reading for THIS item.
      // Small increases (sensor noise, a hand briefly resting on the
      // container) no longer get misread as a restock. When a genuine
      // restock is detected, the new weight becomes this item's new 100%
      // reference (lastRestockWeight) going forward - other items are
      // unaffected and keep their own reference (default 15.0 kg).
      // First scan since boot is a special case: there is no real previous
      // reading to compare against yet (lastKnownWeight is just a 0.0
      // placeholder), so the delta check is skipped entirely. The reading
      // is recorded as the baseline and lastRestockWeight is left untouched
      // (still the default 15.0 kg, or whatever it already was).
      if (!items[i].hasBeenScanned) {
        items[i].hasBeenScanned = true;
        items[i].lastKnownWeight = items[i].currentWeight;
        Serial.print("[STOCK] First scan since boot for ");
        Serial.print(items[i].name); Serial.print(" - baseline set at ");
        Serial.print(items[i].currentWeight, 3);
        Serial.println(" kg, no restock check applied.");
      } else {
        float delta = items[i].currentWeight - items[i].lastKnownWeight;
        if (delta >= RESTOCK_THRESHOLD_KG) {
          items[i].lastRestockWeight = items[i].currentWeight;
          Serial.print("[STOCK] RESTOCK detected for ");
          Serial.print(items[i].name); Serial.print(" (+");
          Serial.print(delta, 2); Serial.print(" kg) -> new 100% reference: ");
          Serial.print(items[i].lastRestockWeight, 3); Serial.println(" kg");
        }
        items[i].lastKnownWeight = items[i].currentWeight;
      }

      currentWeight = items[i].currentWeight;
      stockPercentage = (items[i].lastRestockWeight > 0) ?
                        (int)(currentWeight / items[i].lastRestockWeight * 100) : 0;
      stockPercentage = constrain(stockPercentage, 0, 100);

      Serial.print("[STOCK] "); Serial.print(items[i].name);
      Serial.print(" = "); Serial.print(currentWeight, 3);
      Serial.print(" kg ("); Serial.print(stockPercentage); Serial.println("%)");

      showItemScreen();
      sendToThingSpeak();
      checkLowStockAndNotify();
      found = true;
      break;
    }
  }

  if (!found) {
    Serial.println("[RFID] Unknown tag - no match in item database.");
    onHomeScreen = false;
    lcdClear();
    lcdSetCursor(0, 1); lcdPrint("Unknown Tag!");
    delay(UNKNOWN_TAG_DISPLAY);
    showHomeScreen();
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  // ---- Reset the RF field so a NEW card can be detected promptly ----
  // This is a mitigation for readers that otherwise refuse to recognize
  // a new card until some timeout elapses. It does NOT fix the separate,
  // distinct case where a previous card is still physically resting in
  // the field when a new one is presented - that's an RF-level collision
  // between two active ISO14443 cards, not something software can resolve
  // without full anti-collision handling (which this codebase doesn't
  // implement). If scanning still misbehaves after this, the fix is
  // physically removing the first card before presenting the next one.
  rfid.PCD_AntennaOff();
  delay(50);
  rfid.PCD_AntennaOn();
  Serial.println("[RFID] Antenna reset - ready for next scan.");
}

// ===================== THINGSPEAK =====================
void sendToThingSpeak() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[THINGSPEAK] Skipped upload - WiFi not connected.");
    return;
  }

  ThingSpeak.setField(items[currentItemIndex].weightField, currentWeight);
  ThingSpeak.setField(items[currentItemIndex].stockField, stockPercentage);
  unsigned long callStart = millis();
  int result = ThingSpeak.writeFields(channelID, writeAPIKey);
  unsigned long callDuration = millis() - callStart;

  if (result == 200) {
    Serial.print("[THINGSPEAK] Upload OK. Took ");
    Serial.print(callDuration); Serial.println(" ms.");
  } else {
    Serial.print("[THINGSPEAK] Upload FAILED, HTTP code: ");
    Serial.print(result);
    Serial.print(". Call took "); Serial.print(callDuration); Serial.println(" ms.");
  }
}

void sendDailyUpload() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return;

  if (timeinfo.tm_hour == 23 && timeinfo.tm_min == 0) {
    if (millis() - lastUploadTime > 60000) {
      Serial.println("=== Daily 11PM Upload ===");
      onHomeScreen = false;
      lcdClear();
      lcdSetCursor(1, 1); lcdPrint("Daily Upload");
      loadingDot(1, 13);

      for (int i = 0; i < 4; i++) {
        float pct = (items[i].lastRestockWeight > 0) ?
                    (items[i].currentWeight / items[i].lastRestockWeight * 100.0) : 0;
        pct = constrain(pct, 0, 100);

        ThingSpeak.setField(items[i].weightField, items[i].currentWeight);
        ThingSpeak.setField(items[i].stockField, (int)pct);
        int result = ThingSpeak.writeFields(channelID, writeAPIKey);

        Serial.print("[DAILY UPLOAD] "); Serial.print(items[i].name);
        Serial.print(" -> "); Serial.print(items[i].currentWeight, 3);
        Serial.print(" kg, "); Serial.print((int)pct); Serial.print("%, result=");
        Serial.println(result);

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
    Serial.println("[ALERT] LOW STOCK - sending email notification...");

    beep(3);

    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin(googleScriptURL);
      http.addHeader("Content-Type", "application/json");

      String payload = "{\"item\":\"" + currentItemName +
                      "\",\"stock\":" + String(stockPercentage) +
                      ",\"weight\":" + String(currentWeight, 3) + "}";

      int httpResponseCode = http.POST(payload);

      if (httpResponseCode > 0) {
        Serial.println("[ALERT] Email sent successfully.");
        lcdSetCursor(0, 3);
        lcdPrint("Alert Sent!");
      } else {
        Serial.print("[ALERT] Email FAILED, error code: ");
        Serial.println(httpResponseCode);
        lcdSetCursor(0, 3);
        lcdPrint("Alert Failed");
      }
      http.end();
    } else {
      Serial.println("[ALERT] Skipped email - WiFi not connected.");
    }
    delay(2500);
  }
}

// ===================== LOOP =====================
void loop() {
  // Backlight timeout
  if (lcdBacklightOn && millis() - lastActivityTime > INACTIVITY_TIMEOUT) {
    setBacklight(false);
    Serial.println("[LCD] Backlight timed out - turning off.");
  }

  // ---- Background WiFi status check (non-blocking) ----
  if (millis() - lastWifiCheckTime > WIFI_CHECK_INTERVAL) {
    lastWifiCheckTime = millis();
    bool nowConnected = (WiFi.status() == WL_CONNECTED);

    if (nowConnected != wifiConnected) {
      wifiConnected = nowConnected;
      if (wifiConnected) {
        Serial.print("[WIFI] Connected in background. IP: ");
        Serial.println(WiFi.localIP());
      } else {
        Serial.println("[WIFI] Disconnected / not connected.");
      }
    }

    // Only draw the W/X indicator while sitting on the home screen.
    if (onHomeScreen) {
      lcdSetCursor(15, 0);
      lcdPrint(wifiConnected ? "W" : "X");
    }
  }

  // RFID Scan
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    setBacklight(true);
    lastActivityTime = millis();
    processRFID();
  }

  // Daily upload check
  sendDailyUpload();

  // Return to home screen after 20 seconds of inactivity
  if (itemSelected && millis() - lastScanTime > HOME_SCREEN_TIMEOUT) {
    itemSelected = false;
    currentItemIndex = -1;
    showHomeScreen();
  }

  delay(50);
}
