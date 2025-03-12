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
    activeBeats = constrain(activeBeats, 1, barLength);
    pattern = 0;

    float spacing = float(barLength) / activeBeats;
    for (uint8_t i = 1; i < barLength; i++) {
        if (int(i * spacing) != int((i - 1) * spacing)) {
            pattern |= (1 << i);
        }
    }
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