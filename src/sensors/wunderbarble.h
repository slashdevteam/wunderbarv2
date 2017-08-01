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

// UUIDs for all used characteristics
namespace characteristics
{
    namespace sensor
    {
    const uint16_t ID = 0x2010;
    const uint16_t BEACON_FREQ = 0x2011;
    const uint16_t FREQUENCY = 0x2012;
    const uint16_t LED_STATE = 0x2013;
    const uint16_t THRESHOLD = 0x2014;
    const uint16_t CONFIG = 0x2015;
    const uint16_t DATA_R = 0x2016;
    const uint16_t DATA_W = 0x2017;
    const uint16_t PASSKEY = 0x2018;
    const uint16_t MITM_REQ_FLAG = 0x2019;
    }

    namespace ble
    {
    const uint16_t BATTERY_LEVEL = 0x2A19;
    const uint16_t MANUFACTURER_NAME = 0x2A29;
    const uint16_t HARDWARE_REVISION = 0x2A27;
    const uint16_t FIRMWARE_REVISION = 0x2A26;
    }
}

namespace limits
{
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
