#ifndef SOLENOID_CONTROLLER_H
#define SOLENOID_CONTROLLER_H

#include <Arduino.h>
#include <Ticker.h>
#include "MetronomeState.h"
#include "config.h"

class SolenoidController
{
private:
  uint8_t solenoidPin;
  uint8_t solenoidPin2;
  uint16_t accentPulseMs;
  uint16_t weakPulseMs;
  Ticker pulseTicker;
  bool pulseActive = false;

  static SolenoidController *_instance;
  static void IRAM_ATTR endPulseCallback(); // Just declaration, implementation in cpp

public:
  SolenoidController(uint8_t pin_1, uint8_t pin_2, uint16_t weakMs = SOLENOID_PULSE_MS, uint16_t accentMs = ACCENT_PULSE_MS)
      : solenoidPin(pin_1), solenoidPin2(pin_2), weakPulseMs(weakMs), accentPulseMs(accentMs)
  {
    _instance = this;
  }

  ~SolenoidController()
  {
    if (_instance == this)
    {
      _instance = nullptr;
    }
  }

  void init();
  void processBeat(uint8_t channel, BeatState beatState);
  void setPulseDurations(uint16_t weakMs, uint16_t accentMs);
  bool isPulseActive() const;
};

#endif // SOLENOID_CONTROLLER_H