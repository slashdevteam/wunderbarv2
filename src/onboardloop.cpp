#include "loops.h"
#include "loopsutil.h"
#include "DigitalOut.h"
#include "istdinout.h"
#include "configuration.h"
#include "wifiwizard.h"
#include "cloudwizard.h"
#include "blewizard.h"
#include "flash.h"
#include "mbed_wait_api.h"

class Onboarder
{
public:
    Onboarder(Flash& _flash, IStdInOut& _log, IBleGateway& _ble, WiFiInterface& _wifi, NetworkStack& _net)
        : flash(_flash),
          log(_log),
          ble(_ble),
          wifi(_wifi),
          net(_net),
          executor(osPriorityNormal, 0x6000)
    {}

    void run()
    {
        executor.start(mbed::callback(this, &Onboarder::onboard));
        executor.join();
    }

private:
    void onboard()
    {
        // let's wait till stdio is connected
        mbed::DigitalOut led(LED1);
        while(-1 == log.putc('\n'))
        {
            wait(0.3);
            led = !led;
        }

        // configuration can be big and we need heap for TLS
        wunderbar::Configuration config;
        std::memset(&config, 0, sizeof(config));

        log.printf("Welcome to WunderBar V2 onboarding wizard!\r\n");
        log.printf("This app will guide you through the process of onboarding your device.\r\n");
        log.printf("Press ENTER to continue.\r\n");
        log.getc();
        // wizards are blocking till successful completion
        wifiWizard(&wifi, config.wifi, led, log);
        bleWizard(ble, log);
        cloudWizard(&net, config.proto, config.tls, config.rest, led, log);

        log.printf("Now, Wunderbar will store all parameters in memory. Please be patient.");
        flash.store(config);
        wifi.disconnect();
    }

private:
    Flash& flash;
    IStdInOut& log;
    IBleGateway& ble;
    WiFiInterface& wifi;
    NetworkStack& net;
    rtos::Thread executor;
};

void onboardLoop(Flash& flash, IStdInOut& log, IBleGateway& ble, WiFiInterface& wifi, NetworkStack& net)
{
    // mbed does not provide alternative for std::bind
    // so emulating it with thin wrapper with necessary pointers
    // around onboarding thread
    Onboarder onboarder(flash, log, ble, wifi, net);
    onboarder.run();
}

