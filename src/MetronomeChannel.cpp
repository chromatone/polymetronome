#include "MetronomeChannel.h"
#include "WirelessSync.h"
#include "MetronomeState.h"

MetronomeChannel::MetronomeChannel(uint8_t channelId)
    : id(channelId), barLength(4), pattern(0), multiplier(1.0), currentBeat(0),
      enabled(channelId == 0), lastBeatTime(0), editing(false), editStep(0), beatProgress(0.0f),
      lastTriggeredBeatPosition(UINT32_MAX), lastTriggeredTick(0) {}

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
    // Notify pattern change
    if (globalWirelessSync) {
        globalWirelessSync->notifyPatternChanged(id);
    }
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
    if (length > 0 && length <= MAX_BEATS) {
        barLength = length;
        // Notify pattern change since this affects pattern playback
        if (globalWirelessSync) {
            globalWirelessSync->notifyPatternChanged(id);
        }
    }
}

void MetronomeChannel::setPattern(uint16_t pat) {
    pattern = pat;
    // Notify pattern change
    if (globalWirelessSync) {
        globalWirelessSync->notifyPatternChanged(id);
    }
}

void MetronomeChannel::setMultiplier(float mult) { multiplier = mult; }
void MetronomeChannel::toggleEnabled() {
    enabled = !enabled;
    // Notify pattern change since this affects pattern playback
    if (globalWirelessSync) {
        globalWirelessSync->notifyPatternChanged(id);
    }
}
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
    lastTriggeredBeatPosition = UINT32_MAX;
    lastTriggeredTick = 0;
}

// New methods for polyrhythm mode

void MetronomeChannel::updatePolyrhythmBeat(uint32_t masterTick, uint8_t ch1Length, uint8_t ch2Length) {
    if (!enabled || ch1Length == 0 || ch2Length == 0)
        return;
    
    // Handle channel 1 normally (uses the global masterTick)
    if (id == 0) {
        currentBeat = masterTick % barLength;
        lastBeatTime = masterTick;
        return;
    } 
    
    // For channel 2, calculate based on the ratio of the two channels
    // The goal is to distribute ch2Length beats evenly throughout the ch1Length cycle
    
    // Get position in the current cycle (0 to ch1Length-1)
    uint32_t cyclePosition = masterTick % ch1Length;
    
    // Calculate the ratio of beats between the two channels
    float ratio = float(ch2Length) / float(ch1Length);
    
    // Calculate the exact fractional position in channel 2's pattern
    float exactPosition = cyclePosition * ratio;
    
    // Convert to an integer beat position, ensuring we don't exceed the bar length
    currentBeat = uint8_t(exactPosition) % barLength;
    
    // Store the last beat time
    lastBeatTime = masterTick;
}

BeatState MetronomeChannel::getPolyrhythmBeatState(uint32_t ppqnTick, const MetronomeState& state) const {
    // Basic checks
    if (!enabled)
        return SILENT;
    
    // Channel 1 is processed normally
    if (id == 0) {
        return getBeatState();
    }
    
    // For channel 2, determine if this PPQN tick should trigger a beat
    uint8_t ch1Length = state.getChannel(0).getBarLength();
    uint8_t ch2Length = barLength;
    
    // Skip calculation if either channel length is 0
    if (ch1Length == 0 || ch2Length == 0)
        return SILENT;
    
    // Calculate effective tick based on multiplier
    float multiplier = state.getCurrentMultiplier();
    uint32_t effectiveTick = uint32_t(ppqnTick * multiplier);
    
    // Calculate the total ticks in one bar of channel 1
    // PPQN_96 means 96 ticks per quarter note
    uint32_t totalTicksInBar = ch1Length * 96;
    
    // Calculate how many ticks per beat for channel 2
    // We need to distribute ch2Length beats evenly across totalTicksInBar ticks
    float ticksPerBeat = float(totalTicksInBar) / float(ch2Length);
    
    // Get the position within the current bar
    uint32_t tickInBar = effectiveTick % totalTicksInBar;
    
    // Calculate the exact beat position using floating point
    float exactBeatPosition = float(tickInBar) / ticksPerBeat;
    
    // Get the integer beat position and fractional part
    uint32_t beatPosition = uint32_t(exactBeatPosition);
    float fractionalPart = exactBeatPosition - beatPosition;
    
    // Define tolerance for beat detection (0.05 = 5% of a beat)
    float tolerance = 0.05f;
    
    // If we're close to a beat boundary (within tolerance)
    if (fractionalPart < tolerance || fractionalPart > (1.0f - tolerance)) {
        // Ensure we don't exceed the bar length
        beatPosition = beatPosition % ch2Length;
        
        // IMPORTANT: Check if this is a new beat position to prevent double triggers
        // Only trigger if this is a different beat than the last one we processed
        
        // Only trigger if:
        // 1. This is a different beat position than the last one we processed, OR
        // 2. We've gone through a complete bar (tickInBar < last tickInBar)
        bool shouldTrigger = (beatPosition != lastTriggeredBeatPosition) || 
                             (ppqnTick - lastTriggeredTick > totalTicksInBar/2);
        
        if (shouldTrigger) {
            // Update the last triggered position and tick
            // We need to cast away const here because we're modifying member variables
            // This is safe because we're not changing the logical state of the object
            MetronomeChannel* nonConstThis = const_cast<MetronomeChannel*>(this);
            nonConstThis->lastTriggeredBeatPosition = beatPosition;
            nonConstThis->lastTriggeredTick = ppqnTick;
            
            // Return appropriate beat state based on pattern
            if (beatPosition == 0) {
                return ACCENT; // First beat is always accented
            } else if ((pattern >> (beatPosition - 1)) & 1) {
                return WEAK;   // Other beats follow the pattern
            }
        }
    }
    
    return SILENT;
} 