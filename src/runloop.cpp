#include "loops.h"
#include "flash.h"
#include "cdc.h"
#include "tls.h"
#include "mqttprotocol.h"
#include "resources.h"
#include "DigitalOut.h"
#include "mbed_wait_api.h"
#include "httpsrequest.h"
#include "httpparser.h"
#include "mbed_rtc_time.h"

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
          executor(osPriorityNormal, 0x4500)
    {}

    void run()
    {
        executor.start(mbed::callback(this, &MainLooper::loop));
        executor.join();
    }

private:
    void loop()
    {
        log.printf("Connecting to %s network\r\n", config.wifi.ssid);
        int status = wifi.connect(config.wifi.ssid,
                                  config.wifi.pass,
                                  config.wifi.security,
                                  config.wifi.channel);

        time_t currentTime = getTime();
        set_time(currentTime);

        TLS              tls(&net, config.tls, &log);
        MqttProtocol     mqtt(&tls, config.proto, &log);

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

    time_t getTime()
    {
        time_t currentTime = 0;
        TLS restTls(&net, config.rest.tls, &log);
        std::string timeUrl(config.rest.path);
        timeUrl.append("time?format=s");
        log.printf("Contacting %s\r\n", timeUrl.c_str());
        HttpsRequest request(restTls, "GET", config.rest.server, config.rest.port, timeUrl.c_str(), nullptr);
        request.setHeader("X-AUTH-TOKEN", config.rest.token);
        uint8_t httpsBuffer[512] = {0};
        if(request.send())
        {
            if(request.recv(httpsBuffer, sizeof(httpsBuffer)))
            {
                HttpParser response(reinterpret_cast<const char*>(httpsBuffer));
                log.printf("STATUS: %d\r\n", response.status);
                if(response)
                {
                    log.printf("BODY: %s\r\n", response.body);
                    sscanf(response.body, "%ld", &currentTime);
                }
                else
                {
                    log.printf("Error response from %s - %d: %s\r\n", config.rest.server, response.status, response.statusString);
                }
            }
            else
            {
                log.printf("No reply from %s\r\n", config.rest.server);
            }
        }
        else
        {
            log.printf("Unable to connect to %s\r\n", config.rest.server);
        }

        return currentTime;
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
