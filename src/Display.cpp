#include "Display.h"
#include "config.h"

// Initialize static member
Display *Display::_instance = nullptr;

Display::Display()
{
    display = new U8G2_SH1106_128X64_NONAME_F_HW_I2C(U8G2_R0, U8X8_PIN_NONE);
    _instance = this;
    animationRunning = false;
}

Display::~Display()
{
    stopAnimation();
    if (_instance == this)
    {
        _instance = nullptr;
    }
    if (display)
    {
        delete display;
    }
}

void Display::animationTickerCallback()
{
    if (_instance)
    {
        _instance->animationTick++;
    }
}

void Display::begin()
{
    display->begin();
    display->setFont(u8g2_font_t0_11_tr);
}

void Display::startAnimation()
{
    animationTicker.detach();
    animationTick = 0;
    // Update animation at 20ms intervals (50Hz)
    animationTicker.attach(0.02, animationTickerCallback);
    animationRunning = true;
}

void Display::stopAnimation()
{
    animationTicker.detach();
    animationRunning = false;
}

void Display::update(const MetronomeState &state)
{
    display->clearBuffer();

    display->drawFrame(0, 0, 128, 64);

    drawGlobalRow(state);
    drawGlobalProgress(state);

    display->drawHLine(1, 17, 126);

    drawChannelBlock(state, 0, 19);

    display->drawHLine(1, 40, 126);

    drawChannelBlock(state, 1, 42);

    display->sendBuffer();
}

void Display::drawGlobalRow(const MetronomeState &state)
{
    char buffer[32];

    // Global beat indicator (4px wide block on the left)
    if (state.isRunning)
    {
        // Calculate flash duration based on BPM and multiplier
        float beatDuration = 60000.0f / state.getEffectiveBpm();           // in milliseconds
        float flashDuration = beatDuration * 0.5f;                         // 25% of beat duration
        uint32_t animTime = (animationTick * 20) % uint32_t(beatDuration); // 20ms per animation tick

        if (animTime < flashDuration)
        {
            display->drawBox(1, 1, 4, 12);
        }
    }
    else if (state.isPaused)
    {
        // Show pause indicator (two vertical bars)
        display->drawBox(3, 3, 1, 8);
        display->drawBox(6, 3, 1, 8);
    }

    // BPM display with selection frame
    sprintf(buffer, "%d BPM", state.bpm);
    if (state.isBpmSelected())
    {
        display->drawFrame(7, 1, 45, 12);
        if (state.isEditing)
        {
            display->drawBox(7, 1, 45, 12);
            display->setDrawColor(0);
        }
    }
    display->drawStr(9, 11, buffer);
    display->setDrawColor(1);

    // Multiplier display
    sprintf(buffer, "x%s", state.getCurrentMultiplierName());
    if (state.isMultiplierSelected())
    {
        display->drawFrame(55, 1, 16, 12);
        if (state.isEditing)
        {
            display->drawBox(55, 1, 16, 12);
            display->setDrawColor(0);
        }
    }
    display->drawStr(57, 11, buffer);
    display->setDrawColor(1);
    
    // Rhythm mode toggle (+ for polymeter, ÷ for polyrhythm)
    if (state.isRhythmModeSelected())
    {
        display->drawFrame(74, 1, 12, 12);
        if (state.isEditing)
        {
            display->drawBox(74, 1, 12, 12);
            display->setDrawColor(0);
        }
    }
    
    // Draw the mode symbol
    if (state.isPolyrhythm()) {
        // Draw division symbol (÷) with primitives
        // Horizontal line
        display->drawHLine(76, 6, 8);
        // Top dot
        display->drawCircle(79, 3, 1);
        // Bottom dot
        display->drawCircle(79, 9, 1);
    } else {
        // Draw plus symbol (+)
        display->drawStr(76, 11, "+");
    }
    display->setDrawColor(1);

    // Beat counter on the right
    uint32_t totalBeats = state.getTotalBeats();
    float currentPosition = float(state.globalTick % totalBeats) + state.tickFraction;
    uint32_t currentBeat = uint32_t(currentPosition) + 1; // Add 1 for 1-based counting
    sprintf(buffer, "%lu/%lu", currentBeat, totalBeats);
    display->drawStr(92, 11, buffer);
}

