#include "mbed.h"

#include "cdc.h"
#include "flash.h"
#include "GS1500MInterface.h"
#include "button.h"
#include "led.h"

using usb::CDC;
using wunderbar::Configuration;

// Putting most object in global scope to save thread_stack_main, which is to small!
const Flash flash;
const Configuration& config = flash.getConfig();
CDC cdc;
GS1500MInterface wifiConnection(WIFI_TX, WIFI_RX, 115200);
Protocol protocol(&wifiConnection, config.proto, &cdc);
Button sw2(&protocol, "button1", SW2);
Led led(&protocol, "actuator/led1", LED1);

int main(int argc, char **argv)
{
    cdc.run();

    cdc.printf(" Welcome to WunderBar v2 mbed OS firmware\n");
    cdc.printf("Running at %d MHz\n", SystemCoreClock/1000000);

    cdc.printf("Connecting to %s network\r\n", config.wifi.ssid);

    int status = wifiConnection.connect(config.wifi.ssid,
                                        config.wifi.pass,
                                        config.wifi.security,
                                        config.wifi.channel);

    if(status == NSAPI_ERROR_OK)
    {
        cdc.printf("Connected to %s network\r\n", config.wifi.ssid);
        cdc.printf("Creating connection over %s to %s:%d\r\n", protocol.name, config.proto.server, config.proto.port);

        if(protocol.connect())
        {
            cdc.printf("%s connected to %s:%d\r\n", protocol.name, config.proto.server, config.proto.port);
            protocol.keepAlive(10000);
            // led.subscribe();
        }
        else
        {
            cdc.printf("Connection to %s:%d over %s failed\r\n", config.proto.server, config.proto.port, protocol.name);
            protocol.disconnect();
        }
    }
    else
    {
        cdc.printf("Connection to %s network failed with status %d\r\n", config.wifi.ssid, status);
    }

    while(true)
    {
        wait(2);
        cdc.printf("Led state: %d\r\n", led.read());
    }

}
