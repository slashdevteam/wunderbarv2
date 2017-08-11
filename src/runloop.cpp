#include "loops.h"
#include "GS1500MInterface.h"
#include "nrf51822interface.h"
#include "flash.h"
#include "cdc.h"
#include "tls.h"
#include "mqttprotocol.h"
#include "resources.h"

using usb::CDC;
using wunderbar::Configuration;

extern Resources resources;
extern Nrf51822Interface ble;
extern GS1500MInterface wifiConnection;
extern Flash flash;
extern CDC cdc;
const Configuration& config = flash.getConfig();



void runLoop()
{
    TLS              tls(&wifiConnection, config.tls, &cdc);
    MqttProtocol     mqtt(&tls, config.proto, &cdc);

    cdc.printf("Connecting to %s network\r\n", config.wifi.ssid);
    int status = wifiConnection.connect(config.wifi.ssid,
                                        config.wifi.pass,
                                        config.wifi.security,
                                        config.wifi.channel);

    if(status == NSAPI_ERROR_OK)
    {
        cdc.printf("Connected to %s network\r\n", config.wifi.ssid);
        cdc.printf("Starting BLE\n");
        ble.startOperation();

        cdc.printf("Creating connection over %s to %s:%d\r\n", mqtt.name, config.proto.server, config.proto.port);
        if(mqtt.connect())
        {
            cdc.printf("%s connected to %s:%d\r\n", mqtt.name, config.proto.server, config.proto.port);
            mqtt.setPingPeriod(10000);

            // add resources to MQTT
            for(auto resource : resources.current)
            {
                resource->advertise(&mqtt);
            }
        }
        else
        {
            cdc.printf("Connection to %s:%d over %s failed\r\n", config.proto.server, config.proto.port, mqtt.name);
            mqtt.disconnect();
            return;
        }
    }
    else
    {
        cdc.printf("Connection to %s network failed with status %d\r\n", config.wifi.ssid, status);
        return;
    }

    while(true)
    {
        wait(2);
    }
}
