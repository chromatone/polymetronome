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
    MENU_CH1_TOGGLE = 2,
    MENU_CH1_LENGTH = 3,
    MENU_CH1_PATTERN = 4,
    MENU_CH2_TOGGLE = 5,
    MENU_CH2_LENGTH = 6,
    MENU_CH2_PATTERN = 7
};

class MetronomeState
{
private:
    MetronomeChannel channels[2];
    uint32_t longPressStart = 0;

    uint32_t gcd(uint32_t a, uint32_t b) const;
    uint32_t lcm(uint32_t a, uint32_t b) const;

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
    uint8_t currentMultiplierIndex = 0;
    
    // Euclidean rhythm feedback
    bool euclideanApplied = false;
    uint32_t euclideanAppliedTime = 0;

    MetronomeState();

    const MetronomeChannel &getChannel(uint8_t index) const;
    MetronomeChannel &getChannel(uint8_t index);

    void update();

    uint8_t getMenuItemsCount() const;
    uint8_t getActiveChannel() const;
    bool isChannelSelected() const;
    bool isBpmSelected() const;
    bool isMultiplierSelected() const;
    bool isToggleSelected(uint8_t channel) const;
    bool isLengthSelected(uint8_t channel) const;
    bool isPatternSelected(uint8_t channel) const;
    float getProgress() const;
    uint32_t getTotalBeats() const;
    float getEffectiveBpm() const;
    const char *getCurrentMultiplierName() const;
    float getCurrentMultiplier() const;
    void adjustMultiplier(int8_t delta);
};