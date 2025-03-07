#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>

const uint8_t ENC_PUSH = 16;
const uint8_t ENC_A = 17;
const uint8_t ENC_B = 18;
const uint8_t BTN_BACK = 25;     // Stop button
const uint8_t BTN_CONFIRM = 26;  // Start button

enum MenuOption {
  TEMPO,
  SUBDIVISION,
  PATTERN,
  MENU_COUNT
};

volatile int32_t encoderValue = 0;
volatile uint8_t lastEncA = HIGH;
volatile uint8_t lastEncB = HIGH;
int lastEncPush = HIGH;

// New metronome variables
int currentBPM = 120;
unsigned long lastBeatTime = 0;
bool beatState = false;
bool isSettingTempo = false;
bool isRunning = false;          // New running state
unsigned long lastTempoChangeTime = 0;
const unsigned long TEMPO_SETTING_TIMEOUT = 2000; // Exit tempo setting after 2 seconds of inactivity

int lastBackBtn = HIGH;     // Track BACK button state
int lastConfirmBtn = HIGH;  // Track CONFIRM button state

U8G2_SH1106_128X64_NONAME_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE);

// Replace menu enum with new variables
const uint8_t MAX_SUBDIVISIONS = 16;
uint8_t currentSubdivision = 4;  // Default to quarter notes
uint8_t currentStep = 0;         // Current step in sequence
uint16_t currentPattern = 1;     // Binary pattern, default to first beat only
MenuOption currentMenu = TEMPO;   // Keep this

const uint8_t MAX_BAR_LENGTH = 16;
uint8_t barLength = 4;      // Default to 4 beats per bar
uint8_t currentBeat = 0;    // Current beat in bar

// Calculate maximum pattern value for given subdivisions or bar length
uint16_t getMaxPattern(uint8_t value) {
  if (value >= 16) return 32767; // Max 15-bit value (first bit always 1)
  return ((1 << value) - 1) >> 1; // (2^value - 1) / 2
}

// Get bit value at position, ensuring first bit is always 1
uint16_t getValidPattern(uint16_t pattern, uint8_t subdivisions) {
  return pattern << 1 | 1; // Add 1 as first bit
}

bool getPatternBit(uint16_t pattern, uint8_t position) {
  return (pattern >> position) & 1;
}

void IRAM_ATTR encoderISR() {
  uint8_t a = digitalRead(ENC_A);
  uint8_t b = digitalRead(ENC_B);
  
  if (a != lastEncA) {
    lastEncA = a;
    if (a != b) {
      encoderValue++;
    } else {
      encoderValue--;
    }
  }
}

// Add new variable after other state variables
bool isEditing = false; // True when editing a parameter value

// Add flash duration constant at the top with other constants
const unsigned long FLASH_DURATION = 50; // 50ms flash duration
unsigned long lastFlashTime = 0;         // Track when the last flash started

// Modify display code to use barLength consistently
void updateDisplay() {
  display.clearBuffer();
  display.setFont(u8g2_font_t0_11_tr);

  // Draw three lines of parameters
  char buffer[32];
  
  // BPM line with beat indicator
  sprintf(buffer, "BPM: %d", currentBPM);
  display.drawStr(5, 10, buffer);
  if (beatState) {
    display.drawBox(100, 2, 12, 12);
  } else {
    display.drawFrame(100, 2, 12, 12);
  }

  // Bar length line (was Subdivision)
  sprintf(buffer, "BAR: %02d/%d", currentBeat + 1, barLength);
  display.drawStr(5, 22, buffer);

  // Pattern line - use barLength for pattern range
  sprintf(buffer, "PATT: %d/%d", currentPattern, getMaxPattern(barLength));
  display.drawStr(5, 34, buffer);

  // Draw selection indicator
  if (isEditing) {
    display.drawStr(0, 10 + currentMenu * 12, "*");
  }
  display.drawStr(0, 10 + currentMenu * 12, ">");

  // Draw grid at bottom
  const uint8_t gridY = 45;
  const uint8_t gridHeight = 16;
  const uint8_t maxWidth = 124;  // Increased from 120 to use more screen space
  const uint8_t progressY = gridY - 2; // Position for progress bar
  
  // Calculate cell width based on bar length
  uint8_t cellWidth = maxWidth / barLength;
  
  // Calculate progress for smooth animation
  float progress = 0;
  if (isRunning) {
    unsigned long beatInterval = 60000 / currentBPM;
    progress = float(millis() - lastBeatTime) / beatInterval;
    if (progress > 1) progress = 1;
  }
  
  // Draw cells
  for (uint8_t i = 0; i < barLength; i++) {
    uint8_t x = 2 + (i * cellWidth);  // Reduced left margin from 4 to 2
    
    // Fill active beat cell
    if (i == currentBeat && isRunning) {
      display.drawBox(x, gridY, cellWidth-1, gridHeight);
    }
    
    // Draw cell frame
    display.drawFrame(x, gridY, cellWidth-1, gridHeight);
    
    // Draw dot for active beats in pattern using barLength
    if (getPatternBit(getValidPattern(currentPattern, barLength), i)) {
      if (i == currentBeat && isRunning) {
        // Invert the dot on active cell
        display.setDrawColor(0);  // Draw in black on white
        display.drawDisc(x + cellWidth/2, gridY + gridHeight/2, 2);
        display.setDrawColor(1);  // Reset to normal
      } else {
        display.drawDisc(x + cellWidth/2, gridY + gridHeight/2, 2);
      }
    }
  }

  // Draw progress bar
  if (isRunning) {
    uint8_t totalWidth = cellWidth * barLength;
    uint8_t progressWidth = totalWidth * ((currentBeat + progress) / barLength);
    display.drawHLine(2, progressY, progressWidth);
  }

  // Update frame blink check to use flash duration
  if (beatState && (millis() - lastFlashTime < FLASH_DURATION)) {
    display.drawFrame(0, 0, 128, 64);
  }
  
  display.sendBuffer();
}

