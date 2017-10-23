#include "mbed.h"

#include "flash.h"
#include "cdc.h"
#include "GS1500MInterface.h"
#include "loops.h"
#include "nrf51822interface.h"

#include "resources.h"
#include "wblightprox.h"
#include "wbhtu.h"
#include "wbgyro.h"
#include "wbinfrared.h"
#include "wbmicrophone.h"
#include "wbbridge.h"
#include "button.h"
#include "led.h"
#include "info.h"

// prevent mbed from using UART
#include "nouartfix.h"
#include "mbedtls/platform.h"

using usb::CDC;

using wunderbar::Configuration;

namespace wunderbar
{
    extern device_specific_descriptors cdcDescriptors;
}

const uint8_t CONTROLLER_ID = 0;
// Putting most objects in global scope to save stack
Flash flash;
CDC               cdc(CONTROLLER_ID, wunderbar::cdcDescriptors);
GS1500MInterface  wifiConnection(WIFI_TX, WIFI_RX, 115200);
Nrf51822Interface ble(MOSI, MISO, SCLK, SSEL, SPI_EXT_INT, &cdc);

// defined in nouartfix.cpp, needed for global IO retarget
extern IStdInOut* stdioRetarget;
// allocate run/onboard loops stack in m_data RAM to avoid issues with new allocator
uint8_t LOOP_STACK[0x7500] __attribute__((section (".hugestack")));

// Dependency is reversed here - normally resources should not know/care
// that they need to be on some kind of list, but due to spurious copy ctors
// that GCC includes for initializer lists (even with perfect forwarding with
// std::move/forward and rvalue references; possibly connected to
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=63707) it's impossible (with GCC,
// clang works) to create global, variable sized list of unique_ptr in compile time in global scope.
// Hence, each Resource is registering itself to resources list.
Resources    resources;
WbHtu        htu(ble, &resources, cdc);
WbGyro       gyro(ble, &resources, cdc);
WbLightProx  light(ble, &resources, cdc);
WbMicrophone mic(ble, &resources, cdc);
WbInfraRed   ir(ble, &resources, cdc);
WbBridge     bridge(ble, &resources, cdc);
Button       sw2(flash, &resources, "button1", SW2, cdc);
Led          led(&resources, "LED", LED1, cdc);
Info         info(&resources, cdc);

int main(int argc, char **argv)
{
    stdioRetarget = &cdc;
    mbedtls_platform_set_printf(&cdcPrintfRetarget);

    cdc.printf("Welcome to WunderBar v2 mbed OS firmware\n");
    cdc.printf("Running at %d MHz\n", SystemCoreClock/1000000);

    while(true)
    {
        if(!flash.isOnboarded())
        {
            // blocking call
            onboardLoop(flash,
                        cdc,
                        ble,
                        wifiConnection,
                        wifiConnection,
                        resources,
                        LOOP_STACK,
                        sizeof(LOOP_STACK));
        }
        else
        {
            runLoop(flash.getConfig(),
                    cdc,
                    ble,
                    info,
                    wifiConnection,
                    wifiConnection,
                    resources,
                    LOOP_STACK,
                    sizeof(LOOP_STACK));
        }
        wait(2);
    }

}
