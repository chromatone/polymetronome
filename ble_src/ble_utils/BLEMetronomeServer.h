#ifndef BLE_METRONOME_SERVER_H
#define BLE_METRONOME_SERVER_H

#include "BLEMetronomeBase.h"


class BLEMetronomeServerClass : public BLEMetronomeBase, public BLEServerCallbacks {
public:
    void begin(const std::string deviceName);

    void setOnConnectCallback(void (*const onConnectCallback)());
    void setOnDisconnectCallback(void (*const onDisconnectCallback)());
    


 
    

private:
    virtual bool sendPacket(uint8_t *packet, uint16_t packetSize) override;
    void onConnect(BLEServer* pServer) override;
    void onDisconnect(BLEServer* pServer) override;
    
    void (*onConnectCallback)() = nullptr;
    void (*onDisconnectCallback)() = nullptr;
    BLECharacteristic* pCharacteristic = nullptr; 

};


class CharacteristicCallback: public BLECharacteristicCallbacks {
public:
    CharacteristicCallback(std::function<void(uint8_t*, uint16_t)> onWriteCallback, std::function<void()> onReadCallback = nullptr);
private:
    void onWrite(BLECharacteristic *pCharacteristic);
    void onRead(BLECharacteristic *pCharacteristic) {
        Serial.println("CharacteristicCallback::onRead");
        if(onReadCallback != nullptr)
            onReadCallback();
    }
    std::function<void(uint8_t*, uint16_t)> onWriteCallback = nullptr;
    std::function<void()> onReadCallback = nullptr;
};


extern BLEMetronomeServerClass BLEMetronomeServer;

#endif