#include <Arduino.h>
#include <uClock.h>
#include "config.h"
#ifdef MAX_BPM
#undef MAX_BPM
#define MAX_BPM MAX_GLOBAL_BPM
#endif
#include "Display.h"
#include "MetronomeState.h"
#include "SolenoidController.h"
#include "AudioController.h"
#include "EncoderController.h"
#include "WirelessSync.h"
#include "Timing.h"
#include "ConfigManager.h"

MetronomeState state;
Display display;
SolenoidController solenoidController(SOLENOID_PIN, SOLENOID_PIN2);
AudioController audioController(DAC_PIN);
WirelessSync wirelessSync;
Timing timing(state, wirelessSync, solenoidController, audioController);
EncoderController encoderController(state, timing);

// Global pointer to WirelessSync instance for pattern change notifications
WirelessSync* globalWirelessSync = &wirelessSync;

// Timer for periodic config saving
unsigned long lastConfigSaveTime = 0;
const unsigned long CONFIG_SAVE_INTERVAL = 60000; // Save every minute
bool configModified = false;

void setup()
{
    Serial.begin(115200);
    Serial.println("Metronome starting...");
    
    // Initialize Preferences
    if (!ConfigManager::init()) {
        Serial.println("Failed to initialize Preferences storage!");
    }
    
    // Try to load saved configuration
    if (!state.loadFromStorage()) {
        Serial.println("Using default configuration");
    } else {
        Serial.println("Loaded configuration from storage");
    }
    
    // Close preferences to free resources during normal operation
    ConfigManager::end();
    
    solenoidController.init();
    audioController.init();
    display.begin();
    encoderController.begin();

    // Initialize wireless sync
    if (wirelessSync.init()) {
        // Set a random priority based on device ID
        uint8_t devicePriority = random(1, 100);
        wirelessSync.setPriority(devicePriority);
        
        // Start leader negotiation
        wirelessSync.negotiateLeadership();
    }

    // Set display reference in timing
    timing.setDisplay(&display);
    
    // Initialize timing system
    timing.init();
    timing.setTempo(state.bpm);
    
    // Start animation immediately (even before playback starts)
    display.startAnimation();
}

void loop()
{
    // Update timing system
    timing.update();
    
    // Make sure animation is always running
    if (!display.isAnimationRunning())
    {
        display.startAnimation();
    }

    // Handle user input
    bool stateChanged = encoderController.handleControls();
    if (stateChanged) {
        configModified = true;
        
        // Save immediately on important changes
        // Check if we're not in pattern editing mode (which changes frequently)
        if (!state.isEditing) {
            // If we're changing BPM, rhythm mode, or channel properties, save right away
            if (state.isBpmSelected() || state.isRhythmModeSelected() || 
                state.isMultiplierSelected() || state.isChannelSelected()) {
                
                // Initialize Preferences for saving
                ConfigManager::init();
                
                if (state.saveToStorage()) {
                    Serial.println("Configuration saved after important change");
                    configModified = false;
                    lastConfigSaveTime = millis();
                }
                
                // Close preferences to free resources
                ConfigManager::end();
            }
        }
    }
    
    // Update state
    state.update();

    // Update wireless sync
    wirelessSync.update(state);
    
    // Check leader status periodically
    wirelessSync.checkLeaderStatus();

    display.update(state);
    
    // Periodically save configuration if modified
    unsigned long currentTime = millis();
    if (configModified && (currentTime - lastConfigSaveTime > CONFIG_SAVE_INTERVAL)) {
        // Initialize Preferences for saving
        ConfigManager::init();
        
        if (state.saveToStorage()) {
            Serial.println("Configuration auto-saved");
            configModified = false;
            lastConfigSaveTime = currentTime;
        }
        
        // Close preferences to free resources
        ConfigManager::end();
    }
    
    // Prevent watchdog timeouts
    yield();
}