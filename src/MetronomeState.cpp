#include "MetronomeState.h"

uint32_t MetronomeState::gcd(uint32_t a, uint32_t b) const {
    while (b != 0) {
        uint32_t t = b;
        b = a % b;
        a = t;
    }
    return a;
}

uint32_t MetronomeState::lcm(uint32_t a, uint32_t b) const {
    return (a * b) / gcd(a, b);
}

MetronomeState::MetronomeState() : channels{MetronomeChannel(0), MetronomeChannel(1)} {}

const MetronomeChannel &MetronomeState::getChannel(uint8_t index) const {
    return channels[index];
}

MetronomeChannel &MetronomeState::getChannel(uint8_t index) {
    return channels[index];
}

void MetronomeState::update() {
    if (isRunning) {
        for (auto &channel : channels) {
            channel.updateProgress(globalTick);
        }
    }
    
    // Check if Euclidean feedback should be cleared
    if (euclideanApplied && (millis() - euclideanAppliedTime > LONG_PRESS_DURATION_MS)) {
        euclideanApplied = false;
    }
}

uint8_t MetronomeState::getMenuItemsCount() const {
    return 8;
}

uint8_t MetronomeState::getActiveChannel() const {
    uint8_t pos = static_cast<uint8_t>(menuPosition);
    return (pos >= MENU_CH1_TOGGLE && pos <= MENU_CH1_PATTERN) ? 0 : 
           (pos >= MENU_CH2_TOGGLE && pos <= MENU_CH2_PATTERN) ? 1 : 0;
}

bool MetronomeState::isChannelSelected() const {
    return static_cast<uint8_t>(menuPosition) > MENU_MULTIPLIER;
}

bool MetronomeState::isBpmSelected() const {
    return navLevel == GLOBAL && menuPosition == MENU_BPM;
}

bool MetronomeState::isMultiplierSelected() const {
    return navLevel == GLOBAL && menuPosition == MENU_MULTIPLIER;
}

bool MetronomeState::isToggleSelected(uint8_t channel) const {
    return navLevel == GLOBAL &&
           menuPosition == static_cast<MenuPosition>(MENU_CH1_TOGGLE + channel * 3);
}

bool MetronomeState::isLengthSelected(uint8_t channel) const {
    return navLevel == GLOBAL &&
           menuPosition == static_cast<MenuPosition>(MENU_CH1_LENGTH + channel * 3);
}

bool MetronomeState::isPatternSelected(uint8_t channel) const {
    return navLevel == GLOBAL &&
           menuPosition == static_cast<MenuPosition>(MENU_CH1_PATTERN + channel * 3);
}

float MetronomeState::getProgress() const {
    if (!isRunning)
        return 0.0f;

    return float(globalTick % getTotalBeats()) / getTotalBeats();
}

uint32_t MetronomeState::getTotalBeats() const {
    uint32_t result = channels[0].getBarLength();
    for (uint8_t i = 1; i < CHANNEL_COUNT; i++) {
        result = lcm(result, channels[i].getBarLength());
    }
    return result;
}

float MetronomeState::getEffectiveBpm() const {
    return bpm * multiplierValues[currentMultiplierIndex];
}

const char *MetronomeState::getCurrentMultiplierName() const {
    return multiplierNames[currentMultiplierIndex];
}

float MetronomeState::getCurrentMultiplier() const {
    return multiplierValues[currentMultiplierIndex];
}

void MetronomeState::adjustMultiplier(int8_t delta) {
    currentMultiplierIndex = (currentMultiplierIndex + MULTIPLIER_COUNT + delta) % MULTIPLIER_COUNT;
} 