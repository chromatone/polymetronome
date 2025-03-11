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
  uint16_t accentPulseMs;
  uint16_t weakPulseMs;
  Ticker pulseTicker;
  bool pulseActive = false;

  static SolenoidController *_instance;
  static void IRAM_ATTR endPulseCallback(); // Just declaration, implementation in cpp

public:
  SolenoidController(uint8_t pin, uint16_t weakMs = SOLENOID_PULSE_MS, uint16_t accentMs = ACCENT_PULSE_MS)
      : solenoidPin(pin), weakPulseMs(weakMs), accentPulseMs(accentMs)
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

  void init()
  {
    pinMode(solenoidPin, OUTPUT);
    digitalWrite(solenoidPin, LOW);
  }

  void processBeat(uint8_t channel, BeatState beatState)
  {
    // Only process if no pulse is currently active
    if (!pulseActive)
    {
      if (beatState == ACCENT || beatState == WEAK)
      {
        pulseActive = true;
        digitalWrite(solenoidPin, HIGH);

        // Schedule turning off the solenoid after the appropriate duration
        float pulseDuration = (beatState == ACCENT) ? (accentPulseMs / 1000.0f) : (weakPulseMs / 1000.0f);

        pulseTicker.once(pulseDuration, endPulseCallback);
      }
    }
  }

  void setPulseDurations(uint16_t weakMs, uint16_t accentMs)
  {
    weakPulseMs = weakMs;
    accentPulseMs = accentMs;
  }

  bool isPulseActive() const
  {
    return pulseActive;
  }
};

#endif // SOLENOID_CONTROLLER_H