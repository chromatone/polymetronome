#pragma once
#include <Arduino.h>
#include "config.h"

// Forward declaration of WirelessSync class
class WirelessSync;
// External declaration of the global instance
extern WirelessSync* globalWirelessSync;

// Forward declaration for MetronomeState to avoid circular includes
class MetronomeState;

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
    uint8_t barLength;
    uint16_t pattern;
    float multiplier;
    uint8_t currentBeat;
    bool enabled;
    uint32_t lastBeatTime;
    bool editing;
    uint8_t editStep;
    float beatProgress;
    
    // For polyrhythm mode to prevent double triggers
    uint32_t lastTriggeredBeatPosition;
    uint32_t lastTriggeredTick;

public:
    MetronomeChannel(uint8_t channelId);

    void update(uint32_t globalBpm, uint32_t globalTick);
    BeatState getBeatState() const;
    void toggleBeat(uint8_t step);
    void generateEuclidean(uint8_t activeBeats);
    uint8_t getId() const;
    uint8_t getBarLength() const;
    uint16_t getPattern() const;
    float getMultiplier() const;
    uint8_t getCurrentBeat() const;
    bool isEnabled() const;
    bool isEditing() const;
    uint8_t getEditStep() const;
    void setBarLength(uint8_t length);
    void setPattern(uint16_t pat);
    void setMultiplier(float mult);
    void toggleEnabled();
    void setEditing(bool edit);
    void setEditStep(uint8_t step);
    bool getPatternBit(uint8_t position) const;
    float getProgress(uint32_t currentTime, uint32_t globalBpm) const;
    uint16_t getMaxPattern() const;
    void updateProgress(uint32_t globalTick);
    void updateBeat(uint32_t globalTick);
    float getProgress() const;
    void resetBeat();
    
    // New methods for polyrhythm mode
    void updatePolyrhythmBeat(uint32_t masterTick, uint8_t ch1Length, uint8_t ch2Length);
    BeatState getPolyrhythmBeatState(uint32_t ppqnTick, const MetronomeState& state) const;
};