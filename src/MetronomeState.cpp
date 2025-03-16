#include "MetronomeState.h"
#include "ConfigManager.h"

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
}

void MetronomeState::updateTickFraction(uint32_t ppqnTick) {
    // If paused, don't update the fraction
    if (isPaused) {
        return;
    }
    
    // Calculate effective tick based on multiplier
    float multiplier = getCurrentMultiplier();
    uint32_t effectiveTick = uint32_t(ppqnTick * multiplier);
    
    // Store the last PPQN tick
    lastPpqnTick = ppqnTick;
    
    // Calculate the fractional part of the current beat
    // PPQN_96 means 96 ticks per quarter note
    uint32_t tickInBeat = effectiveTick % 96;
    tickFraction = float(tickInBeat) / 96.0f;
}

float MetronomeState::getProgress() const {
    if (!isRunning && !isPaused)
        return 0.0f;

    uint32_t totalBeats = getTotalBeats();
    float currentPosition = float(globalTick % totalBeats) + tickFraction;
    
    return currentPosition / totalBeats;
}

uint8_t MetronomeState::getMenuItemsCount() const {
    return 9;
}

uint8_t MetronomeState::getActiveChannel() const {
    uint8_t pos = static_cast<uint8_t>(menuPosition);
    return (pos >= MENU_CH1_TOGGLE && pos <= MENU_CH1_PATTERN) ? 0 : 
           (pos >= MENU_CH2_TOGGLE && pos <= MENU_CH2_PATTERN) ? 1 : 0;
}

bool MetronomeState::isChannelSelected() const {
    return static_cast<uint8_t>(menuPosition) > MENU_RHYTHM_MODE;
}

bool MetronomeState::isBpmSelected() const {
    return navLevel == GLOBAL && menuPosition == MENU_BPM;
}

bool MetronomeState::isMultiplierSelected() const {
    return navLevel == GLOBAL && menuPosition == MENU_MULTIPLIER;
}

bool MetronomeState::isRhythmModeSelected() const {
    return navLevel == GLOBAL && menuPosition == MENU_RHYTHM_MODE;
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

uint32_t MetronomeState::getTotalBeats() const {
    if (rhythmMode == POLYMETER) {
        // In polymeter mode, use LCM of all channel bar lengths
        uint32_t result = channels[0].getBarLength();
        for (uint8_t i = 1; i < CHANNEL_COUNT; i++) {
            result = lcm(result, channels[i].getBarLength());
        }
        return result;
    } else {
        // In polyrhythm mode, first channel determines total bar length
        return channels[0].getBarLength();
    }
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

void MetronomeState::toggleRhythmMode() {
    rhythmMode = (rhythmMode == POLYMETER) ? POLYRHYTHM : POLYMETER;
}

void MetronomeState::resetBpmToDefault() {
    bpm = DEFAULT_BPM;
    
    // Debug output
    Serial.print("BPM reset to default: ");
    Serial.println(DEFAULT_BPM);
}

void MetronomeState::resetPatternsAndMultiplier() {
    // Reset multiplier to default (1.0)
    currentMultiplierIndex = 0;
    
    // Reset all channel patterns
    for (uint8_t i = 0; i < CHANNEL_COUNT; i++) {
        MetronomeChannel &channel = getChannel(i);
        
        // Reset pattern to default (only first beat active)
        channel.setPattern(0);
        
        // Reset bar length to default (4)
        channel.setBarLength(4);
    }
    
    // Reset rhythm mode to default (POLYMETER)
    rhythmMode = POLYMETER;
    
    // Debug output
    Serial.println("Patterns and multiplier reset to defaults");
}

void MetronomeState::resetChannelPattern(uint8_t channelIndex) {
    if (channelIndex < CHANNEL_COUNT) {
        MetronomeChannel &channel = getChannel(channelIndex);
        
        // Reset pattern to default (only first beat active)
        channel.setPattern(0);
        
        // Debug output
        Serial.print("Channel ");
        Serial.print(channelIndex + 1);
        Serial.println(" pattern reset to default");
    }
}

// Configuration persistence methods
bool MetronomeState::saveToStorage() {
    Serial.println("Saving configuration to storage...");
    return ConfigManager::saveConfig(*this);
}

bool MetronomeState::loadFromStorage() {
    Serial.println("Loading configuration from storage...");
    return ConfigManager::loadConfig(*this);
}

bool MetronomeState::clearStorage() {
    Serial.println("Clearing configuration storage...");
    return ConfigManager::clearConfig();
} 