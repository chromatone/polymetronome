#pragma once
#include <Arduino.h>
#include <EEPROM.h>
#include "config.h"
#include "MetronomeState.h"

class ConfigManager {
private:
  static const size_t EEPROM_SIZE = 512; // Adjust based on your ESP32 model

public:
  // Initialize the storage system
  static bool init() {
    return EEPROM.begin(EEPROM_SIZE);
  }
  
  // Save the current state to persistent storage
  static bool saveConfig(const MetronomeState& state) {
    MetronomeConfig config;
    
    // Set version and integrity marker
    config.magicMarker = CONFIG_MAGIC_MARKER;
    config.version = CONFIG_VERSION;
    
    // Save global parameters
    config.bpm = state.bpm;
    config.multiplierIndex = state.currentMultiplierIndex;
    config.rhythmMode = static_cast<uint8_t>(state.rhythmMode);
    
    // Save channel-specific parameters
    for (uint8_t i = 0; i < FIXED_CHANNEL_COUNT; i++) {
      const MetronomeChannel& channel = state.getChannel(i);
      config.channels[i].enabled = channel.isEnabled();
      config.channels[i].barLength = channel.getBarLength();
      config.channels[i].pattern = channel.getPattern();
    }
    
    // Write to EEPROM
    EEPROM.put(CONFIG_STORAGE_ADDR, config);
    return EEPROM.commit();
  }
  
  // Load configuration into the state
  static bool loadConfig(MetronomeState& state) {
    MetronomeConfig config;
    
    // Read from EEPROM
    EEPROM.get(CONFIG_STORAGE_ADDR, config);
    
    // Verify integrity and version
    if (config.magicMarker != CONFIG_MAGIC_MARKER) {
      Serial.println("No valid configuration found");
      return false;
    }
    
    // Handle version differences
    if (config.version != CONFIG_VERSION) {
      Serial.println("Configuration version mismatch - using defaults");
      return false;
    }
    
    // Load global parameters
    state.bpm = constrain(config.bpm, MIN_GLOBAL_BPM, MAX_GLOBAL_BPM);
    state.currentMultiplierIndex = constrain(config.multiplierIndex, 0, MULTIPLIER_COUNT - 1);
    state.rhythmMode = static_cast<MetronomeMode>(
      constrain(config.rhythmMode, 0, 1)); // 0=POLYMETER, 1=POLYRHYTHM
    
    // Load channel-specific parameters
    for (uint8_t i = 0; i < FIXED_CHANNEL_COUNT; i++) {
      MetronomeChannel& channel = state.getChannel(i);
      channel.toggleEnabled(); // Reset first
      if (config.channels[i].enabled != channel.isEnabled()) {
        channel.toggleEnabled(); // Toggle to match saved state
      }
      
      uint8_t barLength = constrain(config.channels[i].barLength, 1, MAX_BEATS);
      channel.setBarLength(barLength);
      
      uint16_t pattern = config.channels[i].pattern;
      channel.setPattern(pattern & channel.getMaxPattern()); // Ensure pattern is valid for bar length
    }
    
    return true;
  }
  
  // Clear all stored configuration (reset to factory defaults)
  static bool clearConfig() {
    MetronomeConfig config;
    config.magicMarker = 0; // Invalid marker
    config.version = 0;
    
    EEPROM.put(CONFIG_STORAGE_ADDR, config);
    return EEPROM.commit();
  }
}; 