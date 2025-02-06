# Metronome machine using Solenoid lock

Simple metronome machine using solenoid to make vibration at given bpm
by using terminal with line feed and type

```
bpm 70
```

for 70bpm etc.

This machine can achieve more than 2000bpm with tiny 5V 1A solenoid lock.

you can optimize the power of stroke 
by define the on-state time at line

```cpp
#define ON_STATE_DURATION_MS 7 // 7ms
```

## Pinout 

The pieze module can be removed. It's used for determine the time different of vibration generated after command. 

Resistor pinout
``1---20k---2``

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


## TODO
Can we lower the on-state for some cycle to make it generate lower sound?