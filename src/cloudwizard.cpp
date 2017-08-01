#include "cloudwizard.h"
#include "loopsutil.h"
#include "https_request.h"
#include "NetworkInterface.h"
#include "base64.h"
#include <cstdarg>
#include "mbed_wait_api.h"
#include "cdc.h"
using usb::CDC;
extern CDC cdc;

using mbed::DigitalOut;
using DeviceOnboard = mbed::Callback<bool(NetworkInterface*, MqttConfig&, TlsConfig&, DigitalOut&)>;

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

bool newDeviceOnboard(NetworkInterface* net, MqttConfig& mqttConfig, TlsConfig& tlsConfig, DigitalOut& led)
{
    // collect username & token
    bool userNameOk = false;
    bool tokenOk = false;
    char userName[30] = {"wunderbar"};
    char token[9] = {"conrad"};
    while(!userNameOk)
    {
        cdc.printf("Please enter your username:\r\n");
        userNameOk = readField(userName, 5, 30, userName, &isCharPrintableAscii, true, led);
    }

    while(!tokenOk)
    {
        cdc.printf("Please enter your token:\r\n");
        tokenOk = readField(token, 6, 8, token, &isCharPrintableAscii, true, led);
    }

    std::string registrationUrl = "https://";
    registrationUrl.append(DEFAULT_SERVER);
    registrationUrl.append(":");
    registrationUrl.append(std::to_string(DEFAULT_REST_PORT));
    registrationUrl.append("/secret");
    cdc.printf("Contacting %s\r\n", registrationUrl.c_str());

    HttpsRequest registrationReq(net, reinterpret_cast<const char*>(tlsConfig.caCert), HTTP_GET, registrationUrl.c_str(), httpsRequestCallback);
    // create Authorization header
    std::string userToken(userName);
    userToken.append(":");
    userToken.append(token);
    uint8_t userTokenB64[70] = {"Basic "};
    size_t bytesOut = 0;
    mbedtls_base64_encode(&userTokenB64[6], sizeof(userTokenB64) - 6, &bytesOut, reinterpret_cast<const uint8_t*>(userToken.c_str()), userToken.size());
    registrationReq.set_header("Authorization", reinterpret_cast<const char*>(userTokenB64));
    // send request and wait till callback clears flag
    HttpResponse* resp = registrationReq.send();

    while(!responseReceived)
    {
        led = !led;
        wait(0.1);
    }
    return responseOk;
}

bool existingDeviceOnboard(NetworkInterface* net, MqttConfig& mqttConfig, TlsConfig& tlsConfig, DigitalOut& led)
{
    return false;
}

int cdcPrintfRetarget(const char *format, ...)
{
    char internalBuffer[256];
    std::va_list arg;
    va_start(arg, format);
    // ARMCC microlib does not properly handle a size of 0.
    // As a workaround supply a dummy buffer with a size of 1.
    char dummy_buf[1];
    int len = vsnprintf(dummy_buf, sizeof(dummy_buf), format, arg);
    if(static_cast<size_t>(len) < sizeof(internalBuffer))
    {
        vsprintf(internalBuffer, format, arg);
        cdc.puts(internalBuffer);
    }
    else
    {
        char *temp = new char[len + 1];
        vsprintf(temp, format, arg);
        cdc.puts(temp);
        delete[] temp;
    }
    va_end(arg);
    return len;
}

bool cloudWizard(NetworkInterface* net, MqttConfig& mqttConfig, TlsConfig& tlsConfig, DigitalOut& led)
{
    mbedtls_platform_set_printf(&cdcPrintfRetarget);
    bool cloudOk = false;
    cdc.printf("\r\nNow we will setup communication with cloud.\r\n");

    // initialize defaults
    std::memcpy(mqttConfig.server, DEFAULT_SERVER, sizeof(DEFAULT_SERVER));
    char portText[6] = {0};
    std::snprintf(portText, sizeof(portText), "%lu", DEFAULT_REST_PORT);
    std::memcpy(&tlsConfig.caCert, DEFAULT_ROOT_CA, sizeof(DEFAULT_ROOT_CA));

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

            char methodText[2] = "";
            if(readField(methodText, 1, 1, "0", &validateOnboardChoice, true, led))
            {
                method = std::atoi(methodText);
                methodOk = true;
            }
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
