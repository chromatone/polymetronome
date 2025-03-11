#include "Display.h"
#include "config.h"

// Initialize static member
Display* Display::_instance = nullptr;

Display::Display()
{
    display = new U8G2_SH1106_128X64_NONAME_F_HW_I2C(U8G2_R0, U8X8_PIN_NONE);
    _instance = this;
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
}

void Display::stopAnimation()
{
    animationTicker.detach();
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

    if (state.isRunning)
    {
        drawFlash();
    }

    display->sendBuffer();
}

void Display::drawGlobalRow(const MetronomeState &state)
{
    char buffer[32];

    // BPM display with selection frame
    sprintf(buffer, "%d BPM", state.bpm);
    if (state.isBpmSelected())
    {
        display->drawFrame(1, 1, 45, 12);
        if (state.isEditing)
        {
            display->drawBox(1, 1, 45, 12);
            display->setDrawColor(0);
        }
    }
    display->drawStr(3, 11, buffer);
    display->setDrawColor(1);

    // Multiplier display
    sprintf(buffer, "x%s", state.getCurrentMultiplierName());
    if (state.isMultiplierSelected())
    {
        display->drawFrame(49, 1, 30, 12);
        if (state.isEditing)
        {
            display->drawBox(49, 1, 30, 12);
            display->setDrawColor(0);
        }
    }
    display->drawStr(51, 11, buffer);
    display->setDrawColor(1);

    // Beat counter on the right
    uint32_t totalBeats = state.getTotalBeats();
    uint32_t currentBeat = (state.globalTick % totalBeats) + 1;
    sprintf(buffer, "%lu/%lu", currentBeat, totalBeats);
    display->drawStr(86, 11, buffer);
}

void Display::drawGlobalProgress(const MetronomeState &state)
{
    if (!state.isRunning && !state.isPaused)
        return;

    float progress = float(state.globalTick % state.getTotalBeats() + 1) / (state.getTotalBeats());

    uint8_t width = uint8_t(progress * (SCREEN_WIDTH - 2));
    display->drawBox(1, 14, width, 2);
}

void Display::drawChannelBlock(const MetronomeState &state, uint8_t channelIndex, uint8_t y)
{
    const MetronomeChannel &channel = state.getChannel(channelIndex);
    char buffer[32];

    // Length display with blinking effect when active
    bool isActive = channel.isEnabled();
    bool shouldBlink = false;

    if (isActive && state.isRunning)
    {
        // Determine if we should blink based on beat state and animation tick
        BeatState beatState = channel.getBeatState();
        uint32_t blinkDurationTicks = 0;

        switch (beatState)
        {
        case ACCENT:
            // Convert ms to animation ticks (20ms per tick)
            blinkDurationTicks = (ACCENT_PULSE_MS * 4) / 20;
            break;
        case WEAK:
            // Convert ms to animation ticks (20ms per tick)
            blinkDurationTicks = (SOLENOID_PULSE_MS * 4) / 20;
            break;
        default:
            blinkDurationTicks = 0;
            break;
        }

        // Calculate how many ticks have passed since the last beat
        uint32_t beatTick = state.globalTick * (1000 / (state.getEffectiveBpm() / 60));
        uint32_t ticksSinceLastBeat = animationTick - beatTick;
        
        shouldBlink = (ticksSinceLastBeat < blinkDurationTicks);
    }

    // Length row
    sprintf(buffer, "%02d", channel.getBarLength());
    bool isLengthSelected = state.isLengthSelected(channelIndex);

    // Box for length
    uint8_t boxWidth = 20;
    uint8_t boxX = 1; // Adjusted for border

    if (isLengthSelected)
    {
        display->drawFrame(1, y - 1, 126, 12); // Adjusted for border
        if (state.isEditing)
        {
            display->drawBox(1, y - 1, 126, 12);
            display->setDrawColor(0);
        }
    }

    // Fixed blinking logic - always draw the text first
    display->drawStr(boxX + 2, y + 8, buffer);

    // Handle blinking effect - redraw with inverted colors if needed
    if (shouldBlink)
    {
        display->drawBox(boxX, y - 1, boxWidth, 12);
        display->setDrawColor(0);                  // White text on black
        display->drawStr(boxX + 2, y + 8, buffer); // Redraw text in inverted color
        display->setDrawColor(1);                  // Back to normal
    }

    // Add pattern counter (current/total)
    uint16_t currentPattern = channel.getPattern() + 1;
    uint16_t maxPattern = channel.getMaxPattern() + 1;
    sprintf(buffer, "%u/%u", currentPattern, maxPattern);
    display->drawStr(85, y + 8, buffer);

    display->setDrawColor(1);

    // Pattern row
    uint8_t patternY = y + 11;
    bool isPatternSelected = state.isPatternSelected(channelIndex);

    if (isPatternSelected)
    {
        display->drawFrame(1, patternY, 126, 10); // Adjusted for border
        if (state.isEditing)
        {
            display->drawBox(1, patternY, 126, 10);
            display->setDrawColor(0);
        }
    }

    drawBeatGrid(2, patternY + 1, channel, false);
    display->setDrawColor(1);
}

void Display::drawBeatGrid(uint8_t x, uint8_t y, const MetronomeChannel &ch, bool isEditing)
{
    uint8_t barLength = ch.getBarLength();
    uint8_t cellWidth = (barLength > 0) ? (124 / barLength) : 0;

    if (cellWidth == 0)
        return; // Safety check

    for (uint8_t i = 0; i < barLength; i++)
    {
        uint8_t cellX = x + (i * cellWidth);
        bool isCurrentBeat = (i == ch.getCurrentBeat());
        bool isBeatActive = ch.getPatternBit(i);

        if (isEditing && i == ch.getEditStep())
        {
            display->drawFrame(cellX, y, cellWidth - 1, 8);
        }

        if (isBeatActive)
        {
            if (isCurrentBeat && ch.isEnabled())
            {
                display->drawBox(cellX + 2, y + 2, cellWidth - 5, 4);
            }
            else
            {
                display->drawDisc(cellX + cellWidth / 2, y + 4, 2);
            }
        }
        else if (isCurrentBeat && ch.isEnabled())
        {
            // Pulsing dot for silent active beat
            float pulse = (millis() % 500) / 500.0f; // 0.0 to 1.0 over 500ms
            uint8_t radius = 1 + uint8_t(pulse);
            display->drawDisc(cellX + cellWidth / 2, y + 4, radius);
        }
        else
        {
            display->drawPixel(cellX + cellWidth / 2, y + 4);
        }
    }
}

void Display::drawFlash()
{
    static const uint32_t FLASH_DURATION_TICKS = 2;  // 50ms at 20ms per tick
    static const uint32_t FLASH_INTERVAL_TICKS = 25; // 500ms at 20ms per tick

    // Calculate flash effect based on animation tick
    uint32_t flashCycle = animationTick % FLASH_INTERVAL_TICKS;
    
    // Draw flash effect if within the flash duration
    if (flashCycle < FLASH_DURATION_TICKS)
    {
        display->drawFrame(0, 0, 128, 64);
    }
}