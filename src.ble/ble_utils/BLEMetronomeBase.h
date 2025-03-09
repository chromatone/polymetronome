#ifndef BLE_METRONOME_BASE_H
#define BLE_METRONOME_BASE_H

#include <NimBLEDevice.h>
#include <functional>
#include "ProtocolMidi.h" 



class BLEMetronomeBase : public ProtocolMidi {
public:
    virtual void begin(const std::string deviceName);
    void end();
    bool isConnected();

protected:
    std::string deviceName;
    bool connected = false;
    const std::string MIDI_SERVICE_UUID = "03b80e5a-ede8-4b33-a751-6ce34ec4c700";
    const std::string MIDI_CHARACTERISTIC_UUID = "7772e5db-3868-4112-a1a9-f2669d106bf3";
  

};


#endif