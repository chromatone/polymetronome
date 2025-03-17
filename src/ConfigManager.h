#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "config.h"
#include "MetronomeState.h"

class ConfigManager {
private:
  static Preferences prefs;

public:
  // Move the static member definition here and make it public
  static inline const char* NAMESPACE_NAME = "metronome";

  // Initialize the storage system
  static bool init() {
    return prefs.begin(NAMESPACE_NAME, false); // Open in RW mode
  }
  
  // Save the current state to persistent storage
  static bool saveConfig(const MetronomeState& state) {
    // Set version and integrity marker
    prefs.putUShort("magicMarker", CONFIG_MAGIC_MARKER);
    prefs.putUChar("version", CONFIG_VERSION);
    
    // Save global parameters
    prefs.putUShort("bpm", state.bpm);
    prefs.putUChar("multiplier", state.currentMultiplierIndex);
    prefs.putUChar("rhythmMode", static_cast<uint8_t>(state.rhythmMode));
    
    // Save channel-specific parameters
    for (uint8_t i = 0; i < FIXED_CHANNEL_COUNT; i++) {
      const MetronomeChannel& channel = state.getChannel(i);
      char keyName[16];
      
      // Create key names for each channel parameter
      snprintf(keyName, sizeof(keyName), "ch%d_enabled", i);
      prefs.putBool(keyName, channel.isEnabled());
      
      snprintf(keyName, sizeof(keyName), "ch%d_barLen", i);
      prefs.putUChar(keyName, channel.getBarLength());
      
      snprintf(keyName, sizeof(keyName), "ch%d_pattern", i);
      prefs.putUShort(keyName, channel.getPattern());
    }
    
    return true; // Preferences automatically commits changes
  }
  
  // Load configuration into the state
  static bool loadConfig(MetronomeState& state) {
    // Verify integrity and version
    uint16_t magicMarker = prefs.getUShort("magicMarker", 0);
    if (magicMarker != CONFIG_MAGIC_MARKER) {
      Serial.println("No valid configuration found");
      return false;
    }
    
    // Handle version differences
    uint8_t version = prefs.getUChar("version", 0);
    if (version != CONFIG_VERSION) {
      Serial.println("Configuration version mismatch - using defaults");
      return false;
    }
    
    // Load global parameters
    state.bpm = constrain(prefs.getUShort("bpm", DEFAULT_BPM), MIN_GLOBAL_BPM, MAX_GLOBAL_BPM);
    state.currentMultiplierIndex = constrain(prefs.getUChar("multiplier", 0), 0, MULTIPLIER_COUNT - 1);
    state.rhythmMode = static_cast<MetronomeMode>(
      constrain(prefs.getUChar("rhythmMode", 0), 0, 1)); // 0=POLYMETER, 1=POLYRHYTHM
    
    // Load channel-specific parameters
    for (uint8_t i = 0; i < FIXED_CHANNEL_COUNT; i++) {
      MetronomeChannel& channel = state.getChannel(i);
      char keyName[16];
      
      // Get enabled state
      snprintf(keyName, sizeof(keyName), "ch%d_enabled", i);
      bool enabled = prefs.getBool(keyName, false);
      
      // Reset first, then set to match saved state if needed
      channel.toggleEnabled(); // Reset first
      if (enabled != channel.isEnabled()) {
        channel.toggleEnabled(); // Toggle to match saved state
      }
      
      // Get bar length
      snprintf(keyName, sizeof(keyName), "ch%d_barLen", i);
      uint8_t barLength = constrain(prefs.getUChar(keyName, 4), 1, MAX_BEATS);
      channel.setBarLength(barLength);
      
      // Get pattern
      snprintf(keyName, sizeof(keyName), "ch%d_pattern", i);
      uint16_t pattern = prefs.getUShort(keyName, 0);
      channel.setPattern(pattern & channel.getMaxPattern()); // Ensure pattern is valid for bar length
    }
    
    return true;
  }
  
  // Clear all stored configuration (reset to factory defaults)
  static bool clearConfig() {
    return prefs.clear();
  }
  
  // Close the preferences namespace when done
  static void end() {
    prefs.end();
  }
  
  // Add the printConfig function as a class method
  static void printConfig() {
    Preferences debugPrefs;
    if (!debugPrefs.begin(NAMESPACE_NAME, true)) { // Open in read-only mode
      Serial.println("Failed to open preferences for debugging");
      return;
    }
    
    Serial.println("Configuration:");
    Serial.print("  Magic Marker: 0x");
    Serial.println(debugPrefs.getUShort("magicMarker", 0), HEX);
    Serial.print("  Version: ");
    Serial.println(debugPrefs.getUChar("version", 0));
    Serial.print("  BPM: ");
    Serial.println(debugPrefs.getUShort("bpm", DEFAULT_BPM));
    Serial.print("  Multiplier Index: ");
    Serial.println(debugPrefs.getUChar("multiplier", 0));
    Serial.print("  Rhythm Mode: ");
    Serial.println(debugPrefs.getUChar("rhythmMode", 0) == 0 ? "POLYMETER" : "POLYRHYTHM");
    
    for (uint8_t i = 0; i < FIXED_CHANNEL_COUNT; i++) {
      Serial.print("  Channel ");
      Serial.print(i + 1);
      Serial.println(":");
      
      char keyName[16];
      
      snprintf(keyName, sizeof(keyName), "ch%d_enabled", i);
      Serial.print("    Enabled: ");
      Serial.println(debugPrefs.getBool(keyName, false) ? "YES" : "NO");
      
      snprintf(keyName, sizeof(keyName), "ch%d_barLen", i);
      Serial.print("    Bar Length: ");
      Serial.println(debugPrefs.getUChar(keyName, 4));
      
      snprintf(keyName, sizeof(keyName), "ch%d_pattern", i);
      uint16_t pattern = debugPrefs.getUShort(keyName, 0);
      Serial.print("    Pattern: 0b");
      
      // Print pattern in binary
      for (int8_t bit = 15; bit >= 0; bit--) {
        Serial.print((pattern >> bit) & 1);
      }
      Serial.println();
    }
    
    debugPrefs.end();
  }
};

// Define the static member variable
// This needs to be outside the class definition
inline Preferences ConfigManager::prefs;