#include "ConfigManager.h"

// This file can remain mostly empty since we've implemented the functions as static inline methods
// in the header file. Any future non-trivial implementation details would go here.

// For debugging purposes, we can add a method to print the current configuration
void printConfig(const MetronomeConfig& config) {
  Serial.println("Configuration:");
  Serial.print("  Magic Marker: 0x");
  Serial.println(config.magicMarker, HEX);
  Serial.print("  Version: ");
  Serial.println(config.version);
  Serial.print("  BPM: ");
  Serial.println(config.bpm);
  Serial.print("  Multiplier Index: ");
  Serial.println(config.multiplierIndex);
  Serial.print("  Rhythm Mode: ");
  Serial.println(config.rhythmMode == 0 ? "POLYMETER" : "POLYRHYTHM");
  
  for (uint8_t i = 0; i < FIXED_CHANNEL_COUNT; i++) {
    Serial.print("  Channel ");
    Serial.print(i + 1);
    Serial.println(":");
    Serial.print("    Enabled: ");
    Serial.println(config.channels[i].enabled ? "YES" : "NO");
    Serial.print("    Bar Length: ");
    Serial.println(config.channels[i].barLength);
    Serial.print("    Pattern: 0b");
    
    // Print pattern in binary
    for (int8_t bit = 15; bit >= 0; bit--) {
      Serial.print((config.channels[i].pattern >> bit) & 1);
    }
    Serial.println();
  }
} 