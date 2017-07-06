#include "mbed.h"

#include "cdc.h"
#include "flash.h"
#include "tls.h"
#include "mqttprotocol.h"
#include "GS1500MInterface.h"
#include "button.h"
#include "led.h"

using usb::CDC;
using wunderbar::Configuration;

// Putting most object in global scope to save thread_stack_main, which is to small!
Flash flash;
const Configuration& config = flash.getConfig();
CDC              cdc;
GS1500MInterface wifiConnection(WIFI_TX, WIFI_RX, 115200);
TLS              tls(&wifiConnection, config.tls, &cdc);
MqttProtocol     mqtt(&tls, config.proto, &cdc);
Button           sw2(&mqtt, "button1", SW2);
Led              led(&mqtt, "actuator/led1", LED1);

int main(int argc, char **argv)
{
    cdc.printf("Welcome to WunderBar v2 mbed OS firmware\n");
    cdc.printf("Running at %d MHz\n", SystemCoreClock/1000000);

    while(true)
    {
        // check if device is already onboarded
        if(flash.isOnboarded())
        {
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
        else
        {
            // here be onboarding
        }
        wait(2);
        cdc.printf("Led state: 0x%x\r\n", flash.getStorage().marker);
    }

}
