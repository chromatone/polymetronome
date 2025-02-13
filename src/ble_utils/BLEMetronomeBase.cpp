#include <Arduino.h>
#include "BLEMetronomeBase.h"

void BLEMetronomeBase::begin(const std::string deviceName)
{
    this->deviceName = deviceName;
    BLEDevice::init(deviceName);
}

void BLEMetronomeBase::end() {
    BLEDevice::deinit();
}

bool BLEMetronomeBase::isConnected()
{
    return connected;
}