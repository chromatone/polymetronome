#pragma once
#include <Arduino.h>
#include "MetronomeChannel.h"
#include "config.h"

enum NavLevel
{
    GLOBAL,
    PATTERN
};

enum MenuPosition
{
    MENU_BPM = 0,
    MENU_MULTIPLIER = 1,
    MENU_CH1_LENGTH = 2,
    MENU_CH1_PATTERN = 3,
    MENU_CH2_LENGTH = 4,
    MENU_CH2_PATTERN = 5
};

class MetronomeState
{
private:
    MetronomeChannel channels[2];
    uint32_t longPressStart = 0;

    uint32_t gcd(uint32_t a, uint32_t b) const {
        while (b != 0) {
            uint32_t t = b;
            b = a % b;
            a = t;
        }
        return a;
    }
    
    uint32_t lcm(uint32_t a, uint32_t b) const {
        return (a * b) / gcd(a, b);
    }

public:
    static const uint8_t CHANNEL_COUNT = 2;

    const float multiplierValues[MULTIPLIER_COUNT] = MULTIPLIERS;
    const char *multiplierNames[MULTIPLIER_COUNT] = MULTIPLIER_NAMES;

    uint16_t bpm = 120;
    bool isRunning = false;
    bool isPaused = false;
    volatile uint32_t globalTick = 0; // Made volatile for ISR access
    uint32_t lastBeatTime = 0;        // Kept public as used by other modules

    NavLevel navLevel = GLOBAL;
    MenuPosition menuPosition = MENU_BPM;
    bool isEditing = false;
    uint32_t currentBeat = 0;
    bool longPressActive = false;
    uint8_t currentMultiplierIndex = 3;

    // Constructor with initialization
    MetronomeState() : channels{MetronomeChannel(0), MetronomeChannel(1)} {}

    const MetronomeChannel &getChannel(uint8_t index) const {
        return channels[index];
    }
    
    MetronomeChannel &getChannel(uint8_t index) {
        return channels[index];
    }

    void update() {
        if (isRunning) {
            uint32_t currentTime = millis();
            if (lastBeatTime == 0) {
                lastBeatTime = currentTime;
            }

            // No beat timing logic here anymore - it's handled by the timer ISR

            // Just update progress for display purposes
            for (auto &channel : channels) {
                channel.updateProgress(currentTime, lastBeatTime, getEffectiveBpm());
            }
        }
    }

    uint8_t getMenuItemsCount() const {
        return 6;
    }
    
    uint8_t getActiveChannel() const {
        return (static_cast<uint8_t>(menuPosition) - MENU_CH1_LENGTH) / 2;
    }
    
    bool isChannelSelected() const {
        return static_cast<uint8_t>(menuPosition) > MENU_MULTIPLIER;
    }

    bool isBpmSelected() const {
        return navLevel == GLOBAL && menuPosition == MENU_BPM;
    }
    
    bool isMultiplierSelected() const {
        return navLevel == GLOBAL && menuPosition == MENU_MULTIPLIER;
    }
    
    bool isLengthSelected(uint8_t channel) const {
        return navLevel == GLOBAL &&
               menuPosition == static_cast<MenuPosition>(MENU_CH1_LENGTH + channel * 2);
    }
    
    bool isPatternSelected(uint8_t channel) const {
        return navLevel == GLOBAL &&
               menuPosition == static_cast<MenuPosition>(MENU_CH1_PATTERN + channel * 2);
    }

    float getGlobalProgress() const {
        if (!isRunning || !lastBeatTime)
            return 0.0f;
        uint32_t beatInterval = 60000 / getEffectiveBpm();
        return float(millis() - lastBeatTime) / beatInterval;
    }

    float getProgress() const {
        if (!isRunning || !lastBeatTime)
            return 0.0f;
        uint32_t beatInterval = 60000 / getEffectiveBpm();
        uint32_t elapsed = millis() - lastBeatTime;
        return float(elapsed) / beatInterval;
    }
    
    uint32_t getTotalBeats() const {
        uint32_t result = channels[0].getBarLength();
        for (uint8_t i = 1; i < CHANNEL_COUNT; i++) {
            result = lcm(result, channels[i].getBarLength());
        }
        return result;
    }

    float getEffectiveBpm() const {
        return bpm * multiplierValues[currentMultiplierIndex];
    }
    
    const char *getCurrentMultiplierName() const {
        return multiplierNames[currentMultiplierIndex];
    }
    
    void adjustMultiplier(int8_t delta) {
        currentMultiplierIndex = (currentMultiplierIndex + MULTIPLIER_COUNT + delta) % MULTIPLIER_COUNT;
    }
};