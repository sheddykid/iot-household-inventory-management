// ================================================
// HARDWARE DIAGNOSTIC SKETCH - ESP32-C6
// Tests I2C (LCD), RFID (MFRC522), HX711, and WiFi
// INDEPENDENTLY of each other and INDEPENDENTLY of
// the main project firmware. Open Serial Monitor at
// 115200 baud after flashing this.
//
// This does NOT run your inventory logic - it only
// checks whether each piece of hardware is actually
// responding on the bus/pins you've wired it to.
// ================================================

#include <Wire.h>
#include <SPI.h>
#include <MFRC522.h>
#include <HX711.h>
#include <WiFi.h>

// ---- Pins - must match your actual wiring ----
#define I2C_SDA      21
#define I2C_SCL      22

#define RST_PIN      5
#define SS_PIN       4
// SPI bus pins used by rfid.PCD_Init() via SPI.begin() below
#define SPI_SCK      18
#define SPI_MISO     19
#define SPI_MOSI     23

#define LOADCELL_DT  6
#define LOADCELL_SCK 7

const char* ssid     = "Babalola Timilehin";
const char* password = "Demilade123..";

MFRC522 rfid(SS_PIN, RST_PIN);
HX711 scale;

void printBanner(const char* title) {
  Serial.println();
  Serial.println("========================================");
  Serial.print("  ");
  Serial.println(title);
  Serial.println("========================================");
}

// ===================== TEST 1: I2C BUS SCAN =====================
// Finds every I2C device actually responding on the bus, regardless
// of what address you think your LCD backpack is on. If your LCD is
// wired correctly, you should see ONE device found, typically at
// 0x27 or 0x3F for PCF8574-based 16x4/20x4 backpacks.
void testI2C() {
  printBanner("TEST 1: I2C BUS SCAN (LCD)");

  Wire.setPins(I2C_SDA, I2C_SCL);
  Wire.begin();
  Wire.setClock(100000);
  delay(100);

  int found = 0;
  for (byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    byte error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("  FOUND I2C device at address 0x");
      if (addr < 16) Serial.print("0");
      Serial.println(addr, HEX);
      found++;
    }
  }

  if (found == 0) {
    Serial.println("  RESULT: FAIL - No I2C devices found at all.");
    Serial.println("  Check: SDA on GPIO21, SCL on GPIO22, VCC to 5V or 3.3V");
    Serial.println("         (check your backpack's rated voltage), GND common");
    Serial.println("         with the ESP32, and that the backpack module itself");
    Serial.println("         is soldered/seated properly.");
  } else if (found == 1) {
    Serial.println("  RESULT: PASS - exactly one I2C device found (expected: your LCD).");
  } else {
    Serial.print("  RESULT: WARNING - "); Serial.print(found);
    Serial.println(" devices found. More than expected - check for address conflicts.");
  }
}

// ===================== TEST 2: MFRC522 RFID READER =====================
// Reads the MFRC522's version register directly. A genuine, correctly
// wired MFRC522 reports version 0x91 or 0x92. 0x00 or 0xFF means the
// SPI bus isn't reaching the chip - dead wiring, no power, or wrong pins.
void testRFID() {
  printBanner("TEST 2: MFRC522 RFID READER");

  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, SS_PIN);
  rfid.PCD_Init();
  delay(100);

  byte version = rfid.PCD_ReadRegister(MFRC522::VersionReg);

  Serial.print("  VersionReg read: 0x");
  Serial.println(version, HEX);

  if (version == 0x00 || version == 0xFF) {
    Serial.println("  RESULT: FAIL - No response from MFRC522.");
    Serial.println("  Check: SDA/SS->GPIO4, SCK->GPIO18, MOSI->GPIO23, MISO->GPIO19,");
    Serial.println("         RST->GPIO5, 3.3V power (MFRC522 is NOT 5V tolerant -");
    Serial.println("         powering it from 5V can permanently damage it), GND common.");
  } else if (version == 0x91 || version == 0x92) {
    Serial.println("  RESULT: PASS - genuine MFRC522 responding correctly.");
  } else {
    Serial.println("  RESULT: WARNING - got a response, but not a recognized MFRC522");
    Serial.println("           version number. Could be a clone chip; may still work.");
  }
}

// ===================== TEST 3: HX711 LOAD CELL =====================
// Checks whether the HX711 is responding at all (independent of
// calibration). If is_ready() never returns true, the ESP32 isn't
// receiving a clock/data toggle from the HX711 - almost always a
// wiring or power issue, not a calibration issue.
void testHX711() {
  printBanner("TEST 3: HX711 LOAD CELL (Rice)");

  scale.begin(LOADCELL_DT, LOADCELL_SCK);
  delay(200);

  bool ready = scale.wait_ready_timeout(2000);

  if (!ready) {
    Serial.println("  RESULT: FAIL - HX711 never signaled ready within 2 seconds.");
    Serial.println("  Check: DT->GPIO6, SCK->GPIO7, VCC to 3.3V or 5V (check your");
    Serial.println("         specific HX711 board's rating), GND common with ESP32.");
    Serial.println("         Also check the 4-wire load cell connections into the");
    Serial.println("         HX711 (E+/E-/A+/A-) are seated and not swapped.");
    return;
  }

  Serial.println("  HX711 responded. Reading 5 raw (uncalibrated) values:");
  for (int i = 0; i < 5; i++) {
    if (scale.wait_ready_timeout(1000)) {
      long raw = scale.read();
      Serial.print("    Raw reading "); Serial.print(i + 1); Serial.print(": ");
      Serial.println(raw);
    } else {
      Serial.println("    (timed out waiting for a reading)");
    }
    delay(200);
  }
  Serial.println("  RESULT: PASS - HX711 is communicating.");
  Serial.println("  NOTE: raw values changing when you press on the load cell");
  Serial.println("        confirms the load cell itself is wired correctly too.");
  Serial.println("        If raw values are static/near-zero even under pressure,");
  Serial.println("        check the load cell's own 4-wire connection to the HX711.");
}

// ===================== TEST 4: WIFI =====================
void testWiFi() {
  printBanner("TEST 4: WIFI");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("  Connecting to \""); Serial.print(ssid); Serial.println("\"...");

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("  RESULT: PASS - connected. IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.print("  RESULT: FAIL - could not connect after ");
    Serial.print(attempts * 500 / 1000);
    Serial.println(" seconds.");
    Serial.println("  Check: SSID/password are exactly correct (case-sensitive),");
    Serial.println("         router is 2.4GHz (ESP32 does not support 5GHz WiFi),");
    Serial.println("         and the ESP32 is in range of the router.");
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000);  // give the serial monitor time to connect before first output

  Serial.println("\n\n#########################################");
  Serial.println("#   ESP32-C6 HARDWARE DIAGNOSTIC START   #");
  Serial.println("#########################################");

  testI2C();
  testRFID();
  testHX711();
  testWiFi();

  Serial.println();
  Serial.println("=========================================");
  Serial.println(" ALL TESTS COMPLETE - see PASS/FAIL above");
  Serial.println("=========================================");
}

void loop() {
  // Nothing here - all tests run once in setup().
  // Reset the board to run the tests again.
  delay(10000);
}
