#pragma once
#include <Arduino.h>
#include "MetronomeChannel.h"

enum NavLevel {
    GLOBAL,     // BPM, Channel selection
    CHANNEL     // Bar length, Pattern
};

class MetronomeState {
private:
    MetronomeChannel channels[2] = {MetronomeChannel(0), MetronomeChannel(1)};
    
public:
    uint16_t bpm = 120;
    bool isRunning = false;
    uint32_t globalTick = 0;
    
    NavLevel navLevel = GLOBAL;
    uint8_t menuPosition = 0;    // Global: 0=BPM, 1=Ch1, 2=Ch2
    uint8_t activeChannel = 0;   // Currently selected channel
    bool isEditing = false;
    uint32_t lastBeatTime = 0;
    uint32_t currentBeat = 0;
    
    const MetronomeChannel& getChannel(uint8_t index) const { 
        return channels[index]; 
    }
    
    MetronomeChannel& getChannel(uint8_t index) { 
        return channels[index]; 
    }
    
    void update() {
        if (isRunning) {
            uint32_t currentTime = millis();
            for (auto& channel : channels) {
                channel.update(bpm, currentTime);
            }
        }
    }
};
