#include "cloudwizard.h"
#include "loopsutil.h"
#include "httpsrequest.h"
#include "httpparser.h"
#include "NetworkStack.h"
#include "base64.h"
#include <cstdarg>
#include "mbed_wait_api.h"
#include "cdc.h"
using usb::CDC;
extern CDC cdc;

using mbed::DigitalOut;
using DeviceOnboard = mbed::Callback<bool(NetworkStack*, MqttConfig&, TlsConfig&, DigitalOut&)>;

#include "cloudconfig.cpp"

volatile bool responseReceived = false;
volatile bool responseOk = false;

bool validateOnboardChoice(char c)
{
    bool valid = false;

    if((0x30 <= c) && (0x31 >= c))
    {
        valid = true;
    }

    return valid;
}

void httpsRequestCallback(const char *at, size_t length)
{
    cdc.printf("%s\r\n", at);
}

bool newDeviceOnboard(NetworkStack* net, MqttConfig& mqttConfig, TlsConfig& tlsConfig, DigitalOut& led)
{
    // collect username & token
    bool userNameOk = false;
    bool tokenOk = false;
    char userName[30] = {"team@slashdev.team"};
    char token[9] = {"DnxwmNw"};
    // while(!userNameOk)
    // {
    //     cdc.printf("Please enter your username:\r\n");
    //     userNameOk = readField(userName, 5, 30, userName, &isCharPrintableAscii, true, led);
    // }

    // while(!tokenOk)
    // {
    //     cdc.printf("Please enter your token:\r\n");
    //     tokenOk = readField(token, 6, 8, token, &isCharPrintableAscii, true, led);
    // }

    uint8_t buffer[2048] = {0};

    tlsConfig.caCert = reinterpret_cast<const uint8_t*>(REST_CA_CHAIN);
    TLS tls(net, tlsConfig, &cdc);
    std::string registrationUrl = REST_API_PATH;
    registrationUrl.append("raspberry/email=").append(userName).append("/devices/getCredentials");
    cdc.printf("Contacting %s\r\n", registrationUrl.c_str());
    HttpsRequest request(tls, "GET", REST_SERVER, REST_PORT, registrationUrl.c_str(), nullptr);
    request.setHeader("X-AUTH-TOKEN", VENDOR_TOKEN);
    request.setHeader("shortToken", token);
    request.setHeader("Accept", "*/*");
    request.setHeader("Accept-Encoding", "gzip, deflate");
    request.setHeader("Accept-Language", "en-us");

    if(request.send())
    {
        if(request.recv(buffer, sizeof(buffer)))
        {
            cdc.printf("REPLY: %s\r\n", buffer);
            HttpParser response(reinterpret_cast<const char*>(buffer));
            cdc.printf("STATUS: %d\r\n", response.status);
            if(response)
            {
                cdc.printf("BODY: %s\r\n", response.body);
            }
        }
    }

    while(!responseReceived)
    {
        led = !led;
        wait(2);
    }
    return responseOk;
}

bool existingDeviceOnboard(NetworkStack* net, MqttConfig& mqttConfig, TlsConfig& tlsConfig, DigitalOut& led)
{
    return false;
}

bool cloudWizard(NetworkStack* net, MqttConfig& mqttConfig, TlsConfig& tlsConfig, DigitalOut& led)
{
    bool cloudOk = false;
    cdc.printf("\r\nNow we will setup communication with cloud.\r\n");

    // initialize defaults
    std::memcpy(mqttConfig.server, MQTT_SERVER, sizeof(MQTT_SERVER));
    // char portText[6] = {0};
    // std::snprintf(portText, sizeof(portText), "%lu", DEFAULT_REST_PORT);
    // std::memcpy(&tlsConfig.caCert, DEFAULT_ROOT_CA, sizeof(DEFAULT_ROOT_CA));

    const DeviceOnboard onboardMethods[] = {&newDeviceOnboard, &existingDeviceOnboard};
    while(!cloudOk)
    {
        bool methodOk = false;
        uint32_t method = 0;
        while(!methodOk)
        {
            cdc.printf("Would you like to:\r\n");
            cdc.printf("0 - register new device using your username and token\r\n");
            cdc.printf("1 - onboard with existing device using your device id and token\r\n");

            // char methodText[2] = "";
            // if(readField(methodText, 1, 1, "0", &validateOnboardChoice, true, led))
            // {
            //     method = std::atoi(methodText);
                methodOk = true;
            // }
        }

        bool onboardOk = false;

        while(!onboardOk)
        {
            onboardMethods[method](net, mqttConfig, tlsConfig, led);
        }

    }

    cdc.printf("\r\nPerfect! WunderBar successfully connected to MQTT server: %s\r\n", mqttConfig.server);
    cdc.printf("Your device name is: \r\n", mqttConfig.clientId);

    return true;
}
