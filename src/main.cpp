#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>

// =====================================================
// PIN DEFINITIONS
// =====================================================

const byte BRAKE_INPUT  = 2;
const byte LIGHT_OUTPUT = 3;

const byte BUTTON_UP     = 4;
const byte BUTTON_DOWN   = 5;
const byte BUTTON_SELECT = 6;

// =====================================================
// OLED SETTINGS
// =====================================================

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
#define OLED_ADDRESS 0x3C

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  OLED_RESET
);

// =====================================================
// FLASH PROFILES
// =====================================================

struct FlashProfile {
  unsigned long delayTime;
  unsigned long onTime;
  unsigned long offTime;
};

//                  DELAY    ON     OFF
FlashProfile profiles[] = {
  {1500,            100,    100},   // Profile 1
  {1000,            100,    100},   // Profile 2
  {2000,            100,    100},   // Profile 3
  {1500,             75,     75},   // Profile 4
  {1500,             50,    100},   // Profile 5
  { 500,            100,    100}    // Profile 6
};

const byte PROFILE_COUNT =
  sizeof(profiles) / sizeof(profiles[0]);

// =====================================================
// PROFILE VARIABLES
// =====================================================

byte activeProfile = 0;
byte viewProfile = 0;

const int EEPROM_ACTIVE_PROFILE = 0;

// =====================================================
// FLASHER STATE
// =====================================================

enum FlasherState {
  IDLE,
  BRAKE_DELAY,
  FLASHING
};

FlasherState flasherState = IDLE;

unsigned long stateStartTime = 0;

bool lightState = true;

// =====================================================
// BUTTON VARIABLES
// =====================================================

bool lastUpState = HIGH;
bool lastDownState = HIGH;
bool lastSelectState = HIGH;

// =====================================================
// OLED DISPLAY
// =====================================================

void updateDisplay() {

  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Line 1
  display.setCursor(0, 0);
  display.print("BRAKE FLASHER");

  // Line 2
  display.setCursor(0, 8);
  display.print("VIEWING:");
  display.print(viewProfile + 1);
  display.print(" ACT:");
  display.print(activeProfile + 1);

  // Line 3
  display.setCursor(0, 16);
  display.print("DELAY:");
  display.print(profiles[viewProfile].delayTime);
  display.print("ms");

  // Line 4
  display.setCursor(0, 24);
  display.print("FLASH:");
  display.print(profiles[viewProfile].onTime);
  display.print("/");
  display.print(profiles[viewProfile].offTime);

  display.display();
}

// =====================================================
// STARTUP ANIMATION
// =====================================================

void startupAnimation() {

  // ---------------------------------------------------
  // SCREEN 1 - BRAKE
  // ---------------------------------------------------

  display.clearDisplay();

  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(10, 8);
  display.print("BRAKE");

  display.display();

  delay(500);

  // ---------------------------------------------------
  // SCREEN 2 - FLASHER
  // ---------------------------------------------------

  display.clearDisplay();

  display.setCursor(4, 8);
  display.print("FLASHER");

  display.display();

  delay(500);

  // ---------------------------------------------------
  // SCREEN 3 - BREAKING SOMETHING...
  // ---------------------------------------------------

  display.clearDisplay();

  display.setTextSize(1);

  const char text[] = "BREAKING SOMETHING...";

  // Calculate text width using the default font.
  int textWidth = strlen(text) * 6;

  // Centre the text horizontally.
  int textX = (SCREEN_WIDTH - textWidth) / 2;

  // Keep text inside the display.
  if (textX < 0) {
    textX = 0;
  }

  display.setCursor(textX, 0);
  display.print(text);

  // Progress bar outline
  display.drawRect(
    0,
    15,
    128,
    10,
    SSD1306_WHITE
  );

  // Animate progress bar
  for (int progress = 0; progress <= 124; progress += 2) {

    // Clear only the progress bar area
    display.fillRect(
      1,
      16,
      126,
      8,
      SSD1306_BLACK
    );

    // Draw progress bar
    display.fillRect(
      2,
      17,
      progress,
      6,
      SSD1306_WHITE
    );

    // Redraw progress bar outline
    display.drawRect(
      0,
      15,
      128,
      10,
      SSD1306_WHITE
    );

    display.display();

    delay(30);
  }

  delay(300);

  // Go directly to normal display
  updateDisplay();
}

// =====================================================
// SAVE ACTIVE PROFILE
// =====================================================

void saveActiveProfile() {

  EEPROM.update(
    EEPROM_ACTIVE_PROFILE,
    activeProfile
  );
}

// =====================================================
// LOAD ACTIVE PROFILE
// =====================================================

