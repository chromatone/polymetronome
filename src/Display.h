#pragma once
#include <U8g2lib.h>
#include <Ticker.h>
#include "MetronomeState.h"
#include "MetronomeChannel.h"

class Display
{
private:
    U8G2_SH1106_128X64_NONAME_F_HW_I2C *display;
    
    // Animation timing variables
    uint32_t animationTick = 0;
    Ticker animationTicker;
    
    static Display* _instance;
    static void animationTickerCallback();

    void drawGlobalRow(const MetronomeState &state);
    void drawGlobalProgress(const MetronomeState &state);
    void drawChannelBlock(const MetronomeState &state, uint8_t channelIndex, uint8_t y);
    void drawBeatGrid(uint8_t x, uint8_t y, const MetronomeChannel &ch, bool isEditing);
    void drawFlash();

public:
    Display();
    ~Display(); // Added destructor to prevent memory leaks
    void begin();
    void update(const MetronomeState &state);
    
    // Animation control
    void startAnimation();
    void stopAnimation();
    uint32_t getAnimationTick() const { return animationTick; }
};