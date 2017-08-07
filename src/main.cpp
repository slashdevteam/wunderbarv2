#include "mbed.h"

#include "flash.h"
#include "cdc.h"
#include "GS1500MInterface.h"
#include "loops.h"
#include "nrf51822interface.h"

#include "mqttprotocol.h"

#include "wblightprox.h"
#include "wbhtu.h"
#include "wbgyro.h"
#include "wbinfrared.h"
#include "wbmicrophone.h"
#include "wbbridge.h"

// prevent mbed from using UART
#include "nouartfix.h"
#include "mbedtls/platform.h"

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
// Nrf51822Interface ble(MOSI, MISO, SCLK, SSEL, SPI_EXT_INT, &cdc);

extern MqttProtocol mqtt;

// WbHtu        htu(ble, &mqtt);
// WbGyro       gyro(ble, &mqtt);
// WbLightProx  light(ble, &mqtt);
// WbMicrophone mic(ble, &mqtt);
// WbInfraRed   ir(ble, &mqtt);
// WbBridge     bridge(ble, &mqtt);

int main(int argc, char **argv)
{
    mbedtls_platform_set_printf(&cdcPrintfRetarget);
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
