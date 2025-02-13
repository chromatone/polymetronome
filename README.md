# 🎵 ESP32 Polyrhythmic Metronome

An advanced ESP32-based metronome using a solenoid actuator for precise rhythm training. Features configurable BPM, time signatures, subdivisions and accent patterns.

## 🎯 Features

- Adjustable tempo (20-500 BPM)
- Variable time signatures (1-16 beats)
- Subdivisions (1/2, 1/4, 1/8 notes) 
- Configurable accent patterns
- Low latency operation via hardware interrupts
- Serial command interface

## 🛠️ Hardware Setup

### Components
- ESP32 development board
- 5V solenoid lock actuator (1A)
- N-channel MOSFET
- Optional: Piezo sensor for timing analysis
- 2x 20kΩ resistors (for piezo circuit)

### Pinout


|ESP32|20k ohm|20k ohm|PIEZO MODULE|MOSFET|SOLENOID|SUPPLY|
|---|---|---|---|---|---|---|
||1||S||||
|17|2|1|||||
|16||||TRIG|||
|GND||2|GND|GND|||
|||||VOUT+|+||
|||||VOUT-|-||
|||||VIN+||5V|
|||||VIN-||GND|


## 💻 Usage

Connect via serial terminal at 115200 baud. Available commands:

- `bpm <20-500>` - Set tempo
- `measure <1-16>` - Set beats per measure
- `pattern <n>` - Select accent pattern
- `subdivision <2,4,8>` - Set rhythm subdivision
- `help` - List all commands

## 🔧 Configuration

Key timing parameters in main.cpp:
```cpp
#define NORMAL_DURATION_MS 5    // Normal hit duration
#define ACCENT_DURATION_MS 7    // Accented hit duration
#define MIN_TRIGGER_INTERVAL_MS 100
```

📝 License
MIT License - Feel free to use and modify for your rhythm practice needs! 