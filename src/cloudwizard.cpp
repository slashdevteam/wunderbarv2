#include "cloudwizard.h"
#include "loopsutil.h"
#include "httpsrequest.h"
#include "httpparser.h"
#include "NetworkStack.h"
#include "base64.h"
#include <cstdarg>
#include "mbed_wait_api.h"
#include "cdc.h"
#include "resources.h"
#include "jsondecode.h"

using usb::CDC;
extern CDC cdc;
extern Resources resources;
using mbed::DigitalOut;

#include "cloudconfig.cpp"


extern "C" WEAK void getCpuId(uint32_t* part0,
                              uint32_t* part1,
                              uint32_t* part2,
                              uint32_t* part3)
{
    *part0 = 0x42C0FFEE;
    *part1 = 0xC0FFEE42;
    *part2 = 0x42C0FFEE;
    *part3 = 0xC0FFEE42;
}

// capabilities JSON can be humongous so creating
// static buffer
char capabilities[2048] = {0};
uint8_t buffer[2048] = {0};

bool validateOnboardChoice(char c)
{
    bool valid = false;

    if((0x30 <= c) && (0x31 >= c))
    {
        valid = true;
    }

    return valid;
}

size_t generateCapabilities(char* caps,
                            size_t maxSize,
                            const char* userId,
                            const char* serialNo,
                            const char* name,
                            const char* deviceId,
                            const char* authToken)
{
    const char capsHeaderFormat[] =
    "{"
    "\"id\":\"%s\","
    "\"serialNo\":\"%s\","
    "\"name\":\"%s\","
    "\"userId\":\"%s\","
    "\"authToken\":\"%s\","
    "\"type\":\"%s\","
    "\"productCode\":\"%s\","
    "\"version\":\"%s\","
    "\"deviceState\":\"Alright\","
    "\"lastHeartbeatTime\":%ld,"
    "\"sensors\":[";

    size_t outLen = std::snprintf(caps,
                                maxSize,
                                capsHeaderFormat,
                                deviceId,
                                serialNo,
                                name,
                                userId,
                                authToken,
                                PRODUCT_TYPE,
                                PRODUCT_CODE,
                                PRODUCT_VERSION,
                                time(nullptr));

    for(auto resource : resources.current)
    {
        const char* spec = resource->getSenseSpec();
        if(spec && std::strlen(spec) > 0)
        {
            outLen += std::snprintf(caps + outLen,
                                    maxSize - outLen,
                                    "%s,",
                                    spec);
        }
    }

    outLen += std::snprintf(caps + outLen - 1,
                                maxSize - outLen,
                                "],\"actuators\":[");

    outLen -= 1;
    for(auto resource : resources.current)
    {
        const char* spec = resource->getActuateSpec();
        if(spec && std::strlen(spec) > 0)
        {
            outLen += std::snprintf(caps + outLen,
                                    maxSize - outLen,
                                    "%s,",
                                    spec);
        }
    }

    outLen += std::snprintf(caps + outLen - 1,
                            maxSize - outLen,
                            "]}");
    return outLen;
}

#include "mbed_stats.h"

