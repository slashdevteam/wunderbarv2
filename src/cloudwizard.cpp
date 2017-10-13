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
char capabilities[8192] = {0};
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

bool getCredentials(IStdInOut& log,
                    NetworkStack* net,
                    const char* userName,
                    const char* token,
                    MqttConfig& mqttConfig,
                    TlsConfig& tlsConfig,
                    RestConfig& restConfig,
                    mbed::DigitalOut& led)
{
    bool credentialsOk = false;
    IStdInOut devNull;
    TLS tls(net, restConfig.tls, &devNull);
    std::memcpy(&restConfig.path, REST_API_PATH, sizeof(REST_API_PATH));
    std::memcpy(&restConfig.server, REST_SERVER, sizeof(REST_SERVER));
    std::memcpy(&restConfig.token, VENDOR_TOKEN, sizeof(VENDOR_TOKEN));
    restConfig.port = REST_PORT;
    std::string registrationUrl(restConfig.path);
    registrationUrl.append("raspberry/email=").append(userName).append("/devices/getCredentials");
    log.printf("\r\nRequesting MQTT credentials.");
    log.printf("\r\nContacting %s\r\n", registrationUrl.c_str());
    HttpsRequest request(tls, "GET", restConfig.server, restConfig.port, registrationUrl.c_str(), nullptr);
    request.setHeader("X-AUTH-TOKEN", restConfig.token);
    request.setHeader("shortToken", token);
    request.setHeader("Accept", "*/*");
    request.setHeader("Accept-Encoding", "gzip, deflate");
    request.setHeader("Accept-Language", "en-us");


    ProgressBar progressBar(log, led, false, 133);
    progressBar.start();
    if(request.send())
    {
        if(request.recv(buffer, sizeof(buffer)))
        {
            HttpParser response(reinterpret_cast<const char*>(buffer));
            if(response)
            {
                char userId[40] = {0};
                char deviceId[30] = {0};
                char authToken[256] = {0};
                JsonDecode message(response.body, 16);
                if(message)
                {
                    message.copyTo("userId", userId, sizeof(userId));
                    message.copyTo("deviceId", deviceId, sizeof(deviceId));
                    message.copyTo("authToken", authToken, sizeof(authToken));
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

                    credentialsOk = true;
                }
                else
                {
                    log.printf("\r\nInvalid data from %s.\r\n", REST_SERVER);
                }
            }
            else
            {
                log.printf("\r\nError response from %s - %d: %.15s.\r\n", REST_SERVER, response.status, response.statusString);
            }
        }
        else
        {
            log.printf("\r\nNo reply from %s.\r\n", REST_SERVER);
        }
    }
    else
    {
        log.printf("\r\nUnable to send data to %s.\r\n", REST_SERVER);
    }
    progressBar.terminate();

    return credentialsOk;
}

bool setDeviceDescription(IStdInOut& log,
                          NetworkStack* net,
                          const char* deviceName,
                          MqttConfig& mqttConfig,
                          RestConfig& restConfig,
                          mbed::DigitalOut& led)
{
    char serialNo[40] = {0};
    uint32_t part0, part1, part2, part3;
    getCpuId(&part0, &part1, &part2, &part3);
    snprintf(serialNo, sizeof(serialNo), "%08lx%8lx%8lx%8lx", part0, part1, part2, part3);

    generateCapabilities(capabilities,
                         sizeof(capabilities),
                         mqttConfig.userId,
                         serialNo,
                         deviceName,
                         mqttConfig.clientId,
                         mqttConfig.password);
    log.printf("\r\nUpdating device capabilities: %s\r\n", capabilities);

    bool capabilitiesOk = false;
    IStdInOut devNull;
    TLS tls(net, restConfig.tls, &devNull);
    std::memcpy(&restConfig.path, REST_API_PATH, sizeof(REST_API_PATH));
    std::memcpy(&restConfig.server, REST_SERVER, sizeof(REST_SERVER));
    std::memcpy(&restConfig.token, VENDOR_TOKEN, sizeof(VENDOR_TOKEN));
    restConfig.port = REST_PORT;
    std::string registrationUrl(restConfig.path);
    registrationUrl.append("users/email=").append(mqttConfig.userId).append("/devices/");
    log.printf("Contacting %s\r\n", registrationUrl.c_str());
    HttpsRequest request(tls, "PUT", restConfig.server, restConfig.port, registrationUrl.c_str(), capabilities);
    request.setHeader("X-AUTH-TOKEN", restConfig.token);

    ProgressBar progressBar(log, led, false, 133);
    progressBar.start();
    if(request.send())
    {
        if(request.recv(buffer, sizeof(buffer)))
        {
            HttpParser response(reinterpret_cast<const char*>(buffer));
            if(response)
            {
                capabilitiesOk = true;
            }
            else
            {
                log.printf("\r\nError response from %s - %d: %.15s.\r\n", REST_SERVER, response.status, response.statusString);
            }
        }
        else
        {
            log.printf("\r\nNo reply from %s.\r\n", REST_SERVER);
        }
    }
    else
    {
        log.printf("\r\nUnable to send data to %s.\r\n", REST_SERVER);
    }
    progressBar.terminate();

    return capabilitiesOk;
}

