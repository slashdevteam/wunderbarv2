#include "mbed.h"

#include "cdc.h"
#include "flash.h"
#include "tls.h"
#include "mqttprotocol.h"
#include "GS1500MInterface.h"
#include "nrf51822interface.h"
#include "button.h"
#include "led.h"
#include "blebridge.h"
#include "InterruptIn.h"
#include "DigitalIn.h"
#include "DigitalOut.h"

using usb::CDC;
using wunderbar::Configuration;

// Putting most object in global scope to save thread_stack_main, which is to small!
const Flash flash;
const Configuration& config = flash.getConfig();
CDC               cdc;
CDC* gcdc = &cdc;
GS1500MInterface  wifiConnection(WIFI_TX, WIFI_RX, 115200);
Nrf51822Interface ble(NRF_MOSI, NRF_MISO, NRF_SCLK, NC, NRF_SPI_EXT_INT);
// Nrf51822Interface ble(NRF_MOSI, NRF_MISO, NRF_SCLK, NRF_SSEL, NRF_SPI_EXT_INT);
TLS               tls(&wifiConnection, config.tls, &cdc);
MqttProtocol      mqtt(&tls, config.proto, &cdc);
Button            sw2(&mqtt, "button1", SW2);
Led               led(&mqtt, "actuator/led1", LED1);
BleBridge         bridge(&mqtt, "actuator/bridgetx", "actuator/bridgerx", ble);

int main(int argc, char **argv)
{
    cdc.printf("Welcome to WunderBar v2 mbed OS firmware\n");
    cdc.printf("Running at %d MHz\n", SystemCoreClock/1000000);

    cdc.printf("Configuring BLE\n");
    if(ble.configure())
    {
    //     cdc.printf("BLE config ok. Storing config.\n");
    //     if(ble.storeConfig())
    //     {
    //         cdc.printf("BLE config stored ok\n");
    //     }
        cdc.printf("Connecting to %s network\r\n", config.wifi.ssid);

        int status = wifiConnection.connect(config.wifi.ssid,
                                            config.wifi.pass,
                                            config.wifi.security,
                                            config.wifi.channel);

        if(status == NSAPI_ERROR_OK)
        {
            cdc.printf("Connected to %s network\r\n", config.wifi.ssid);
            cdc.printf("Creating connection over %s to %s:%d\r\n", mqtt.name, config.proto.server, config.proto.port);

            if(mqtt.connect())
            {
                cdc.printf("%s connected to %s:%d\r\n", mqtt.name, config.proto.server, config.proto.port);
                mqtt.setPingPeriod(10000);
                led.subscribe();
                bridge.subscribe();
            }
            else
            {
                cdc.printf("Connection to %s:%d over %s failed\r\n", config.proto.server, config.proto.port, mqtt.name);
                mqtt.disconnect();
            }
        }
        else
        {
            cdc.printf("Connection to %s network failed with status %d\r\n", config.wifi.ssid, status);
        }
    }
    while(true)
    {
        wait(2);
        cdc.printf("Led state: %d\r\n", led.read());
    }

}
