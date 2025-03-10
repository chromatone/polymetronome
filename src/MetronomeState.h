#pragma once
#include <Arduino.h>
#include "MetronomeChannel.h"
#include "config.h"

enum NavLevel
{
    GLOBAL, // BPM, Length, Pattern selection
    PATTERN // Step editing (future implementation)
};

class MetronomeState
{
private:
    MetronomeChannel channels[2] = {MetronomeChannel(0), MetronomeChannel(1)};
    const float multiplierValues[MULTIPLIER_COUNT] = MULTIPLIERS;
    const char *multiplierNames[MULTIPLIER_COUNT] = MULTIPLIER_NAMES;
    uint8_t currentMultiplierIndex = 3; // Default to 1x

public:
    uint16_t bpm = 120;
    bool isRunning = false;
    bool isPaused = false;
    uint32_t globalTick = 0;

    NavLevel navLevel = GLOBAL;
    uint8_t menuPosition = 0; // Global: 0=BPM, 1=Multiplier, 2=Ch1Len, 3=Ch1Pat, 4=Ch2Len, 5=Ch2Pat
    bool isEditing = false;
    uint32_t lastBeatTime = 0;
    uint32_t currentBeat = 0;
    uint32_t longPressStart = 0;
    bool longPressActive = false;

    const MetronomeChannel &getChannel(uint8_t index) const
    {
        return channels[index];
    }

    MetronomeChannel &getChannel(uint8_t index)
    {
        return channels[index];
    }

    void update()
    {
        if (isRunning)
        {
            uint32_t currentTime = millis();

            // If we're resuming from a pause, update lastBeatTime to maintain rhythm
            if (lastBeatTime == 0)
            {
                lastBeatTime = currentTime;
            }

            uint32_t beatInterval = 60000 / getEffectiveBpm();

            if (currentTime - lastBeatTime >= beatInterval)
            {
                globalTick++;
                lastBeatTime = currentTime;

                for (auto &channel : channels)
                {
                    channel.updateBeat();
                }
            }

            for (auto &channel : channels)
            {
                channel.updateProgress(currentTime, lastBeatTime, getEffectiveBpm());
            }
        }
    }
    uint8_t getMenuItemsCount() const
    {
        return 6; // BPM, Multiplier, Ch1Len, Ch1Pat, Ch2Len, Ch2Pat
    }

    uint8_t getActiveChannel() const
    {
        return (menuPosition - 2) / 2; // Adjusted for new multiplier position
    }

    bool isChannelSelected() const
    {
        return menuPosition > 1; // Adjusted for new multiplier position
    }

    // Helper methods to determine which parameter is selected
    bool isBpmSelected() const { return navLevel == GLOBAL && menuPosition == 0; }
    bool isMultiplierSelected() const { return navLevel == GLOBAL && menuPosition == 1; }
    bool isLengthSelected(uint8_t channel) const
    {
        return navLevel == GLOBAL && menuPosition == (channel * 2 + 2);
    }
    bool isPatternSelected(uint8_t channel) const
    {
        return navLevel == GLOBAL && menuPosition == (channel * 2 + 3);
    }

    // Calculate progress for global cycle
    float getGlobalProgress() const
    {
        if (!isRunning || !lastBeatTime)
            return 0.0f;
        uint32_t beatInterval = 60000 / getEffectiveBpm();
        return float(millis() - lastBeatTime) / beatInterval;
    }

    // Add new helper methods
    uint32_t gcd(uint32_t a, uint32_t b) const
    {
        while (b != 0)
        {
            uint32_t t = b;
            b = a % b;
            a = t;
        }
        return a;
    }

    uint32_t lcm(uint32_t a, uint32_t b) const
    {
        return (a * b) / gcd(a, b);
    }

    uint32_t getTotalBeats() const
    {
        uint32_t result = channels[0].getBarLength();
        for (uint8_t i = 1; i < 2; i++)
        {
            result = lcm(result, channels[i].getBarLength());
        }
        return result;
    }

    float getProgress() const
    {
        if (!isRunning || !lastBeatTime)
            return 0.0f;
        uint32_t beatInterval = 60000 / getEffectiveBpm();
        uint32_t elapsed = millis() - lastBeatTime;
        return float(elapsed) / beatInterval;
    }

    float getEffectiveBpm() const
    {
        return bpm * multiplierValues[currentMultiplierIndex];
    }

    const char *getCurrentMultiplierName() const
    {
        return multiplierNames[currentMultiplierIndex];
    }

    void adjustMultiplier(int8_t delta)
    {
        currentMultiplierIndex = (currentMultiplierIndex + MULTIPLIER_COUNT + delta) % MULTIPLIER_COUNT;
    }
};
