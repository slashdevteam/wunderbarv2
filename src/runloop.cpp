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
#include "loopsutil.h"

class MainLooper
{
public:
    MainLooper(const wunderbar::Configuration& _config,
               IStdInOut& _log,
               IBleGateway& _ble,
               Info& _info,
               WiFiInterface& _wifi,
               NetworkStack& _net,
               const Resources& _resources,
               uint8_t* loopStack,
               size_t loopStackSize)
        : config(_config),
          log(_log),
          ble(_ble),
          info(_info),
          wifi(_wifi),
          net(_net),
          resources(_resources),
          executor(osPriorityNormal, loopStackSize, loopStack, "MAIN_LOOPER")
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
        mbed::DigitalOut led(LED1);
        ProgressBar progressBar(log, led, false, 450);
        progressBar.start();
        int status = wifi.connect(config.wifi.ssid,
                                  config.wifi.pass,
                                  config.wifi.security,
                                  config.wifi.channel);
        progressBar.terminate();

        if(status == NSAPI_ERROR_OK)
        {
            log.printf("Connected to %s network.\r\n", config.wifi.ssid);
            time_t currentTime = getTime(led);
            set_time(currentTime);

            // by default TLS is not logging, to enable pass &log to TLS constructor instead of &devnull
            IStdInOut devNull;
            TLS              tls(&net, config.tls, &devNull);
            MqttProtocol     mqtt(&tls, config.proto, &log);

            log.printf("Creating connection over %s to %s:%d.\r\n", mqtt.name, config.proto.server, config.proto.port);
            ProgressBar progressBar(log, led, false, 133);
            progressBar.start();
            bool mqttConnected = mqtt.connect();
            progressBar.terminate();
            if(mqttConnected)
            {
                log.printf("%s connected to %s:%d.\r\n", mqtt.name, config.proto.server, config.proto.port);
                mqtt.setPingPeriod(info.getPingIntervalMs());
                info.setPingChangeCallback(mbed::callback(&mqtt, &MqttProtocol::setPingPeriod));
                ble.configure(config.ble);
                // add resources to MQTT
                for(auto resource : resources.current)
                {
                    resource->advertise(&mqtt);
                }
                log.printf("Starting BLE\n");

                ble.startOperation();

                // blocks while MQTT is connected
                mqtt.loop();

                // clean-up when MQTT is disconnected
                ble.stopOperation();
                for(auto resource : resources.current)
                {
                    resource->revoke();
                }
            }
            else
            {
                log.printf("Connection to %s:%d over %s failed.\r\n", config.proto.server, config.proto.port, mqtt.name);
                mqtt.disconnect();
                return;
            }
        }
        else
        {
            log.printf("Connection to %s network failed with status %d.\r\n", config.wifi.ssid, status);
            return;
        }
    }

    time_t getTime(mbed::DigitalOut& led)
    {
        time_t currentTime = 0;
        IStdInOut devNull;
        TLS restTls(&net, config.rest.tls, &devNull);
        std::string timeUrl(config.rest.path);
        timeUrl.append("time?format=s");
        log.printf("\r\nContacting %s\r\n", timeUrl.c_str());
        HttpsRequest request(restTls, "GET", config.rest.server, config.rest.port, timeUrl.c_str(), nullptr);
        request.setHeader("X-AUTH-TOKEN", config.rest.token);
        uint8_t httpsBuffer[512] = {0};
        ProgressBar progressBar(log, led, false, 133);
        progressBar.start();
        bool sendStatus = request.send();
        progressBar.terminate();
        if(sendStatus)
        {
            if(request.recv(httpsBuffer, sizeof(httpsBuffer)))
            {
                HttpParser response(reinterpret_cast<const char*>(httpsBuffer));
                if(response)
                {
                    sscanf(response.body, "%ld", &currentTime);
                }
                else
                {
                    log.printf("Error response from %s - %d: %.10s.\r\n", config.rest.server, response.status, response.statusString);
                }
            }
            else
            {
                log.printf("No reply from %s.\r\n", config.rest.server);
            }
        }
        else
        {
            log.printf("Unable to connect to %s.\r\n", config.rest.server);
        }

        return currentTime;
    }

private:
    const wunderbar::Configuration& config;
    IStdInOut& log;
    IBleGateway& ble;
    Info& info;
    WiFiInterface& wifi;
    NetworkStack& net;
    const Resources& resources;
    rtos::Thread executor;
};

void runLoop(const wunderbar::Configuration& config,
             IStdInOut& log,
             IBleGateway& ble,
             Info& info,
             WiFiInterface& wifi,
             NetworkStack& net,
             const Resources& resources,
             uint8_t* loopStack,
             size_t loopStackSize)
{
    // mbed does not provide alternative for std::bind
    // so emulating it with thin wrapper with necessary pointers
    // around run thread
    MainLooper mainLooper(config,
                          log,
                          ble,
                          info,
                          wifi,
                          net,
                          resources,
                          loopStack,
                          loopStackSize);
    mainLooper.run();
}
