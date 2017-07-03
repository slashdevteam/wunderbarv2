#include "wunderbarsensor.h"
#include "wunderbarble.h"

WunderbarSensor::WunderbarSensor(IBleGateway& _gateway,
                                 ServerName&& _name,
                                 BleService&& _wunderbarRunService,
                                 ServerUUID&& _uuid,
                                 PassKey&& _passKey,
                                 BleServerCallback _callback)
    : BleServer(_gateway,
                std::forward<ServerName>(_name),
                0xFF,
                {{wunderbar::services::SHORT_CONFIG_UUID,
                  BLE_UUID_TYPE_BLE,
                  UseMode::ONBOARD,
                  {{wunderbar::characteristics::SENSOR_ID_UUID, AccessMode::READ},
                   {wunderbar::characteristics::SENSOR_PASSKEY_UUID, AccessMode::WRITE},
                   {wunderbar::characteristics::SENSOR_MITM_REQ_FLAG_UUID, AccessMode::WRITE}}},
                 _wunderbarRunService,
                 {wunderbar::services::SHORT_DEVICE_INFORMATION_UUID,
                  BLE_UUID_TYPE_BLE,
                  UseMode::ALWAYS,
                  {{wunderbar::characteristics::BLE_MANUFACTURER_NAME_STRING_UUID, AccessMode::READ},
                   {wunderbar::characteristics::BLE_HARDWARE_REVISION_STRING_UUID, AccessMode::READ},
                   {wunderbar::characteristics::BLE_FIRMWARE_REVISION_STRING_UUID, AccessMode::READ}}},
                 {wunderbar::services::SHORT_BATTERY_UUID,
                  BLE_UUID_TYPE_BLE,
                  UseMode::ALWAYS,
                  {{wunderbar::characteristics::BLE_BATTERY_LEVEL_UUID, AccessMode::READ}}}},
                std::forward<ServerUUID>(_uuid),
                std::forward<PassKey>(_passKey),
                true,
                mbed::callback(this, &WunderbarSensor::wunderbarEvent)),
      sensorCallback(_callback),
      uuidUnderVerification(0)
{}

void WunderbarSensor::handleDiscovery()
{
    if(discoveryOk && discoveryServiceIdx < config.services.size())
    {
        auto& bleService = config.services[discoveryServiceIdx];
        discoveryServiceIdx++;
        if(discoveryCharacteristicIdx < bleService.characteristics.size())
        {
            auto& characteristic = bleService.characteristics[discoveryCharacteristicIdx];
            discoveryCharacteristicIdx++;
            switch(characteristic.mode)
            {
                case AccessMode::VERIFY:
                    uuidUnderVerification = characteristic.uuid; // intentional fall-through
                case AccessMode::READ:
                    gateway.readCharacteristic(config, characteristic);
                    break;
                case AccessMode::WRITE:
                    {
                        uint8_t* data;
                        size_t len = getWriteDataForCharacteristic(characteristic.uuid, data);
                        gateway.writeCharacteristic(config,
                                                    characteristic,
                                                    data,
                                                    len);
                    }
                    break;
                default:
                    break;
            }
        }
    }
    else if(discoveryOk)
    {
        gateway.serverDiscoveryComlpete(config);
    }
}

void WunderbarSensor::handleDiscoveryCharacteristic(BleEvent event, const uint8_t* data, size_t len)
{
    switch(event)
    {
        case BleEvent::DISCOVERY_CHARACTERISTIC_WRITE_DONE:
            handleDiscovery();
            break;
        case BleEvent::DISCOVERY_CHARACTERISTIC_READ_DONE:
            discoveryOk = verifyData(uuidUnderVerification, data, len);
            uuidUnderVerification = 0;
            handleDiscovery();
            break;
        default:
            break;
    }
}

void WunderbarSensor::wunderbarEvent(BleEvent event, const uint8_t* data, size_t len)
{
    if(registrationOk)
    {
        switch(event)
        {
            case BleEvent::DISCOVERY_COMPLETE:
                handleDiscovery();
                break;
            case BleEvent::DISCOVERY_CHARACTERISTIC_WRITE_DONE:
            case BleEvent::DISCOVERY_CHARACTERISTIC_READ_DONE:
                handleDiscoveryCharacteristic(event, data, len);
                break;
            case BleEvent::DISCOVERY_ERROR:
                discoveryOk = false;
                break;
            default:
                if(sensorCallback)
                {
                    sensorCallback(event, data, len);
                }
                break;
        }
    }
}

bool WunderbarSensor::verifyData(uint16_t characteristic, const uint8_t* data, size_t len)
{
    bool verificationOk = false;
    if(characteristic == wunderbar::characteristics::SENSOR_ID_UUID)
    {
        verificationOk = (std::memcmp(&config.uuid, data, sizeof(ServerUUID)) == 0);
    }
    return verificationOk;

}

size_t WunderbarSensor::getWriteDataForCharacteristic(uint16_t characteristic, uint8_t*& data)
{
    size_t len = 0;
    data = nullptr;
    if(characteristic == wunderbar::characteristics::SENSOR_PASSKEY_UUID)
    {
        data = reinterpret_cast<uint8_t*>(&config.uuid);
        len = sizeof(PassKey);
    }
    if(characteristic == wunderbar::characteristics::SENSOR_MITM_REQ_FLAG_UUID)
    {
        data = reinterpret_cast<uint8_t*>(&config.security);
        len = sizeof(Security);
    }
    return len;
}
