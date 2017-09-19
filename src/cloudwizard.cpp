#include "cloudwizard.h"
#include "loopsutil.h"
#include "httpsrequest.h"
#include "httpparser.h"
#include "NetworkStack.h"
#include "base64.h"
#include <cstdarg>
#include "mbed_wait_api.h"
#include "resources.h"
#include "jsondecode.h"

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
char capabilities[4096] = {0};
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

    size_t spurComaLen = 0;

    for(auto resource : resources.current)
    {
        size_t resSpecLen = resource->getSenseSpec(caps + outLen, maxSize - outLen);

        if(resSpecLen > 0)
        {
            outLen += resSpecLen;
            outLen += std::snprintf(caps + outLen, maxSize - outLen, ",");

            // i.e. "if there is at least one coma"
            spurComaLen = 1;
        }
    }
    
    outLen -= spurComaLen;

    outLen += std::snprintf(caps + outLen,
                            maxSize - outLen,
                            "],\"actuators\":[");

    spurComaLen = 0;

    for(auto resource : resources.current)
    {
        size_t resSpecLen = resource->getActuateSpec(caps + outLen, maxSize - outLen);
            
        if(resSpecLen > 0)
        {
            outLen += resSpecLen;
            outLen += std::snprintf(caps + outLen, maxSize - outLen, ",");

            // i.e. "if there is at least one coma"
            spurComaLen = 1;
        }
    }

    outLen -= spurComaLen;

    outLen += std::snprintf(caps + outLen,
                            maxSize - outLen,
                            "]}");

    return outLen;
}

#include "mbed_stats.h"

bool deviceRegistration(IStdInOut& log,
                        NetworkStack* net,
                        MqttConfig& mqttConfig,
                        TlsConfig& tlsConfig,
                        RestConfig& restConfig,
                        mbed::DigitalOut& led)
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
        log.printf("Please enter your user email:\r\n");
        userNameOk = readField(log, userName, 5, 30, userName, &isCharPrintableAscii, true, led);
    }

    while(!tokenOk)
    {
        log.printf("Please enter your token:\r\n");
        tokenOk = readField(log, token, 6, 8, token, &isCharPrintableAscii, true, led);
    }

    while(!deviceNameOk)
    {
        log.printf("Please enter your device name:\r\n");
        deviceNameOk = readField(log, deviceName, 1, 10, deviceName, &isCharPrintableAscii, true, led);
    }

    restConfig.tls.caCert = reinterpret_cast<const uint8_t*>(REST_CA_CHAIN);
    restConfig.tls.deviceCert = nullptr;
    restConfig.tls.key = nullptr;
    restConfig.tls.authMode = MBEDTLS_SSL_VERIFY_REQUIRED;
    std::memcpy(&restConfig.tls.deviceId, deviceName, sizeof(deviceName));
    TLS tls(net, restConfig.tls, &log);
    std::memcpy(&restConfig.path, REST_API_PATH, sizeof(REST_API_PATH));
    std::memcpy(&restConfig.server, REST_SERVER, sizeof(REST_SERVER));
    std::memcpy(&restConfig.token, VENDOR_TOKEN, sizeof(VENDOR_TOKEN));
    restConfig.port = REST_PORT;
    std::string registrationUrl(restConfig.path);
    registrationUrl.append("raspberry/email=").append(userName).append("/devices/getCredentials");
    log.printf("Contacting %s\r\n", registrationUrl.c_str());
    HttpsRequest request(tls, "GET", restConfig.server, restConfig.port, registrationUrl.c_str(), nullptr);
    request.setHeader("X-AUTH-TOKEN", restConfig.token);
    request.setHeader("shortToken", token);
    request.setHeader("Accept", "*/*");
    request.setHeader("Accept-Encoding", "gzip, deflate");
    request.setHeader("Accept-Language", "en-us");

    if(request.send())
    {
        if(request.recv(buffer, sizeof(buffer)))
        {
            HttpParser response(reinterpret_cast<const char*>(buffer));
            log.printf("STATUS: %d\r\n", response.status);
            if(response)
            {
                log.printf("BODY: %s\r\n", response.body);
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
                    log.printf("Current device capabilities: %s \r\n", capabilities);
                    // prepare configuration
                    tlsConfig.caCert = reinterpret_cast<const uint8_t*>(MQTT_CERT);
                    tlsConfig.deviceCert = nullptr;
                    tlsConfig.key = nullptr;
                    tlsConfig.authMode = MBEDTLS_SSL_VERIFY_REQUIRED;
                    std::memcpy(&tlsConfig.deviceId, deviceId, sizeof(tlsConfig.deviceId));

                    mqttConfig.port = MQTT_PORT;
                    std::memcpy(&mqttConfig.server, MQTT_SERVER, std::strlen(MQTT_SERVER));
                    std::memcpy(&mqttConfig.clientId, deviceId, std::strlen(deviceId));
                    std::memcpy(&mqttConfig.userId, userId, std::strlen(userId));
                    std::memcpy(&mqttConfig.password, authToken, std::strlen(authToken));

                    responseOk = true;
                }
                else
                {
                    log.printf("Invalid data from %s\r\n", REST_SERVER);
                }
            }
            else
            {
                log.printf("Error response from %s - %d: %s\r\n", REST_SERVER, response.status, response.statusString);
            }
        }
        else
        {
            log.printf("No reply from %s\r\n", REST_SERVER);
        }
    }
    else
    {
        log.printf("Unable to connect to %s\r\n", REST_SERVER);
    }

    return responseOk;
}

bool cloudWizard(NetworkStack* net,
                 MqttConfig& mqttConfig,
                 TlsConfig& tlsConfig,
                 RestConfig& restConfig,
                 mbed::DigitalOut& led,
                 IStdInOut& log)
{
    bool cloudOk = false;
    bool mqttOk = false;
    log.printf("\r\n\r\nNow we will setup communication with cloud.\r\n");

    log.printf("Registering device using your user email and token\r\n");
    while(!cloudOk)
    {
        cloudOk = deviceRegistration(log, net, mqttConfig, tlsConfig, restConfig, led);
    }

    while(!mqttOk)
    {
        TLS tls(net, tlsConfig, &log);
        MqttProtocol  mqtt(&tls, mqttConfig, &log);
        mqttOk = mqtt.connect();
        if(!mqttOk)
        {
            log.printf("MQTT connection failed. Trying again in 1s\r\n");
            wait(1);
            // @TODO: maybe ask user should we continue or abort?
        }
    }
    log.printf("\r\nPerfect! WunderBar successfully connected to MQTT server: %s\r\n", mqttConfig.server);
    log.printf("Your device name is: %s\r\n", mqttConfig.clientId);

    return true;
}
