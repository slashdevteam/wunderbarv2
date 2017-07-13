#include "mbed.h"

#include "cdc.h"
#include "flash.h"
#include "tls.h"
#include "mqttprotocol.h"
#include "GS1500MInterface.h"
#include "nrf51822interface.h"
#include "button.h"
#include "led.h"
#include "blebridge.h"
#include "InterruptIn.h"
#include "DigitalIn.h"
#include "DigitalOut.h"
#include <list>

using usb::CDC;
using wunderbar::Configuration;

// Putting most object in global scope to save thread_stack_main, which is to small!
const Flash flash;
const Configuration& config = flash.getConfig();
CDC               cdc;
CDC* gcdc = &cdc;

GS1500MInterface  wifiConnection(WIFI_TX, WIFI_RX, 115200);
Nrf51822Interface ble(NRF_MOSI, NRF_MISO, NRF_SCLK, NRF_SSEL, NRF_SPI_EXT_INT);
TLS               tls(&wifiConnection, config.tls, &cdc);
MqttProtocol      mqtt(&tls, config.proto, &cdc);
Button            sw2(&mqtt, "button1", SW2);
Led               led(&mqtt, "actuator/led1", LED1);
BleBridge         bridge(&mqtt, "actuator/bridgetx", "actuator/bridgerx", ble);

// Nrf51822 nrf(NRF_MOSI, NRF_MISO, NRF_SCLK, NC, NRF_SPI_EXT_INT);

// std::list<DataId> devToBeOnboarded = {};
// std::list<DataId> devOnboarded = {};

// void handleNrfMessage(SpiFrame& rxFrame);
// void startRun();
// void killNrf();

// void nrfRead() {
//     SpiFrame rxFrame;
//     nrf.read((char*)&rxFrame,sizeof(rxFrame));
//     cdc.printf("Read %d bytes: ",sizeof(rxFrame));
//     // cdc.printf("0x");
//     // for (uint32_t byte=0;byte<sizeof(rxFrame);byte++)cdc.printf("%X",((char*)&rxFrame)[byte]);
//     cdc.printf("dataId 0x%X, fieldId 0x%X, op 0x%X",rxFrame.dataId, rxFrame.fieldId, rxFrame.operation);
//     cdc.printf("\n");

//     handleNrfMessage(rxFrame);
// }

// void handleNrfMessage(SpiFrame& rxFrame) {

//     if (FieldId::CONFIG_ONBOARD_DONE == rxFrame.fieldId) {
//         if (rxFrame.dataId >= DataId::DEV_HTU &&
//             rxFrame.dataId <= DataId::DEV_IR) {
//                 // Mark sensor as onboarded
//                 cdc.printf("Onboarding of sensorId %d done\n", rxFrame.dataId);
//                 devOnboarded.push_back(rxFrame.dataId);
//                 devOnboarded.sort();

//                 // Check and handle sensors' onboarding done
//                 if (devToBeOnboarded == devOnboarded) {
//                     cdc.printf("All sensors have been onboarded, requesting run mode...\n");
//                     wait(3.0);
//                     killNrf();
//                     wait(3.0);
//                     startRun();
//                 }
//             }
//     }
// }

// void startConfig() {

//     devToBeOnboarded = {DataId::DEV_LIGHT, DataId::DEV_GYRO, DataId::DEV_HTU, DataId::DEV_SOUND, DataId::DEV_BRIDGE, DataId::DEV_IR };
//     devToBeOnboarded.sort();
//     devOnboarded = {};

//     cdc.printf("Requesting config state from nrf\n");
//     SpiFrame txFrame;
//     SpiFrame rxFrame;
//     memset((char*)&txFrame,0,sizeof(txFrame));

//     txFrame.dataId  = DataId::CONFIG;
//     txFrame.fieldId = FieldId::CONFIG_START;
//     nrf.readWrite((char*)&rxFrame,(char*)&txFrame,sizeof(txFrame));
//     cdc.printf("...also read: ",sizeof(rxFrame));
//     cdc.printf("dataId 0x%X, fieldId 0x%X, op 0x%X",rxFrame.dataId, rxFrame.fieldId, rxFrame.operation);
//     cdc.printf("\n");
// }

// void startRun() {
//     cdc.printf("Requesting running state from nrf\n");
//     SpiFrame txFrame;
//     SpiFrame rxFrame;
//     memset((char*)&txFrame,0,sizeof(txFrame));

