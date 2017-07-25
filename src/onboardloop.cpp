#include "loops.h"
#include "loopsutil.h"
#include "DigitalOut.h"
#include "cdc.h"
#include "GS1500MInterface.h"
#include "nrf51822interface.h"
#include "configuration.h"
#include "wifiwizard.h"
#include "cloudwizard.h"

extern GS1500MInterface wifiConnection;
extern Nrf51822Interface ble;
using usb::CDC;
extern CDC cdc;

void onboard()
{
    // let's wait till user connects to CDC console
    DigitalOut led(LED1);
    while(-1 == cdc.putc('\n'))
    {
        wait(0.3);
        led = !led;
    }

    // configuration can be big (SSL certs) and we need stack for TLS, so allocate space for config on heap
    auto wunderConfig = std::make_unique<wunderbar::Configuration>();
    std::memset(wunderConfig.get(), 0, sizeof(wunderbar::Configuration));

    cdc.printf("Welcome to WunderBar V2 onboarding wizard!\r\n");
    cdc.printf("This app will guide you through the process of onboarding your device.\r\n");
    cdc.printf("Press ENTER to continue.\r\n");
    cdc.getc();
    // wizards are blocking till successful completion
    wifiWizard(&wifiConnection, wunderConfig->wifi, led);
    cloudWizard(&wifiConnection, wunderConfig->proto, wunderConfig->tls, led);


    cdc.printf("Now, Wunderbar will store all parameters in memory. Please be patient.");

    cdc.printf("Configuring BLE... \n");
    ble.configure();

    while(true)
    {
        wait(2);
        cdc.printf("Onboard state: %d\r\n", 1);
    }
}

void onboardLoop()
{
    rtos::Thread onboarder(osPriorityNormal, 0x4000);
    onboarder.start(&onboard);
    onboarder.join();
}

