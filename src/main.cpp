#include "mbed.h"

#include "cdc.h"
#include "flash.h"
#include "GS1500MInterface.h"

DigitalOut led1(LED1);
using usb::CDC;
using wunderbar::Configuration;

int main(int argc, char **argv)
{
    CDC cdc;

    cdc.printf(" Welcome to WunderBar v2 mbed OS firmware\n");
    cdc.printf("Running at %d MHz\n", SystemCoreClock/1000000);

    Flash flash;
    const Configuration& config = flash.getConfig();

    cdc.printf("Connecting to %s network\r\n", config.wifi.ssid);

    GS1500MInterface wifiConnection(WIFI_TX, WIFI_RX, 115200, false);

    wifiConnection.connect(config.wifi.ssid,
                           config.wifi.pass,
                           config.wifi.security,
                           config.wifi.channel);

    cdc.printf("Connected to %s network\r\n", config.wifi.ssid);

    Protocol protocol(&wifiConnection, config.proto, &cdc);
    cdc.printf("Creating connection over %s to %s:%d\r\n", protocol.name, config.proto.server, config.proto.port);

    if(protocol.connect())
    {
        cdc.printf("%s connected to %s to %s:%d\r\n", protocol.name, config.proto.server, config.proto.port);
    }
    else
    {
        cdc.printf("Connection to %s:%d over %s failed\r\n", config.proto.server, config.proto.port, protocol.name);
        MBED_ASSERT(false);
    }


    while (true)
    {
        led1 = !led1;
        wait(2);
        cdc.printf("Led state: %d\r\n", !led1.read());
    }

    wifiConnection.disconnect();
}

