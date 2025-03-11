#ifndef SOLENOID_CONTROLLER_H
#define SOLENOID_CONTROLLER_H

#include <Arduino.h>
#include "MetronomeState.h"
#include "config.h"

class SolenoidController
{
private:
  uint8_t solenoidPin;
  uint16_t accentPulseMs;
  uint16_t weakPulseMs;

public:
  SolenoidController(uint8_t pin, uint16_t weakMs = SOLENOID_PULSE_MS, uint16_t accentMs = ACCENT_PULSE_MS)
      : solenoidPin(pin), weakPulseMs(weakMs), accentPulseMs(accentMs)
  {
  }

  void init()
  {
    pinMode(solenoidPin, OUTPUT);
    digitalWrite(solenoidPin, LOW);
  }

  void processBeat(uint8_t channel, BeatState beatState)
  {
    if (beatState == ACCENT)
    {
      digitalWrite(solenoidPin, HIGH);
      delayMicroseconds(accentPulseMs * 1000);
      digitalWrite(solenoidPin, LOW);
    }
    else if (beatState == WEAK)
    {
      digitalWrite(solenoidPin, HIGH);
      delayMicroseconds(weakPulseMs * 1000);
      digitalWrite(solenoidPin, LOW);
    }
  }

  void setPulseDurations(uint16_t weakMs, uint16_t accentMs)
  {
    weakPulseMs = weakMs;
    accentPulseMs = accentMs;
  }
};

#endif // SOLENOID_CONTROLLER_H