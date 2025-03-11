# Hardware Documentation

## Required Components

- ESP32 Development Board
- SH1106 128x64 OLED Display (I2C)
- Rotary Encoder with push button
- 2 Control buttons (Start/Stop)
- IRLZ44N N-channel MOSFET
- 5V Solenoid actuator

## Pin Assignments

| Component      | Pin | Description    |
| -------------- | --- | -------------- |
| Display SDA    | 21  | I2C Data       |
| Display SCL    | 22  | I2C Clock      |
| Encoder A      | 17  | Rotary Encoder |
| Encoder B      | 18  | Rotary Encoder |
| Encoder Button | 16  | Push Button    |
| Start Button   | 26  | Control        |
| Stop Button    | 33  | Control        |
| Solenoid       | 13  | MOSFET Gate    |

## Solenoid Circuit

```
                  5V
                   │
                   ├──┬──────┐
                   │  │      │
               100nF  │ 470µF│
                   │  │      │
                   │  │      │
         ┌─────────┴──┴──────┤
         │      SOLENOID     │
         │                   │
         └──────────┬────────┘
                    │    ┌─── 1N4007
                    ├────┤
                    │    └───┐
                    │        │
 ESP32 ──┐          │        │
       10kΩ         │        │
          └───────┤ │        │
               IRLZ44N       │
                    │        │
                    └────────┘
                    GND
```

### Components List

- 5V Solenoid (1A peak current)
- IRLZ44N N-channel MOSFET
- 1N4007 Flyback diode
- 470µF electrolytic capacitor
- 100nF ceramic capacitor
- 10kΩ pull-down resistor

The parallel capacitors manage current spikes:

- 470µF electrolytic: Main power reservoir
- 100nF ceramic: High-frequency transient suppression
- Flyback diode: Protects against back-EMF
- Pull-down resistor: Ensures MOSFET off state
