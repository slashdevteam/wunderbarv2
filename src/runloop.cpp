#include "loops.h"
#include "flash.h"
#include "cdc.h"
#include "tls.h"
#include "mqttprotocol.h"
#include "resources.h"
#include "DigitalOut.h"
#include "mbed_wait_api.h"

class MainLooper
{
public:
    MainLooper(const wunderbar::Configuration& _config,
               IStdInOut& _log,
               IBleGateway& _ble,
               WiFiInterface& _wifi,
               NetworkStack& _net,
               const Resources& _resources)
        : config(_config),
          log(_log),
          ble(_ble),
          wifi(_wifi),
          net(_net),
          resources(_resources),
          executor(osPriorityNormal, 0x2500)
    {}

    void run()
    {
        executor.start(mbed::callback(this, &MainLooper::loop));
        executor.join();
    }

private:
    void loop()
    {
        TLS              tls(&net, config.tls, &log);
        MqttProtocol     mqtt(&tls, config.proto, &log);

        log.printf("Connecting to %s network\r\n", config.wifi.ssid);
        int status = wifi.connect(config.wifi.ssid,
                                  config.wifi.pass,
                                  config.wifi.security,
                                  config.wifi.channel);

        if(status == NSAPI_ERROR_OK)
        {
            log.printf("Connected to %s network\r\n", config.wifi.ssid);
            log.printf("Starting BLE\n");
            ble.startOperation();

            log.printf("Creating connection over %s to %s:%d\r\n", mqtt.name, config.proto.server, config.proto.port);
            if(mqtt.connect())
            {
                log.printf("%s connected to %s:%d\r\n", mqtt.name, config.proto.server, config.proto.port);
                mqtt.setPingPeriod(10000);

                // add resources to MQTT
                for(auto resource : resources.current)
                {
                    resource->advertise(&mqtt);
                }
            }
            else
            {
                log.printf("Connection to %s:%d over %s failed\r\n", config.proto.server, config.proto.port, mqtt.name);
                mqtt.disconnect();
                return;
            }
        }
        else
        {
            log.printf("Connection to %s network failed with status %d\r\n", config.wifi.ssid, status);
            return;
        }

        while(true)
        {
            wait(2);
        }
    }

private:
    const wunderbar::Configuration& config;
    IStdInOut& log;
    IBleGateway& ble;
    WiFiInterface& wifi;
    NetworkStack& net;
    const Resources& resources;
    rtos::Thread executor;
};

void runLoop(const wunderbar::Configuration& config,
             IStdInOut& log,
             IBleGateway& ble,
             WiFiInterface& wifi,
             NetworkStack& net,
             const Resources& resources)
{
    // mbed does not provide alternative for std::bind
    // so emulating it with thin wrapper with necessary pointers
    // around onboarding thread
    MainLooper mainLooper(config, log, ble, wifi, net, resources);
    mainLooper.run();
}
