#include "MetronomeChannel.h"

MetronomeChannel::MetronomeChannel(uint8_t channelId)
    : id(channelId), barLength(4), pattern(0), multiplier(1.0), currentBeat(0),
      enabled(true), lastBeatTime(0), editing(false), editStep(0), beatProgress(0.0f) {}

void MetronomeChannel::update(uint32_t globalBpm, uint32_t globalTick) {
    if (!enabled)
        return;
    currentBeat = globalTick % barLength;
    lastBeatTime = globalTick;
}

BeatState MetronomeChannel::getBeatState() const {
    if (!enabled)
        return SILENT;
    uint16_t fullPattern = (pattern << 1) | 1; // First beat always on
    if (currentBeat == 0) {
        return ACCENT;
    }
    return (fullPattern >> currentBeat) & 1 ? WEAK : SILENT;
}

void MetronomeChannel::toggleBeat(uint8_t step) {
    if (step == 0)
        return; // Can't toggle first beat
    uint16_t mask = 1 << step;
    pattern ^= mask;
}

void MetronomeChannel::generateEuclidean(uint8_t activeBeats) {
    // Constrain active beats to be at least 1 and at most barLength
    activeBeats = constrain(activeBeats, 1, barLength);
    
    // Reset pattern
    pattern = 0;
    
    // Debug output
    Serial.print("Generating Euclidean rhythm: ");
    Serial.print(activeBeats);
    Serial.print(" beats in ");
    Serial.print(barLength);
    Serial.println(" positions");
    
    // If we only have one active beat, it should be the first beat
    if (activeBeats <= 1) {
        // First beat is always active and not stored in pattern
        return; // pattern remains 0
    }
    
    // Proper Bjorklund's algorithm implementation
    // Calculate the number of beats per group and remainder
    uint8_t beatsPerGroup = barLength / activeBeats;
    uint8_t remainder = barLength % activeBeats;
    
    // Create a temporary pattern that includes all positions
    uint16_t fullPattern = 0;
    uint8_t position = 0;
    
    // Place beats with spacing
    for (uint8_t i = 0; i < activeBeats; i++) {
        // Set the bit at the current position
        fullPattern |= (1 << position);
        
        // Move position by beats per group plus 1 if we're in the remainder
        position += beatsPerGroup + (i < remainder ? 1 : 0);
    }
    
    // Debug output
    Serial.print("Full pattern: 0b");
    for (int8_t i = barLength - 1; i >= 0; i--) {
        Serial.print((fullPattern >> i) & 1);
    }
    Serial.println();
    
    // Now extract the pattern excluding the first beat
    // First, ensure the first beat is active in our pattern
    if (!(fullPattern & 1)) {
        // If the first beat isn't active in the Euclidean pattern,
        // we need to rotate the pattern to make it active
        uint8_t firstActivePos = 0;
        for (uint8_t i = 0; i < barLength; i++) {
            if (fullPattern & (1 << i)) {
                firstActivePos = i;
                break;
            }
        }
        
        // Rotate the pattern to make the first active beat be at position 0
        fullPattern = ((fullPattern >> firstActivePos) | (fullPattern << (barLength - firstActivePos))) & ((1 << barLength) - 1);
    }
    
    // Now extract the pattern excluding the first beat
    pattern = (fullPattern >> 1);
    
    // Debug output
    Serial.print("Final pattern: 0b");
    for (int8_t i = barLength - 2; i >= 0; i--) {
        Serial.print((pattern >> i) & 1);
    }
    Serial.println();
}

uint8_t MetronomeChannel::getId() const { return id; }
uint8_t MetronomeChannel::getBarLength() const { return barLength; }
uint16_t MetronomeChannel::getPattern() const { return pattern; }
float MetronomeChannel::getMultiplier() const { return multiplier; }
uint8_t MetronomeChannel::getCurrentBeat() const { return currentBeat; }
bool MetronomeChannel::isEnabled() const { return enabled; }
bool MetronomeChannel::isEditing() const { return editing; }
uint8_t MetronomeChannel::getEditStep() const { return editStep; }

void MetronomeChannel::setBarLength(uint8_t length) {
    barLength = constrain(length, 1, 16);
    pattern &= ((1 << barLength) - 1);
}

void MetronomeChannel::setPattern(uint16_t pat) { pattern = pat; }
void MetronomeChannel::setMultiplier(float mult) { multiplier = mult; }
void MetronomeChannel::toggleEnabled() { enabled = !enabled; }
void MetronomeChannel::setEditing(bool edit) { editing = edit; }

void MetronomeChannel::setEditStep(uint8_t step) {
    editStep = step % barLength;
}

bool MetronomeChannel::getPatternBit(uint8_t position) const {
    if (!enabled)
        return false;
    uint16_t fullPattern = (pattern << 1) | 1; // First beat always on
    return (fullPattern >> position) & 1;
}

float MetronomeChannel::getProgress(uint32_t currentTime, uint32_t globalBpm) const {
    if (!enabled || !lastBeatTime)
        return 0.0f;
    uint32_t beatInterval = 60000 / (globalBpm * multiplier);
    return float(currentTime - lastBeatTime) / beatInterval;
}

uint16_t MetronomeChannel::getMaxPattern() const {
    if (barLength >= 16)
        return 32767;                     // Max 15-bit value
    return ((1 << barLength) - 1) >> 1;   // (2^length - 1) / 2, first bit always 1
}

void MetronomeChannel::updateProgress(uint32_t globalTick) {
    if (!enabled)
        return;
    beatProgress = (globalTick % 1) / 1.0f;
}

void MetronomeChannel::updateBeat(uint32_t globalTick) {
    if (!enabled)
        return;
    currentBeat = globalTick % barLength;
}

float MetronomeChannel::getProgress() const {
    return enabled ? beatProgress : 0.0f;
}

void MetronomeChannel::resetBeat() {
    currentBeat = 0;
    lastBeatTime = 0;
    beatProgress = 0.0f;
} 