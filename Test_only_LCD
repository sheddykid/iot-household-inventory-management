#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 4);

// Real DDRAM start addresses for each row on a 20x4 display
const byte rowAddr[4] = {0x00, 0x40, 0x14, 0x54};

// Printable ASCII runs from 0x20 (space) to 0x7E (~)
// That gives 95 characters: A-Z, a-z, 0-9, and all standard symbols

void fillScreen(char startChar) {
  char c = startChar;

  for (int row = 0; row < 4; row++) {
    lcd.command(0x80 | rowAddr[row]); // set cursor directly via DDRAM address
    for (int col = 0; col < 20; col++) {
      lcd.write(c);
      c++;
      if (c > 0x7E) c = 0x20; // wrap around back to space if we run out
    }
  }
}

void setup() {
  lcd.init();
  lcd.backlight();
}

void loop() {
  // Shift the starting character each loop so you can see
  // every character eventually land on every cell position
  for (char start = 0x20; start <= 0x7E; start++) {
    lcd.clear();
    fillScreen(start);
    delay(800); // 0.8 seconds per frame, adjust if too fast or slow
  }
}
