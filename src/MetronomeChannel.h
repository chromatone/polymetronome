#pragma once
#include <Arduino.h>
#include "config.h"

class MetronomeChannel {
private:
    uint8_t id;
    uint8_t barLength = 4;
    uint16_t pattern = 1;
    float multiplier = 1.0;
    uint8_t currentBeat = 0;
    bool beatState = false;
    uint32_t lastBeatTime = 0;

public:
    MetronomeChannel(uint8_t channelId) : id(channelId) {}

    void update(uint32_t globalBpm, uint32_t currentTime) {
        uint32_t effectiveBpm = globalBpm * multiplier;
        uint32_t beatInterval = 60000 / effectiveBpm;
        
        if (currentTime - lastBeatTime >= beatInterval) {
            currentBeat = (currentBeat + 1) % barLength;
            beatState = getPatternBit(currentBeat);
            lastBeatTime = currentTime;
        }
    }

    bool getPatternBit(uint8_t position) const {
        uint16_t fullPattern = (pattern << 1) | 1;
        return (fullPattern >> position) & 1;
    }

    uint16_t getMaxPattern() const {
        if (barLength >= 16) return 32767;
        return ((1 << barLength) - 1) >> 1;
    }

    // Getters
    uint8_t getId() const { return id; }
    uint8_t getBarLength() const { return barLength; }
    uint16_t getPattern() const { return pattern; }
    float getMultiplier() const { return multiplier; }
    uint8_t getCurrentBeat() const { return currentBeat; }
    bool getBeatState() const { return beatState; }

    // Setters
    void setBarLength(int8_t length) { barLength = constrain(length, 1, MAX_BEATS); }
    void setPattern(int16_t pat) { pattern = constrain(pat, 0, getMaxPattern()); }
    void toggleMultiplier() {
        if (multiplier == 0.5f) multiplier = 1.0f;
        else if (multiplier == 1.0f) multiplier = 2.0f;
        else multiplier = 0.5f;
    }
};
