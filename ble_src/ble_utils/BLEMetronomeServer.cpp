#include "BLEMetronomeServer.h"

void BLEMetronomeServerClass::begin(const std::string deviceName)
{
    BLEMetronomeBase::begin(deviceName);
    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(this);
    BLEAdvertising *pAdvertising = pServer->getAdvertising();

    /* -------------------------------------------------------------------------- */
    /*                       MIDI SERVICE AND CHARESTERISTIC                      */
    /* -------------------------------------------------------------------------- */
    BLEService *pServiceMidi = pServer->createService(BLEUUID(MIDI_SERVICE_UUID));
    pCharacteristic = pServiceMidi->createCharacteristic(
        BLEUUID(MIDI_CHARACTERISTIC_UUID),
        NIMBLE_PROPERTY::READ   |
        NIMBLE_PROPERTY::WRITE  |
        NIMBLE_PROPERTY::NOTIFY |
        NIMBLE_PROPERTY::WRITE_NR
    );
    pCharacteristic->setCallbacks(new CharacteristicCallback([this](uint8_t *data, uint16_t size) { this->midi_receivePacket(data, size); }));
    pServiceMidi->start();
    pAdvertising->addServiceUUID(pServiceMidi->getUUID());
 
 
 

    pAdvertising->start();
}

void BLEMetronomeServerClass::setOnConnectCallback(void (*const onConnectCallback)())
{
    this->onConnectCallback = onConnectCallback;
}

void BLEMetronomeServerClass::setOnDisconnectCallback(void (*const onDisconnectCallback)())
{
    this->onDisconnectCallback = onDisconnectCallback;
}

bool BLEMetronomeServerClass::sendPacket(uint8_t *packet, uint16_t packetSize)
{
    if(!connected)
        return false;
    pCharacteristic->setValue(packet, packetSize);
    pCharacteristic->notify();
    return true;
} 



void BLEMetronomeServerClass::onConnect(BLEServer* pServer)
{
    connected = true;
    if(onConnectCallback != nullptr)
        onConnectCallback();
}

void BLEMetronomeServerClass::onDisconnect(BLEServer* pServer)
{
    connected = false;
    if(onDisconnectCallback != nullptr)
        onDisconnectCallback();
    pServer->startAdvertising();
}

CharacteristicCallback::CharacteristicCallback(std::function<void(uint8_t*, uint16_t)> onWriteCallback, std::function<void()> onReadCallback) : onWriteCallback(onWriteCallback), onReadCallback(onReadCallback) {}

void CharacteristicCallback::onWrite(BLECharacteristic *pCharacteristic)
{
    std::string rxValue = pCharacteristic->getValue();

    if (rxValue.length() > 0 && onWriteCallback != nullptr)
        onWriteCallback((uint8_t*)rxValue.c_str(), rxValue.length());

    vTaskDelay(0);      // We leave some time for the IDLE task call esp_task_wdt_reset_watchdog
                        // See comment from atanisoft here : https://github.com/espressif/arduino-esp32/issues/2493

}

BLEMetronomeServerClass BLEMetronomeServer;