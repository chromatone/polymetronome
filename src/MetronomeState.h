#pragma once
#include <Arduino.h>
#include "MetronomeChannel.h"

enum NavLevel {
    GLOBAL,     // BPM, Length, Pattern selection
    PATTERN     // Step editing (future implementation)
};

class MetronomeState {
private:
    MetronomeChannel channels[2] = {MetronomeChannel(0), MetronomeChannel(1)};
    
public:
    uint16_t bpm = 120;
    bool isRunning = false;
    uint32_t globalTick = 0;
    
    NavLevel navLevel = GLOBAL;
    uint8_t menuPosition = 0;    // Global: 0=BPM, 1=Ch1Len, 2=Ch1Pat, 3=Ch2Len, 4=Ch2Pat
    bool isEditing = false;
    uint32_t lastBeatTime = 0;
    uint32_t currentBeat = 0;
    uint32_t longPressStart = 0;
    bool longPressActive = false;
    
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

    // Remove globalMultiplier and related methods

    uint8_t getMenuItemsCount() const {
        return 5; // BPM, Ch1Len, Ch1Pat, Ch2Len, Ch2Pat
    }

    // Add these helper methods
    uint8_t getActiveChannel() const {
        return (menuPosition - 1) / 2;
    }

    bool isChannelSelected() const {
        return menuPosition > 0;
    }

    // Helper methods to determine which parameter is selected
    bool isBpmSelected() const { return navLevel == GLOBAL && menuPosition == 0; }
    bool isLengthSelected(uint8_t channel) const { 
        return navLevel == GLOBAL && menuPosition == (channel * 2 + 1); 
    }
    bool isPatternSelected(uint8_t channel) const { 
        return navLevel == GLOBAL && menuPosition == (channel * 2 + 2); 
    }

    // Calculate progress for global cycle
    float getGlobalProgress() const {
        if (!isRunning || !lastBeatTime) return 0.0f;
        uint32_t beatInterval = 60000 / bpm;
        return float(millis() - lastBeatTime) / beatInterval;
    }
};
