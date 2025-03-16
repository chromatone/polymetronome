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

MetronomeState state;
Display display;
SolenoidController solenoidController(SOLENOID_PIN, SOLENOID_PIN2);
AudioController audioController(DAC_PIN);
WirelessSync wirelessSync;
Timing timing(state, wirelessSync, solenoidController, audioController);
EncoderController encoderController(state, timing);

// Global pointer to WirelessSync instance for pattern change notifications
WirelessSync* globalWirelessSync = &wirelessSync;

void setup()
{
    Serial.begin(115200);
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

    encoderController.handleControls();
    
    // Update state
    state.update();

    // Update wireless sync
    wirelessSync.update(state);
    
    // Check leader status periodically
    wirelessSync.checkLeaderStatus();

    display.update(state);
    // Prevent watchdog timeouts
    yield();
}