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
