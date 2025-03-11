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
    MetronomeChannel(uint8_t channelId);

    void update(uint32_t globalBpm, uint32_t currentTime);
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
    void updateProgress(uint32_t currentTime, uint32_t lastBeatTime, uint32_t globalBpm);
    void updateBeat();
    float getProgress() const;
    void resetBeat();
};