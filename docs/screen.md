# Screen Layout Documentation

## Display Organization

The 128x64 OLED screen is divided into three main sections:

1. Global Row (0-10px)
   - BPM display with selection frame
   - Multiplier display
   - Beat counter
2. Global Progress (11px)
   - Single pixel progress bar
3. Channel Blocks (13-63px)
   - Channel 1: 13-34px
   - Channel 2: 35-56px

## Layout Example

```
┌──────────────────┐
│120 BPM x1    5/12│
│|**--------------|│
│(X) 04       5/7  │
│[ X ][ X ][ ][ x ]│
│(X) 03       1/4  │
│[  X  ][   ][    ]│
└──────────────────┘
```

## Visual Elements

### Selection Frames

- Outlined box for selected parameter
- Filled box for editing mode
- Inverted colors for active editing

### Beat Indicators

- Filled circle: Active beat
- Empty circle: Inactive beat
- Pulsing dot: Current position
- Frame: Selected step in edit mode

### Progress Indicators

- Global bar: Shows position in total cycle
- Click indicators: Shows beat state
- Channel progress: Shows position in pattern