//     txFrame.dataId  = DataId::CONFIG;
//     txFrame.fieldId = FieldId::RUN;
//     nrf.readWrite((char*)&rxFrame,(char*)&txFrame,sizeof(txFrame));
//     cdc.printf("...also read: ",sizeof(rxFrame));
//     cdc.printf("dataId 0x%X, fieldId 0x%X, op 0x%X",rxFrame.dataId, rxFrame.fieldId, rxFrame.operation);
//     cdc.printf("\n");
// }

// void killNrf() {
//     cdc.printf("Killing nrf\n");
//     SpiFrame txFrame;
//     SpiFrame rxFrame;
//     memset((char*)&txFrame,0,sizeof(txFrame));

//     txFrame.dataId  = DataId::CONFIG;
//     txFrame.fieldId = FieldId::RUN;
//     nrf.readWrite((char*)&rxFrame,(char*)&txFrame,sizeof(txFrame));
//     cdc.printf("...also read: ",sizeof(rxFrame));
//     cdc.printf("dataId 0x%X, fieldId 0x%X, op 0x%X",rxFrame.dataId, rxFrame.fieldId, rxFrame.operation);
//     cdc.printf("\n");
// }

// void readSensorVal() {
//     cdc.printf("Reading sensor, size %d\n");
//     SpiFrame txFrame;
//     SpiFrame rxFrame;
//     memset((char*)&txFrame,0,sizeof(txFrame));

//     txFrame.dataId    = DataId::DEV_LIGHT;
//     txFrame.fieldId   = FieldId::CHAR_SENSOR_DATA_R;
//     txFrame.operation = Operation::READ;
//     nrf.readWrite((char*)&rxFrame,(char*)&txFrame,sizeof(txFrame));
//     cdc.printf("Read %d bytes: 0x",sizeof(rxFrame));
//     for (uint32_t byte=0;byte<sizeof(rxFrame);byte++)cdc.printf("%X",((char*)&rxFrame)[byte]);
//     cdc.printf("\n");
// }

// void nrfCb () {
//     cdc.printf("Callback fired\n");
//     nrfRead();
// }
PassKey defaultPass = {0x30, 0x30, 0x30, 0x30, 0x30, 0x31, 0x00, 0x00};

void bleCb(BleEvent a, const uint8_t* b, size_t c) {

}

// using BleServerCallback = mbed::Callback<void(BleEvent, const uint8_t*, size_t)>;

int main(int argc, char **argv)
{

    std::list<WunderbarSensor> wubarSensors;
    for(const auto& wubarName : ServerNamesToDataId)
    {
        wubarSensors.emplace_back(WunderbarSensor(ble, ServerName(std::get<const ServerName>(wubarName)), PassKey(defaultPass), bleCb));
    }

    // DigitalOut gsPD(PTD5);
    // gsPD = 0;

    // wait(5.0);
    // cdc.printf("Welcome to WunderBar v2 mbed OS firmware\n");
    // cdc.printf("Running at %d MHz\n", SystemCoreClock/1000000);

    // cdc.printf("Configuring BLE\n");
    // nrf.config(nrfCb);
    // cdc.printf("resetting BLE now...\n");
    // nrf.reset();
    // // killNrf();

    // wait(3.0);

    // startConfig();
    
    if(ble.configure())
    {
        cdc.printf("BLE config ok. Storing config.\n");
        if(ble.storeConfig())
        {
            cdc.printf("BLE config stored ok\n");
        }
        cdc.printf("Connecting to %s network\r\n", config.wifi.ssid);

        int status = wifiConnection.connect(config.wifi.ssid,
                                            config.wifi.pass,
                                            config.wifi.security,
                                            config.wifi.channel);

        if(status == NSAPI_ERROR_OK)
        {
            cdc.printf("Connected to %s network\r\n", config.wifi.ssid);
            cdc.printf("Creating connection over %s to %s:%d\r\n", mqtt.name, config.proto.server, config.proto.port);

            if(mqtt.connect())
            {
                cdc.printf("%s connected to %s:%d\r\n", mqtt.name, config.proto.server, config.proto.port);
                mqtt.setPingPeriod(10000);
                led.subscribe();
                bridge.subscribe();
            }
            else
            {
                cdc.printf("Connection to %s:%d over %s failed\r\n", config.proto.server, config.proto.port, mqtt.name);
                mqtt.disconnect();
            }
        }
        else
        {
            cdc.printf("Connection to %s network failed with status %d\r\n", config.wifi.ssid, status);
        }
    }

    while(true)
    {
        wait(1);
        cdc.printf("Led state: %d\r\n", led.read());
    }

}
