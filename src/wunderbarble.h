#pragma once

const uint8_t BLE_UUID_TYPE_BLE = 0x01;

namespace wunderbar
{

namespace services
{
    const uint16_t SHORT_RELAYR_UUID = 0x2000;
    const uint16_t SHORT_CONFIG_UUID = 0x2001;
    const uint16_t SHORT_RELAYR_OPEN_COMM_UUID = 0x2002;
    const uint16_t SHORT_DEVICE_INFORMATION_UUID = 0x180A;
    const uint16_t SHORT_BATTERY_UUID = 0x180F;
}

namespace characteristics
{
    const uint16_t SENSOR_ID_UUID = 0x2010;
    const uint16_t SENSOR_BEACON_FREQUENCY_UUID = 0x2011;
    const uint16_t SENSOR_LED_STATE_UUID = 0x2013;
    const uint16_t SENSOR_CONFIG_UUID = 0x2015;
    const uint16_t SENSOR_DATA_R_UUID = 0x2016;
    const uint16_t SENSOR_DATA_W_UUID = 0x2017;
    const uint16_t SENSOR_PASSKEY_UUID = 0x2018;
    const uint16_t SENSOR_MITM_REQ_FLAG_UUID = 0x2019;
    const uint16_t BLE_BATTERY_LEVEL_UUID = 0x2A19;
    const uint16_t BLE_MANUFACTURER_NAME_STRING_UUID = 0x2A29;
    const uint16_t BLE_HARDWARE_REVISION_STRING_UUID = 0x2A27;
    const uint16_t BLE_FIRMWARE_REVISION_STRING_UUID = 0x2A26;
}

namespace limits
{
    const size_t MAX_DISCOVERY_SERVICES = 4;
    const size_t MAX_NUMBER_OF_CHARACTERISTICS = 12;
    const size_t MAX_SERVERS = 7;
    const size_t SERVER_NAME_MAX_LEN = 14;
    const size_t SERVER_PASS_MAX_LEN = 6;
    const size_t SERVER_UUID_MAX_LEN = 16;
}

namespace sensors
{
    const uint8_t DATA_ID_DEV_HTU             = 0x0;
    const uint8_t DATA_ID_DEV_GYRO            = 0x1;
    const uint8_t DATA_ID_DEV_LIGHT           = 0x2;
    const uint8_t DATA_ID_DEV_SOUND           = 0x3;
    const uint8_t DATA_ID_DEV_BRIDGE          = 0x4;
    const uint8_t DATA_ID_DEV_IR              = 0x5;
}

}
