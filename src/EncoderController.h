#pragma once
#include <Arduino.h>
#include "MetronomeState.h"
#include "config.h"

class EncoderController
{
private:
  MetronomeState &state;

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

public:
  EncoderController(MetronomeState &state);

  void begin();

  void handleControls();

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