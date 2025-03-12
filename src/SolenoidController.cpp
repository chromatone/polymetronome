#include "SolenoidController.h"

// Initialize static member
SolenoidController* SolenoidController::_instance = nullptr;

// Implementation of the static callback function
void IRAM_ATTR SolenoidController::endPulseCallback()
{
  if (_instance)
  {
    digitalWrite(_instance->solenoidPin, LOW);
    digitalWrite(_instance->solenoidPin2, LOW);
    _instance->pulseActive = false;
  }
}

void SolenoidController::init() {
    pinMode(solenoidPin, OUTPUT);
    pinMode(solenoidPin2, OUTPUT);
    digitalWrite(solenoidPin, LOW);
    digitalWrite(solenoidPin2, LOW);
}

void SolenoidController::processBeat(uint8_t channel, BeatState beatState) {
    if (beatState == ACCENT || beatState == WEAK) {
        digitalWrite((channel ? solenoidPin : solenoidPin2), HIGH);

        // Schedule turning off the solenoid after the appropriate duration
        float pulseDuration = (beatState == ACCENT) ? (accentPulseMs / 1000.0f) : (weakPulseMs / 1000.0f);

        pulseTicker.once(pulseDuration, endPulseCallback);
    }
}

void SolenoidController::setPulseDurations(uint16_t weakMs, uint16_t accentMs) {
    weakPulseMs = weakMs;
    accentPulseMs = accentMs;
}

bool SolenoidController::isPulseActive() const {
    return pulseActive;
}
