#include "MetronomeTimer.h"

// Initialize static member
MetronomeTimer* MetronomeTimer::_instance = nullptr;

// Implementation of the static callback function
void IRAM_ATTR MetronomeTimer::timerCallback() {
  if (_instance) {
    _instance->handleInterrupt();
  }
}