void setup() {
  Serial.begin(115200);
  
  pinMode(ENC_PUSH, INPUT_PULLUP);
  pinMode(ENC_A, INPUT_PULLUP);
  pinMode(ENC_B, INPUT_PULLUP);
  pinMode(BTN_BACK, INPUT_PULLUP);    // Setup BACK button
  pinMode(BTN_CONFIRM, INPUT_PULLUP);  // Setup CONFIRM button
  
  display.begin();
  display.setFont(u8g2_font_ncenB14_tr);
  updateDisplay();
  
  attachInterrupt(digitalPinToInterrupt(ENC_A), encoderISR, CHANGE);
}

void loop() {
  static int32_t lastEncoderValue = encoderValue;
  // Divide encoder value by 2 to get actual steps
  int32_t currentEncoderStep = encoderValue / 2;
  int32_t lastEncoderStep = lastEncoderValue / 2;
  
  if (currentEncoderStep != lastEncoderStep) {
    int32_t diff = currentEncoderStep - lastEncoderStep;
    
    if (isEditing) {
      // Edit the selected parameter
      switch(currentMenu) {
        case TEMPO:
          currentBPM = constrain(currentBPM + diff, 10, 500);
          break;
        case SUBDIVISION:  // Now controls bar length
          barLength = constrain(barLength + diff, 1, MAX_BAR_LENGTH);
          currentPattern = 0; // Reset pattern when changing subdivisions
          break;
        case PATTERN:
          {
            int16_t newPattern = currentPattern + diff;
            uint16_t maxPattern = getMaxPattern(barLength); // Use barLength here
            if (newPattern < 0) newPattern = maxPattern;
            if (newPattern > maxPattern) newPattern = 0;
            currentPattern = newPattern;
          }
          break;
      }
    } else {
      // Navigate menu
      int8_t newMenu = ((int8_t)currentMenu + diff);
      if (newMenu < 0) newMenu = MENU_COUNT - 1;
      if (newMenu >= MENU_COUNT) newMenu = 0;
      currentMenu = (MenuOption)newMenu;
    }
    lastEncoderValue = encoderValue;
    updateDisplay();
  }

  // Handle encoder push - toggle edit mode instead of tempo setting
  int encPush = digitalRead(ENC_PUSH);
  if (encPush != lastEncPush && encPush == LOW) {
    isEditing = !isEditing;
    updateDisplay();
  }
  lastEncPush = encPush;

  // Remove the tempo setting timeout code since we're using isEditing now

  // Handle start/stop buttons
  int backBtn = digitalRead(BTN_BACK);
  int confirmBtn = digitalRead(BTN_CONFIRM);

  if (confirmBtn != lastConfirmBtn && confirmBtn == LOW) {
    isRunning = true;
    lastBeatTime = millis(); // Reset beat timing when starting
  }
  
  if (backBtn != lastBackBtn && backBtn == LOW) {
    isRunning = false;
    beatState = false;      // Reset beat state when stopping
    currentBeat = 0;        // Reset beat position
    lastBeatTime = millis(); // Reset timing
    updateDisplay();
  }

  lastBackBtn = backBtn;
  lastConfirmBtn = confirmBtn;

  // Handle metronome timing (only if running)
  if (isRunning) {
    unsigned long beatInterval = 60000 / currentBPM; // Just use BPM directly for beat timing
    if (millis() - lastBeatTime >= beatInterval) {
      lastBeatTime = millis();
      currentBeat = (currentBeat + 1) % barLength;
      beatState = getPatternBit(getValidPattern(currentPattern, barLength), currentBeat);
      lastFlashTime = millis();  // Start new flash
    }
    // Update more frequently for smooth progress bar
    updateDisplay();
  }
}