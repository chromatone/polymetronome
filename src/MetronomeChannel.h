#pragma once
#include <Arduino.h>
#include "config.h"

enum BeatState
{
    SILENT = 0,
    WEAK = 1,
    ACCENT = 2
};

class MetronomeChannel
{
private:
    uint8_t id;
    uint8_t barLength = 4;
    uint16_t pattern = 1;
    float multiplier = 1.0;
    uint8_t currentBeat = 0;
    bool enabled = true;
    uint32_t lastBeatTime = 0;
    bool editing = false;
    uint8_t editStep = 0;
    float beatProgress = 0.0f;

public:
    MetronomeChannel(uint8_t channelId) : id(channelId) {}

    void update(uint32_t globalBpm, uint32_t currentTime) {
        if (!enabled)
            return;

        uint32_t effectiveBpm = globalBpm * multiplier;
        uint32_t beatInterval = 60000 / effectiveBpm;

        if (currentTime - lastBeatTime >= beatInterval) {
            currentBeat = (currentBeat + 1) % barLength;
            lastBeatTime = currentTime;
        }
    }

    BeatState getBeatState() const {
        if (!enabled)
            return SILENT;
        uint16_t fullPattern = (pattern << 1) | 1; // First beat always on
        return (fullPattern >> currentBeat) & 1 ? ACCENT : SILENT;
    }

    void toggleBeat(uint8_t step) {
        if (step == 0)
            return; // Can't toggle first beat
        uint16_t mask = 1 << step;
        pattern ^= mask;
    }

    void generateEuclidean(uint8_t activeBeats) {
        // Ensure first beat is included in count
        activeBeats = constrain(activeBeats, 1, barLength);
        pattern = 0;

        // Euclidean algorithm implementation
        float spacing = float(barLength) / activeBeats;
        for (uint8_t i = 1; i < barLength; i++) {
            if (int(i * spacing) != int((i - 1) * spacing)) {
                pattern |= (1 << i);
            }
        }
    }

    uint8_t getId() const { return id; }
    uint8_t getBarLength() const { return barLength; }
    uint16_t getPattern() const { return pattern; }
    float getMultiplier() const { return multiplier; }
    uint8_t getCurrentBeat() const { return currentBeat; }
    bool isEnabled() const { return enabled; }
    bool isEditing() const { return editing; }
    uint8_t getEditStep() const { return editStep; }

    void setBarLength(uint8_t length) {
        barLength = constrain(length, 1, 16);
        pattern &= ((1 << barLength) - 1);
    }

    void setPattern(uint16_t pat) { pattern = pat; }
    void setMultiplier(float mult) { multiplier = mult; }
    void toggleEnabled() { enabled = !enabled; }
    void setEditing(bool edit) { editing = edit; }
    
    void setEditStep(uint8_t step) {
        editStep = step % barLength;
    }

    bool getPatternBit(uint8_t position) const {
        if (!enabled)
            return false;
        uint16_t fullPattern = (pattern << 1) | 1; // First beat always on
        return (fullPattern >> position) & 1;
    }

    float getProgress(uint32_t currentTime, uint32_t globalBpm) const {
        if (!enabled || !lastBeatTime)
            return 0.0f;
        uint32_t beatInterval = 60000 / (globalBpm * multiplier);
        return float(currentTime - lastBeatTime) / beatInterval;
    }

    uint16_t getMaxPattern() const {
        if (barLength >= 16)
            return 32767;                     // Max 15-bit value
        return ((1 << barLength) - 1) >> 1;   // (2^length - 1) / 2, first bit always 1
    }

    void updateProgress(uint32_t globalTick) {
        if (!enabled)
            return;
        // Calculate progress based on the global tick
        // This is simpler and more consistent than using millis()
        beatProgress = (globalTick % 1) / 1.0f;
    }

    void updateBeat() {
        if (!enabled)
            return;
        currentBeat = (currentBeat + 1) % barLength;
    }

    float getProgress() const {
        return enabled ? beatProgress : 0.0f;
    }

    void resetBeat() {
        currentBeat = 0;
        lastBeatTime = 0;
        beatProgress = 0.0f;
    }
};