void Display::drawGlobalProgress(const MetronomeState &state)
{
    if (!state.isRunning && !state.isPaused)
        return;

    // Get the smooth progress value (includes fractional part)
    float progress = state.getProgress();

    // Calculate the width of the progress bar
    uint8_t width = uint8_t(progress * (SCREEN_WIDTH - 2));
    
    // Draw a solid progress bar when running, dashed when paused
    if (state.isPaused) {
        // Draw dashed progress bar when paused
        for (uint8_t x = 1; x < width; x += 4) {
            uint8_t dashWidth = min(2, width - x);
            display->drawBox(x, 14, dashWidth, 2);
        }
    } else {
        // Draw solid progress bar when running
        display->drawBox(1, 14, width, 2);
    }
}

void Display::drawChannelBlock(const MetronomeState &state, uint8_t channelIndex, uint8_t y)
{
    const MetronomeChannel &channel = state.getChannel(channelIndex);
    char buffer[32];

    // Channel beat indicator (4px wide block on the left)
    if (channel.isEnabled() && state.isRunning)
    {
        BeatState beatState;
        
        // In polyrhythm mode, use the appropriate beat state calculation
        if (state.isPolyrhythm() && channelIndex == 1) {
            // For channel 2 in polyrhythm mode, check if we're on a beat boundary
            beatState = channel.getPolyrhythmBeatState(state.lastPpqnTick, state);
        } else {
            // Normal beat state calculation
            beatState = channel.getBeatState();
        }
        
        if (beatState != SILENT)
        {
            // Calculate flash duration - shorter than half the beat length
            float beatDuration;
            
            if (state.isPolyrhythm() && channelIndex == 1) {
                // For channel 2 in polyrhythm mode, calculate the beat duration based on the ratio
                uint8_t ch1Length = state.getChannel(0).getBarLength();
                uint8_t ch2Length = channel.getBarLength();
                float ratio = float(ch1Length) / float(ch2Length);
                beatDuration = 60000.0f / (state.getEffectiveBpm() * ratio);
            } else {
                // Normal beat duration calculation
                beatDuration = 60000.0f / state.getEffectiveBpm();
            }
            
            float flashDuration = beatDuration * 0.5f;                         // 50% of beat duration
            uint32_t animTime = (animationTick * 20) % uint32_t(beatDuration); // 20ms per animation tick

            if (animTime < flashDuration)
            {
                display->drawBox(1, y - 1, 4, 12); // Only the height of the upper row
            }
        }
    }

    // Draw channel toggle (on/off)
    bool isToggleSelected = state.isToggleSelected(channelIndex);
    if (isToggleSelected)
    {
        display->drawFrame(7, y - 1, 16, 12);
        if (state.isEditing)
        {
            display->drawBox(7, y - 1, 16, 12);
            display->setDrawColor(0);
        }
    }
    
    // Draw toggle circle
    if (channel.isEnabled()) {
        display->drawDisc(14, y + 5, 3); // Filled circle when enabled
    } else {
        display->drawCircle(14, y + 5, 3); // Empty circle when disabled
    }
    display->setDrawColor(1);

    // Length row
    sprintf(buffer, "%02d", channel.getBarLength());
    bool isLengthSelected = state.isLengthSelected(channelIndex);

    // Box for length (shifted right to make room for toggle)
    if (isLengthSelected)
    {
        display->drawFrame(25, y - 1, 16, 12);
        if (state.isEditing)
        {
            display->drawBox(25, y - 1, 16, 12);
            display->setDrawColor(0);
        }
    }

    // Draw length text
    display->drawStr(27, y + 8, buffer);
    display->setDrawColor(1);

    // Add pattern counter (current/total)
    uint16_t currentPattern = channel.getPattern() + 1;
    uint16_t maxPattern = channel.getMaxPattern() + 1;
    sprintf(buffer, "%u/%u", currentPattern, maxPattern);
    display->drawStr(91, y + 8, buffer);

    // Pattern row
    uint8_t patternY = y + 11;
    bool isPatternSelected = state.isPatternSelected(channelIndex);

    if (isPatternSelected)
    {
        display->drawFrame(1, patternY, 126, 10);
        if (state.isEditing)
        {
            display->drawBox(1, patternY, 126, 10);
            display->setDrawColor(0);
        }
    }
    display->drawHLine(1, patternY, 126);
    
    // Show Euclidean rhythm applied message
    if (state.euclideanApplied && state.isPatternSelected(channelIndex)) {
        // Show message for 2 seconds
        if (millis() - state.euclideanAppliedTime < LONG_PRESS_DURATION_MS) {
            display->setDrawColor(0);
            display->drawBox(30, patternY + 1, 70, 8);
            display->setDrawColor(1);
            display->drawStr(32, patternY + 8, "EUCLIDEAN");
            
            // Skip drawing the beat grid
            display->setDrawColor(1);
            return;
        } else {
            // Clear the flag after 2 seconds
            MetronomeState &mutableState = const_cast<MetronomeState&>(state);
            mutableState.euclideanApplied = false;
        }
    }
    
    // Get max length for visualization, depends on rhythm mode
    uint8_t maxLength;
    
    if (state.isPolyrhythm()) {
        // In polyrhythm mode, each pattern takes the full width
        maxLength = channel.getBarLength();
    } else {
        // In polymeter mode, use max length between channels for consistent visualization
        maxLength = max(state.getChannel(0).getBarLength(), state.getChannel(1).getBarLength());
    }
    
    drawBeatGrid(2, patternY + 1, channel, maxLength, state.isPolyrhythm(), state);
    display->setDrawColor(1);
}

