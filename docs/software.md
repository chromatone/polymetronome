# Software Architecture

## Core Classes

### MetronomeState

Central state management handling:

- Global BPM (20-500)
- Tempo multipliers (1/4×-4×)
- Navigation state
- Channel synchronization
- Global progress tracking

### MetronomeChannel

Individual channel logic:

- Pattern storage (up to 16 steps)
- Beat states (Silent/Weak/Accent)
- Progress tracking
- Pattern generation
- Channel enable/disable

### Display

UI rendering system:

- Global progress bar
- Channel visualization
- Beat indicators
- Menu navigation
- Selection highlighting

## Code Structure

```
src/
  ├── main.cpp           // Program entry, setup, loop
  ├── config.h           // Constants and configurations
  ├── MetronomeState.h   // Global state management
  ├── MetronomeChannel.h // Channel logic
  ├── Display.h          // Display interface
  └── Display.cpp        // UI implementation
```

## Navigation Hierarchy

1. Global Level (editLevel = GLOBAL)

   - BPM control (20-500)
   - Multiplier selection
   - Channel selection

2. Pattern Level
   - Step-by-step beat editing
   - Pattern generation
   - Channel enable/disable
