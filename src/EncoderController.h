#pragma once
#include <Arduino.h>
#include "MetronomeState.h"
#include "MetronomeTimer.h"
#include "config.h"

class EncoderController
{
private:
  MetronomeState &state;
  MetronomeTimer &timer;

  // Encoder state tracking
  volatile int32_t encoderValue = 0;
  volatile uint8_t lastEncA = HIGH;

  // Button states
  bool lastEncBtn = HIGH;
  bool lastStartBtn = HIGH;
  bool lastStopBtn = HIGH;

public:
  EncoderController(MetronomeState &state, MetronomeTimer &timer);

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