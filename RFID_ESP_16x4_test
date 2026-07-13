#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ====================== PINS ======================
#define SS_PIN      4
#define RST_PIN     5
#define BUZZER_PIN  15

// ====================== LCD =======================
LiquidCrystal_I2C lcd(0x27, 16, 4);

// ====================== RFID ======================
MFRC522 rfid(SS_PIN, RST_PIN);

// ====================== AUTHORIZED UIDS ======================
String allowedUIDs[] = {
  "73 92 83 FE",
  "45 63 9F 28",
  "2A B1 FD 3E"
};

const int numAllowed =
  sizeof(allowedUIDs) / sizeof(allowedUIDs[0]);

// ====================== HISTORY ======================
String history[3] = {"", "", ""};

// ====================== SIMPLE CLOCK ======================
int hours = 0;
int minutes = 0;

void updateTime() {
  unsigned long secs = millis() / 1000;

  minutes = (secs / 60) % 60;
  hours   = (secs / 3600) % 24;
}

String getTimeString() {
  char buf[6];
  sprintf(buf, "%02d:%02d", hours, minutes);
  return String(buf);
}

// ====================== LCD ROWS ======================
void setLCDRow(byte row) {

  switch (row) {

    case 0:
      lcd.command(0x80);
      break;

    case 1:
      lcd.command(0xC0);
      break;

    case 2:
      lcd.command(0x90);
      break;

    case 3:
      lcd.command(0xD0);
      break;
  }

  delayMicroseconds(100);
}

// ====================== SOUNDS ======================
void successSound() {

  tone(BUZZER_PIN, 1200, 150);
  delay(180);

  tone(BUZZER_PIN, 1600, 150);
  delay(180);

  tone(BUZZER_PIN, 2000, 250);
}

void deniedSound() {

  tone(BUZZER_PIN, 600, 400);
  delay(450);

  tone(BUZZER_PIN, 400, 600);
}

// ====================== SETUP ======================
void setup() {

  Serial.begin(115200);

  delay(2000);

  pinMode(BUZZER_PIN, OUTPUT);

  // I2C
  Wire.begin(21, 22);

  // LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();

  setLCDRow(0);
  lcd.print("RFID SYSTEM");

  setLCDRow(1);
  lcd.print("Initializing");

  // SPI + RFID
  SPI.begin(18, 19, 23, SS_PIN);

  delay(200);

  rfid.PCD_Init();

  delay(200);

  Serial.println("RFID Ready");

  lcd.clear();

  setLCDRow(0);
  lcd.print("Ready to Scan");

  setLCDRow(1);
  lcd.print("Place Card...");
}

// ====================== LOOP ======================
void loop() {

  updateTime();

  if (!rfid.PICC_IsNewCardPresent())
    return;

  if (!rfid.PICC_ReadCardSerial())
    return;

  String uidString = "";

  for (byte i = 0; i < rfid.uid.size; i++) {

    if (rfid.uid.uidByte[i] < 0x10)
      uidString += "0";

    uidString += String(rfid.uid.uidByte[i], HEX);

    if (i < rfid.uid.size - 1)
      uidString += " ";
  }

  uidString.toUpperCase();

  Serial.print("UID: ");
  Serial.println(uidString);

  bool authorized = false;

  for (int i = 0; i < numAllowed; i++) {

    if (uidString == allowedUIDs[i]) {

      authorized = true;
      break;
    }
  }

  // Store history
  history[2] = history[1];
  history[1] = history[0];
  history[0] = uidString;

  lcd.clear();

  // Row 0
  setLCDRow(0);

  if (authorized) {
    lcd.print("ACCESS GRANTED");
    successSound();
  }
  else {
    lcd.print("ACCESS DENIED");
    deniedSound();
  }

  // Row 1
  setLCDRow(1);
  lcd.print(uidString);

  // Row 2
  setLCDRow(2);
  lcd.print("Prev:");

  if (history[1] != "") {
    lcd.print(history[1].substring(0, 11));
  }

  // Row 3
  setLCDRow(3);
  lcd.print(getTimeString());

  lcd.print(" ");

  if (authorized)
    lcd.print("OK");
  else
    lcd.print("NO");

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  delay(3000);

  lcd.clear();

  setLCDRow(0);
  lcd.print("Ready to Scan");

  setLCDRow(1);
  lcd.print("Place Card...");
}
