#include "loops.h"
#include "GS1500MInterface.h"
#include "nrf51822interface.h"
#include "flash.h"
#include "cdc.h"
#include "tls.h"
#include "mqttprotocol.h"
#include "button.h"
#include "led.h"

using usb::CDC;
using wunderbar::Configuration;

extern Nrf51822Interface ble;
extern GS1500MInterface wifiConnection;
extern Flash flash;
extern CDC cdc;
const Configuration& config = flash.getConfig();

TLS              tls(&wifiConnection, config.tls, &cdc);
MqttProtocol     mqtt(&tls, config.proto, &cdc);
Button           sw2(&mqtt, "button1", SW2);
Led              led(&mqtt, "actuator/led1", LED1);

void runLoop()
{
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
            led.subscribe();
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
        cdc.printf("Led state: 0x%x\r\n", led.read());
    }
}