void Display::drawBeatGrid(uint8_t x, uint8_t y, const MetronomeChannel &ch, uint8_t maxLength, bool isPolyrhythm, const MetronomeState &state)
{
    uint8_t barLength = ch.getBarLength();
    uint8_t cellWidth;
    
    if (isPolyrhythm) {
        // For polyrhythm, each channel's grid should take the full width
        cellWidth = (126 / barLength); 
    } else {
        // For polymeter, maintain consistent cell width based on max length
        cellWidth = (maxLength > 0) ? (126 / maxLength) : 0;
    }

    if (cellWidth == 0)
        return; // Safety check

    // Calculate how far to draw (either this channel's bar length or the max length, depending on mode)
    uint8_t drawLength = isPolyrhythm ? barLength : min(barLength, maxLength);

    // Draw only up to this channel's bar length
    for (uint8_t i = 0; i < drawLength; i++)
    {
        uint8_t cellX = x + (i * cellWidth);
        
        // Get beat position, which could be different in polyrhythm mode
        uint8_t beatPosition = i;
        
        // Check if this is the current beat or the next beat for smooth transition
        uint8_t currentBeat = ch.getCurrentBeat();
        bool isCurrentBeat = (i == currentBeat);
        
        // In polyrhythm mode, we need to highlight the current beat position correctly
        if (isPolyrhythm && ch.getId() == 1) {
            isCurrentBeat = (i == currentBeat);
        }
        
        bool isBeatActive = ch.getPatternBit(i);

        if (ch.isEditing() && i == ch.getEditStep())
        {
            display->drawFrame(cellX, y, cellWidth - 1, 8);
        }

        display->drawVLine(cellX - 1, y, 10);

        if (i == drawLength - 1) {
            display->drawVLine(cellX + cellWidth - 1, y, 10); // Closing vertical line for the last beat
        }

        if (isBeatActive)
        {
            if (isCurrentBeat && ch.isEnabled())
            {
                display->drawBox(cellX + 1, y + 1, cellWidth - 3, 7);
            }
            else
            {
                display->drawDisc(cellX + cellWidth / 2 - 1, y + 4, 2);
            }
        }
        else if (isCurrentBeat && ch.isEnabled())
        {
            // Pulsing dot for silent active beat
            float pulse = (millis() % 500) / 500.0f; // 0.0 to 1.0 over 500ms
            uint8_t radius = 1 + uint8_t(pulse);
            display->drawDisc(cellX + cellWidth / 2 - 1, y + 4, radius);
        }
        else
        {
            display->drawPixel(cellX + cellWidth / 2 - 1, y + 4);
        }
    }
}
