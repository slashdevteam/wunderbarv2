#include "mbed.h"

#include "flash.h"
#include "cdc.h"
#include "GS1500MInterface.h"
#include "loops.h"
#include "nrf51822interface.h"
#include "wunderbarsensor.h"

#include "mqttprotocol.h"

using usb::CDC;

using wunderbar::Configuration;

namespace wunderbar
{
    extern device_specific_descriptors cdcDescriptors;
}

const uint8_t CONTROLLER_ID = 0; //kUSB_ControllerKhci0
// Putting most objects in global scope to save thread_stack_main, which is too small!
Flash flash;
CDC               cdc(CONTROLLER_ID, wunderbar::cdcDescriptors);
GS1500MInterface  wifiConnection(WIFI_TX, WIFI_RX, 115200);
Nrf51822Interface ble(MOSI, MISO, SCLK, SSEL, SPI_EXT_INT, &cdc);

// Dummy passkey and callback for ongoing development
PassKey defaultPass = {0x34, 0x36, 0x37, 0x33, 0x36, 0x31, 0x00, 0x00};

void userBleCb(BleEvent a, const uint8_t* b, size_t c) {
    cdc.printf("Ble cb!\n");
}

extern MqttProtocol mqtt;

// WunderbarSensor wbHtu(ble, ServerName(WunderbarSensorNames[0]), PassKey(defaultPass), bleCb);
// WunderbarSensor wbGyro(ble, ServerName(WunderbarSensorNames[1]), PassKey(defaultPass), bleCb);
// WunderbarSensor wbLight(ble, ServerName(WunderbarSensorNames[2]), PassKey(defaultPass), bleCb);
// WunderbarSensor wbMic(ble, ServerName(WunderbarSensorNames[3]), PassKey(defaultPass), bleCb);
// WunderbarSensor wbIr(ble, ServerName(WunderbarSensorNames[5]), PassKey(defaultPass), bleCb);
WunderbarSensor wbBridge(ble, ServerName(WunderbarSensorNames[4]), PassKey(defaultPass), userBleCb, &mqtt, "actuator/bridgetx", "actuator/bridgerx");

int main(int argc, char **argv)
{
    cdc.printf("Welcome to WunderBar v2 mbed OS firmware\n");
    cdc.printf("Running at %d MHz\n", SystemCoreClock/1000000);

    while(true)
    {
        if(!flash.isOnboarded())
        {
            // blocking call
            onboardLoop();
        }
        else
        {
            runLoop();
        }
        wait(2);
        cdc.printf("Flash marker: 0x%x\r\n", flash.getStorage().marker);
    }

}