bool deviceRegistration(IStdInOut& log,
                        NetworkStack* net,
                        MqttConfig& mqttConfig,
                        TlsConfig& tlsConfig,
                        RestConfig& restConfig,
                        mbed::DigitalOut& led)
{
    bool cloudOk = false;
    // collect username & token & device name

    bool userNameOk = false;
    bool tokenOk = false;
    bool deviceNameOk = false;
    bool credentialsOk = false;
    bool deviceDescriptionOk = false;
    char userName[30] = {0};
    char token[9] = {0};
    char deviceName[10] = {0};

    log.printf("\r\nRegistering device using your user email and token.\r\n");
    while(!cloudOk)
    {
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

        // configure TLS for REST calls
        restConfig.tls.caCert = reinterpret_cast<const uint8_t*>(REST_CA_CHAIN);
        restConfig.tls.deviceCert = nullptr;
        restConfig.tls.key = nullptr;
        restConfig.tls.authMode = MBEDTLS_SSL_VERIFY_REQUIRED;
        std::memcpy(&restConfig.tls.deviceId, deviceName, sizeof(deviceName));

        while(!credentialsOk)
        {
            credentialsOk = getCredentials(log, net, userName, token, mqttConfig, tlsConfig, restConfig, led);
            if(!credentialsOk)
            {
                log.printf("Try again? (Y/N)\r\n");
                if(!agree(log, led))
                {
                    return false;
                }
            }
        }

        while(!deviceDescriptionOk)
        {
            deviceDescriptionOk = setDeviceDescription(log, net, deviceName, mqttConfig, restConfig, led);
            // this will currently fail, as it requires an FB-USER-TOKEN unknown to Wunderbar
            if(!deviceDescriptionOk)
            {
                log.printf("\r\nTry again? (Y/N)\r\n");
                if(!agree(log, led))
                {
                    return credentialsOk;
                }
            }
        }

        cloudOk = credentialsOk;
    }

    return cloudOk;
}

bool cloudWizard(NetworkStack* net,
                 MqttConfig& mqttConfig,
                 TlsConfig& tlsConfig,
                 RestConfig& restConfig,
                 mbed::DigitalOut& led,
                 IStdInOut& log)
{
    bool mqttOk = false;
    log.printf("\r\n\r\nNow we will setup communication with Conrad Connect.\r\n");

    if(deviceRegistration(log, net, mqttConfig, tlsConfig, restConfig, led))
    {
        while(!mqttOk)
        {
            IStdInOut devNull;
            TLS tls(net, tlsConfig, &devNull);
            MqttProtocol  mqtt(&tls, mqttConfig, &devNull);
            log.printf("Testing connection to MQTT.\r\n");
            mqttOk = mqtt.connect();
            if(!mqttOk)
            {
                log.printf("MQTT connection failed.\r\n");
                log.printf("\r\nTry again? (Y/N)\r\n");
                if(!agree(log, led))
                {
                    break;
                }
            }
        }
        log.printf("\r\nPerfect! WunderBar successfully connected to MQTT server: %s\r\n", mqttConfig.server);
        log.printf("Your device name is: %s\r\n", mqttConfig.clientId);
    }

    return mqttOk;
}
