#include "mbed.h"

#include "flash.h"
#include "cdc.h"
#include "GS1500MInterface.h"
#include "loops.h"
#include "nrf51822interface.h"
#include "htu.h"

#include "mqttprotocol.h"
#include "lightprox.h"
#include "htu.h"
#include "gyro.h"
#include "infrared.h"
#include "microphone.h"
#include "bridge.h"

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

extern MqttProtocol mqtt;

Htu wbHtu(ble, &mqtt);
Gyro wbGyro(ble, &mqtt);
LightProx wbLight(ble, &mqtt);
Microphone wbMic(ble, &mqtt);
InfraRed wbIr(ble, &mqtt);
Bridge wbBridge(ble, &mqtt);

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
