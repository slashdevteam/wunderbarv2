#include "loops.h"
#include "loopsutil.h"
#include "DigitalOut.h"
#include "cdc.h"
#include "GS1500MInterface.h"
#include "nrf51822interface.h"
#include "configuration.h"
#include "wifiwizard.h"
#include "cloudwizard.h"
#include "blewizard.h"
#include "flash.h"

extern GS1500MInterface wifiConnection;
extern Nrf51822Interface ble;
using usb::CDC;
extern CDC cdc;
extern Flash flash;

void onboard()
{
    // let's wait till user connects to CDC console
    DigitalOut led(LED1);
    while(-1 == cdc.putc('\n'))
    {
        wait(0.3);
        led = !led;
    }

    // configuration can be big and we need heap for TLS
    wunderbar::Configuration config;
    std::memset(&config, 0, sizeof(config));

    cdc.printf("Welcome to WunderBar V2 onboarding wizard!\r\n");
    cdc.printf("This app will guide you through the process of onboarding your device.\r\n");
    cdc.printf("Press ENTER to continue.\r\n");
    cdc.getc();
    // wizards are blocking till successful completion
    wifiWizard(&wifiConnection, config.wifi, led);
    bleWizard(ble);
    cloudWizard(&wifiConnection, config.proto, config.tls, led);

    cdc.printf("Now, Wunderbar will store all parameters in memory. Please be patient.");
    // @TODO: storing not tested as connection to CDC is not working
    // flash.store(config);
    while(true)
    {
        wait(2);
        cdc.printf("Onboard state: %d\r\n", 1);
    }
}

void onboardLoop()
{
    // Thread stack is on heap
    rtos::Thread onboarder(osPriorityNormal, 0x6000);
    onboarder.start(&onboard);
    onboarder.join();
}