void loadActiveProfile() {

  activeProfile =
    EEPROM.read(EEPROM_ACTIVE_PROFILE);

  if (activeProfile >= PROFILE_COUNT) {
    activeProfile = 0;
  }

  viewProfile = activeProfile;
}

// =====================================================
// SELECT PROFILE
// =====================================================

void selectProfile() {

  activeProfile = viewProfile;

  saveActiveProfile();

  updateDisplay();
}

// =====================================================
// BUTTON HANDLING
// =====================================================

void checkButtons() {

  bool upState =
    digitalRead(BUTTON_UP);

  bool downState =
    digitalRead(BUTTON_DOWN);

  bool selectState =
    digitalRead(BUTTON_SELECT);

  // ---------------------------------------------------
  // UP
  // ---------------------------------------------------

  if (
    upState == LOW &&
    lastUpState == HIGH
  ) {

    if (viewProfile < PROFILE_COUNT - 1) {
      viewProfile++;
    }
    else {
      viewProfile = 0;
    }

    updateDisplay();
  }

  // ---------------------------------------------------
  // DOWN
  // ---------------------------------------------------

  if (
    downState == LOW &&
    lastDownState == HIGH
  ) {

    if (viewProfile > 0) {
      viewProfile--;
    }
    else {
      viewProfile = PROFILE_COUNT - 1;
    }

    updateDisplay();
  }

  // ---------------------------------------------------
  // SELECT
  // ---------------------------------------------------

  if (
    selectState == LOW &&
    lastSelectState == HIGH
  ) {

    selectProfile();
  }

  lastUpState = upState;
  lastDownState = downState;
  lastSelectState = selectState;
}

// =====================================================
// BRAKE FLASHER
// =====================================================

void brakeFlasher() {

  bool brakePressed =
    digitalRead(BRAKE_INPUT) == HIGH;

  unsigned long now = millis();

  // ===================================================
  // BRAKE RELEASED
  // ===================================================

  if (!brakePressed) {

    // Immediately cancel flashing
    flasherState = IDLE;

    // Running light always ON
    digitalWrite(
      LIGHT_OUTPUT,
      HIGH
    );

    lightState = true;

    return;
  }

  // ===================================================
  // IDLE -> BRAKE DELAY
  // ===================================================

  if (flasherState == IDLE) {

    flasherState = BRAKE_DELAY;

    stateStartTime = now;

    // Running light stays ON
    digitalWrite(
      LIGHT_OUTPUT,
      HIGH
    );

    lightState = true;

    return;
  }

  // ===================================================
  // BRAKE DELAY
  // ===================================================

  if (flasherState == BRAKE_DELAY) {

    if (
      now - stateStartTime >=
      profiles[activeProfile].delayTime
    ) {

      flasherState = FLASHING;

      stateStartTime = now;

      // Start with light OFF
      digitalWrite(
        LIGHT_OUTPUT,
        LOW
      );

      lightState = false;
    }

    return;
  }

  // ===================================================
  // FLASHING
  // ===================================================

  if (flasherState == FLASHING) {

    unsigned long interval;

    if (lightState) {
      interval =
        profiles[activeProfile].onTime;
    }
    else {
      interval =
        profiles[activeProfile].offTime;
    }

    if (now - stateStartTime >= interval) {

      stateStartTime = now;

      lightState = !lightState;

      digitalWrite(
        LIGHT_OUTPUT,
        lightState ? HIGH : LOW
      );
    }
  }
}

// =====================================================
// SETUP
// =====================================================

void setup() {

  // Brake input
  pinMode(
    BRAKE_INPUT,
    INPUT
  );

  // Light output
  pinMode(
    LIGHT_OUTPUT,
    OUTPUT
  );

  // Buttons
  pinMode(
    BUTTON_UP,
    INPUT_PULLUP
  );

  pinMode(
    BUTTON_DOWN,
    INPUT_PULLUP
  );

  pinMode(
    BUTTON_SELECT,
    INPUT_PULLUP
  );

  // Running light ON immediately
  digitalWrite(
    LIGHT_OUTPUT,
    HIGH
  );

  // Load saved profile
  loadActiveProfile();

  // Start OLED
  if (
    !display.begin(
      SSD1306_SWITCHCAPVCC,
      OLED_ADDRESS
    )
  ) {

    // OLED failed
    // Keep running light ON
    digitalWrite(
      LIGHT_OUTPUT,
      HIGH
    );

    while (1) {
      digitalWrite(
        LIGHT_OUTPUT,
        HIGH
      );
    }
  }

  // Startup animation
  startupAnimation();

  // Normal display
  updateDisplay();
}

// =====================================================
// MAIN LOOP
// =====================================================

void loop() {

  checkButtons();

  brakeFlasher();
}
