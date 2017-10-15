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

using OnboardStep = mbed::Callback<bool(wunderbar::Configuration&)>;

class Onboarder
{
public:
    Onboarder(Flash& _flash,
              IStdInOut& _log,
              IBleGateway& _ble,
              WiFiInterface& _wifi,
              NetworkStack& _net,
              const Resources& _resources,
              uint8_t* loopStack,
              size_t loopStackSize)
        : flash(_flash),
          log(_log),
          ble(_ble),
          wifi(_wifi),
          net(_net),
          resources(_resources),
          executor(osPriorityNormal, loopStackSize, loopStack, "ONBOARD_LOOP"),
          steps{mbed::callback(this, &Onboarder::wifiStep),
                mbed::callback(this, &Onboarder::bleStep),
                mbed::callback(this, &Onboarder::cloudStep)},
          stepStatus{false, false, false},
          led(LED1)
    {
    }

    void run()
    {
        executor.start(mbed::callback(this, &Onboarder::onboard));
        executor.join();
    }

private:
    void onboard()
    {
        // let's wait till stdio is connected
        waitForUser();
        welcomeUser();

        // configuration can be big and we need heap for TLS
        wunderbar::Configuration config;
        std::memset(&config, 0, sizeof(config));
        config.wifi.security = NSAPI_SECURITY_WPA2;

        while(!stepStatus[0] || !stepStatus[1] || !stepStatus[2])
        {
            uint32_t step = selectStep();
            // wizards are blocking till successful completion
            stepStatus[step] = steps[step](config);
        }

        storeConfig(config);
        cleanUp();
    }

    void waitForUser()
    {
        while(-1 == log.putc('\n'))
        {
            for(uint32_t shortBlink = 0; shortBlink < 4; ++shortBlink)
            {
                wait(0.133);
                led = !led;
            }

            for(uint32_t longBlink = 0; longBlink < 4; ++longBlink)
            {
                wait(0.45);
                led = !led;
            }
        }
    }

    void welcomeUser()
    {
        log.printf("Welcome to WunderBar V2 onboarding wizard!\r\n");
        log.printf("This app will guide you through the process of onboarding your device.\r\n");
        log.printf("Press ENTER to continue.\r\n");
        waitForEnter(log);
    }

    uint32_t selectStep()
    {
        bool stepSelection = false;
        char stepStrIndex[2] = "1";

        while(!stepSelection)
        {
            std::snprintf(stepStrIndex, 2, "%1ld", firstIncompleteState());
            log.printf("\r\nPlease select configuration step:\r\n");
            log.printf("1 - WiFi setup [%s]\r\n", stepStatus[0] ? "Completed" : "Pending");
            log.printf("2 - Bluetooth setup [%s]\r\n", stepStatus[1] ? "Completed" : "Pending");
            log.printf("3 - ConradConnect setup[%s]\r\n", stepStatus[2] ? "Completed" : "Pending");
            stepSelection = readField(log, stepStrIndex, 1, 1, stepStrIndex, mbed::callback(this, &Onboarder::validateStep), true, led);
        }
        // -1 to convert selection to C++ indexing
        return std::atoi(stepStrIndex) - 1;
    }

    bool validateStep(char c)
    {
        bool valid = false;

        if((0x31 <= c) && (0x33 >= c))
        {
            valid = true;
        }

        return valid;
    }

    bool wifiStep(wunderbar::Configuration& config)
    {
        return wifiWizard(&wifi, config.wifi, led, log);
    }

    bool bleStep(wunderbar::Configuration& config)
    {
        return bleWizard(ble, config.ble, led, log);
    }

    bool cloudStep(wunderbar::Configuration& config)
    {
        bool cloudOk = false;
        if(stepStatus[0])
        {
            cloudOk = cloudWizard(&net, config.proto, config.tls, config.rest, led, log, resources);
        }
        else
        {
            log.printf("\r\nWiFi wizard needs to be completed before cloud setup!\r\n\r\n");
        }
        return cloudOk;
    }

    void storeConfig(const wunderbar::Configuration& config)
    {
        log.printf("Now, Wunderbar will store all parameters in memory. Please be patient.\r\n");
        flash.store(config);
    }

    void cleanUp()
    {
        wifi.disconnect();
        log.printf("Onboarding wizard done!\r\n\r\n");
    }

    uint32_t firstIncompleteState()
    {
        uint32_t incompleteStepIdx = 1;
        for(uint32_t step = 0; step < 3; ++step)
        {
            if(stepStatus[step] == false)
            {
                incompleteStepIdx = step + 1;
                break;
            }
        }
        return incompleteStepIdx;
    }

private:
    Flash& flash;
    IStdInOut& log;
    IBleGateway& ble;
    WiFiInterface& wifi;
    NetworkStack& net;
    const Resources& resources;
    rtos::Thread executor;
    OnboardStep steps[3];
    bool stepStatus[3];
    mbed::DigitalOut led;
};

void onboardLoop(Flash& flash,
                 IStdInOut& log,
                 IBleGateway& ble,
                 WiFiInterface& wifi,
                 NetworkStack& net,
                 const Resources& resources,
                 uint8_t* loopStack,
                 size_t loopStackSize)
{
    // mbed does not provide alternative for std::bind
    // so emulating it with thin wrapper with necessary pointers
    // around onboarding thread
    Onboarder onboarder(flash,
                        log,
                        ble,
                        wifi,
                        net,
                        resources,
                        loopStack,
                        loopStackSize);
    onboarder.run();
}