bool deviceRegistration(NetworkStack* net, MqttConfig& mqttConfig, TlsConfig& tlsConfig, DigitalOut& led)
{
    // collect username & token & device name
    mbed_stats_heap_t heap_stats;
    mbed_stats_heap_get(&heap_stats);

    mbed_stats_stack_t stack_stats;
    mbed_stats_stack_get(&stack_stats);

    bool responseOk = false;
    bool userNameOk = false;
    bool tokenOk = false;
    bool deviceNameOk = false;
    char userName[30] = {0};
    char token[9] = {0};
    char deviceName[10] = {0};

    while(!userNameOk)
    {
        cdc.printf("Please enter your user email:\r\n");
        userNameOk = readField(userName, 5, 30, userName, &isCharPrintableAscii, true, led);
    }

    while(!tokenOk)
    {
        cdc.printf("Please enter your token:\r\n");
        tokenOk = readField(token, 6, 8, token, &isCharPrintableAscii, true, led);
    }

    while(!deviceNameOk)
    {
        cdc.printf("Please enter your device name:\r\n");
        deviceNameOk = readField(deviceName, 1, 10, deviceName, &isCharPrintableAscii, true, led);
    }

    TlsConfig restTlsConfig;
    restTlsConfig.caCert = reinterpret_cast<const uint8_t*>(REST_CA_CHAIN);
    restTlsConfig.deviceCert = nullptr;
    restTlsConfig.key = nullptr;
    restTlsConfig.authMode = MBEDTLS_SSL_VERIFY_REQUIRED;
    std::memcpy(&restTlsConfig.deviceId, deviceName, sizeof(deviceName));
    TLS tls(net, restTlsConfig, &cdc);
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
            HttpParser response(reinterpret_cast<const char*>(buffer));
            cdc.printf("STATUS: %d\r\n", response.status);
            if(response)
            {
                cdc.printf("BODY: %s\r\n", response.body);
                char userId[40] = {0};
                char serialNo[40] = {0};
                uint32_t part0, part1, part2, part3;
                getCpuId(&part0, &part1, &part2, &part3);
                snprintf(serialNo, sizeof(serialNo), "%08lx%8lx%8lx%8lx", part0, part1, part2, part3);
                char deviceId[30] = {0};
                char authToken[256] = {0};
                JsonDecode message(response.body, 16);
                if(message)
                {
                    message.copyTo("userId", userId, sizeof(userId));
                    message.copyTo("deviceId", deviceId, sizeof(deviceId));
                    message.copyTo("authToken", authToken, sizeof(authToken));
                    // generate it for user knowledge till an API for Wunderbar is available
                    generateCapabilities(capabilities,
                                         sizeof(capabilities),
                                         userId,
                                         serialNo,
                                         deviceName,
                                         deviceId,
                                         authToken);
                    cdc.printf("Current device capabilities: %s \r\n", capabilities);
                    // prepare configuration
                    tlsConfig.caCert = reinterpret_cast<const uint8_t*>(MQTT_CERT);
                    tlsConfig.deviceCert = nullptr;
                    tlsConfig.key = nullptr;
                    tlsConfig.authMode = MBEDTLS_SSL_VERIFY_REQUIRED;
                    std::memcpy(&tlsConfig.deviceId, deviceId, sizeof(tlsConfig.deviceId));

                    mqttConfig.port = MQTT_PORT;
                    std::memcpy(&mqttConfig.server, MQTT_SERVER, std::strlen(MQTT_SERVER));
                    std::memcpy(&mqttConfig.clientId, userId, std::strlen(deviceId));
                    std::memcpy(&mqttConfig.userId, userId, std::strlen(userId));
                    std::memcpy(&mqttConfig.password, authToken, std::strlen(authToken));

                    responseOk = true;
                }
                else
                {
                    cdc.printf("Invalid data from %s\r\n", REST_SERVER);
                }
            }
            else
            {
                cdc.printf("Error response from %s - %d: %s\r\n", REST_SERVER, response.status, response.statusString);
            }
        }
        else
        {
            cdc.printf("No reply from %s\r\n", REST_SERVER);
        }
    }
    else
    {
        cdc.printf("Unable to connect to %s\r\n", REST_SERVER);
    }

    return responseOk;
}

bool cloudWizard(NetworkStack* net, MqttConfig& mqttConfig, TlsConfig& tlsConfig, DigitalOut& led)
{
    bool cloudOk = false;
    bool mqttOk = false;
    cdc.printf("\r\n\r\nNow we will setup communication with cloud.\r\n");

    cdc.printf("Registering device using your user email and token\r\n");
    while(!cloudOk)
    {
        cloudOk = deviceRegistration(net, mqttConfig, tlsConfig, led);
    }

    while(!mqttOk)
    {
        TLS tls(net, tlsConfig, &cdc);
        MqttProtocol  mqtt(&tls, mqttConfig, &cdc);
        mqttOk = mqtt.connect();
        if(!mqttOk)
        {
            cdc.printf("MQTT connection failed. Trying again in 1s\r\n");
            wait(1);
            // @TODO: maybe ask user should we continue or abort?
        }
    }
    cdc.printf("\r\nPerfect! WunderBar successfully connected to MQTT server: %s\r\n", mqttConfig.server);
    cdc.printf("Your device name is: \r\n", mqttConfig.clientId);

    return true;
}
