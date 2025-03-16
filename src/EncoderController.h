#pragma once
#include <Arduino.h>
#include "MetronomeState.h"
#include "Timing.h"
#include "config.h"

// Duration for long press detection
#define LONG_PRESS_DURATION_MS 1000

// Duration required to hold all buttons for factory reset
#define FACTORY_RESET_DURATION_MS 3000

class EncoderController
{
private:
  MetronomeState &state;
  Timing &timing;

  // Encoder state tracking
  volatile int32_t encoderValue = 0;
  volatile uint8_t lastEncA = HIGH;

  // Button states
  bool lastEncBtn = HIGH;
  bool lastStartBtn = HIGH;
  bool lastStopBtn = HIGH;
  
  // Long press tracking
  uint32_t buttonPressStartTime = 0;
  bool buttonLongPressActive = false;
  
  // Factory reset detection
  bool factoryResetDetected = false;
  uint32_t factoryResetStartTime = 0;

public:
  EncoderController(MetronomeState &state, Timing &timing);

  void begin();

  bool handleControls();

  void resetEncoders();

  // ISR-compatible method for the encoder
  void encoderISRHandler();

private:
  void handleEncoderButton();
  void handleStartButton();
  void handleStopButton();
  void handleRotaryEncoder();
};

// Declare a global pointer for the ISR to access
extern EncoderController *globalEncoderController;