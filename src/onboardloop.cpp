#include "loops.h"

#include "DigitalOut.h"
#include "cdc.h"
#include "GS1500MInterface.h"
#include "configuration.h"

extern GS1500MInterface wifiConnection;
using usb::CDC;
extern CDC cdc;

const char DEFAULT_ROOT_CA[] = "-----BEGIN CERTIFICATE-----\n"
              "-----END CERTIFICATE-----\0";
const char DEFAULT_SERVER[] = "";
const uint32_t DEFAULT_REST_PORT = 8080;
const uint32_t DEFAULT_MQTT_PORT = 8883;

bool validateCharacter(char c)
{
    bool valid = false;

    // 0 - 9
    if((0x30 <= c) && (0x39 >= c))
    {
        valid = true;
    }

    // A - Z
    if((0x41 <= c) && (0x5A >= c))
    {
        valid = true;
    }

    // a - z
    if((0x61 <= c) && (0x7A >= c))
    {
        valid = true;
    }

    // SP ! " # $ % & ' ( ) * + , - . /
    if((0x20 <= c) && (0x2F >= c))
    {
        valid = true;
    }

    // : ; < = > ? @
    if((0x3A <= c) && (0x40 >= c))
    {
        valid = true;
    }

    // [ \ ] ^ _ `
    if((0x5B <= c) && (0x60 >= c))
    {
        valid = true;
    }

    // { | } ~
    if((0x7B <= c) && (0x7E >= c))
    {
        valid = true;
    }

    return valid;
}

bool read(char* field,
          DigitalOut& led,
          const size_t minCharacters,
          const size_t maxCharacters,
          const char* fieldName,
          const char* defaultValue,
          bool echo)
{
    cdc.printf("\r\n%s must be between %d and %d characters long, and can contain only:\r\n", fieldName, minCharacters, maxCharacters);
    cdc.printf(" - digits from 0 to 9\r\n");
    cdc.printf(" - lower case letters from a to z\r\n");
    cdc.printf(" - upper case letters from A to Z\r\n");
    cdc.printf(" - special characters: <SPACE> ! # $ %% & ' ( ) * + , - . / : ; < = > ? @ [ \134 ] ^ _ ` { | } ~ \042 \r\n");
    if(!echo)
    {
        cdc.printf("Characters entered for %s will not be echoed!\r\n", fieldName);
    }
    cdc.printf("[Default: %s]> ", defaultValue);

    size_t defaultLength = std::strlen(defaultValue);
    size_t numCharacters = 0;

    bool gotNewline = false;
    bool ssidOk = true;
    while((numCharacters <= maxCharacters))
    {
        char newChar = cdc.getc();
        if(echo && !((0x8 == newChar) ||  (0x7F == newChar)))
        {
            // echo back to terminal
            cdc.putc(newChar);
        }
        // blink led on character entry
        led = !led;

        if(validateCharacter(newChar))
        {
            field[numCharacters] = newChar;
            numCharacters++;
        }
        else
        {
            // '\n' or '\r' finishes
            if((0xA == newChar) ||  (0xD == newChar))
            {
                gotNewline = true;
                // ensure '\0' at the end
                // user used default value
                if(0 == numCharacters)
                {
                    // there is still space left for '\0' in buffer
                    if(defaultLength < maxCharacters)
                    {
                        field[defaultLength] = '\0';
                    }
                }
                if((numCharacters < maxCharacters))
                {
                    field[numCharacters] = '\0';
                }
                break;
            }

            // handle backspace (0x8 or 0x7f)
            if((0x8 == newChar) ||  (0x7F == newChar))
            {
                cdc.putc('\b');
                cdc.putc(' ');
                cdc.putc('\b');
                if(0 != numCharacters)
                {
                    numCharacters--;
                }
                continue;
            }

            // must ignore empty read
            if(0x0 != newChar)
            {
                // invalid character received
                cdc.printf("\r\n Invalid character 0x%x in %s!\r\n", newChar, fieldName);
                break;
            }
        }
    }

    // check if any character was given
    if(0 == numCharacters && 0 != defaultLength)
    {
        ssidOk = false;
        cdc.printf("\r\nNo characters were given!\r\n");
    }
    // check if password was not too long
    else if((numCharacters < minCharacters) && gotNewline)
    {
        ssidOk = false;
        cdc.printf("\r\n%s too short!\r\n", fieldName);
    }
    // check if password was not too long
    else if((numCharacters == maxCharacters) && !gotNewline)
    {
        ssidOk = false;
        cdc.printf("\r\n%s too long!\r\n", fieldName);
    }
    // invalid character
    else if(!gotNewline)
    {
        ssidOk = false;
        cdc.printf("\r\nInvalid characters in %s!\r\n", fieldName);
    }

    return ssidOk;
}

bool selectSecurity(nsapi_security_t& security, DigitalOut& led)
{
    security = NSAPI_SECURITY_WPA2;
    cdc.printf("Please select one of the following:\r\n");
    cdc.printf("0 - for open network\r\n");
    cdc.printf("1 - for WEP network\r\n");
    cdc.printf("2 - for WPA network\r\n");
    cdc.printf("3 - for WPA2 network\r\n");
    cdc.printf("[Default: 3]> ");


    bool isValidSelected = true;
    bool gotNewline = false;
    while(!gotNewline || !isValidSelected)
    {
        char newChar = cdc.getc();
        if((0x8 != newChar) && (0x7F != newChar))
        {
            cdc.putc(newChar);
        }
        led = !led;

        // handle backspace (0x8 or 0x7f)
        if((0x8 == newChar) || (0x7F == newChar))
        {
            cdc.putc('\b');
            cdc.putc(' ');
            cdc.putc('\b');
            continue;
        }

        // '\n' or '\r' finishes
        if((0xA == newChar) ||  (0xD == newChar))
        {
            gotNewline = true;
            continue;
        }
        // validate range: 0 - 3
        if((0x30 <= newChar) && (0x33 >= newChar))
        {
            isValidSelected = true;
            security = static_cast<nsapi_security_t>(std::atoi(&newChar));
        }
        else
        {
            isValidSelected = false;
            cdc.printf("\r\nInvalid selection: %d. Please chose 0, 1, 2 or 3 and hit ENTER.\r\n", std::atoi(&newChar));
            cdc.printf("[Default: 3]> ");
        }

    }


    return true;
}

void progressDots()
{
    while(true)
    {
        cdc.printf(".");
        wait(1);
    }
}

bool wifiWizard(wunderbar::WiFiConfig& config, DigitalOut& led)
{
    bool wifiOk = false;

    std::memset(&config, 0, sizeof(config));
    while(!wifiOk)
    {
        bool ssidOk = false;
        bool passOk = false;
        // keep asking for SSID till valid name is entered
        cdc.printf("\r\nFirst, we need to setup WiFi connection, so please enter your WiFi SSID below and press ENTER.\r\n");
        while(!ssidOk)
        {
            ssidOk = read(config.ssid, led, 1, 31, "SSID", config.ssid, true);
        }

        cdc.printf("Ok! WunderBar will try to use WiFi: %s\r\n\r\n", config.ssid);
        // keep asking for security till valid one is selected
        cdc.printf("\r\nNow, please select WiFi security and press ENTER.\r\n");
        selectSecurity(config.security, led);

        // keep asking for password till valid one is entered
        while(!passOk)
        {
            passOk = read(config.pass, led, 8, 63, "Password", "<previous password>", false);
        }

        cdc.printf("\r\n\r\nOk! WunderBar will now try to connect to WiFi: %s\r\n", config.ssid);
        cdc.printf("Please be patient - this might take w while!\r\n");

        rtos::Thread progressBar(osPriorityNormal);
        progressBar.start(&progressDots);
        int status = wifiConnection.connect(config.ssid,
                                            config.pass,
                                            config.security,
                                            config.channel);
        progressBar.terminate();
        wifiOk = (status == NSAPI_ERROR_OK);
        if(!wifiOk)
        {
            cdc.printf("\r\nCould not connect to WiFi: %s. \r\nPlease try configuration again.\r\n", config.ssid);
            continue;
        }
    }

    cdc.printf("\r\nGreat! WunderBar successfully connected to WiFi: %s\r\n", config.ssid);
    cdc.printf("WunderBar IP: %s\r\n", wifiConnection.get_ip_address());
    return true;
}

bool getLine(char* buffer, size_t maxLen, const char* defaultValue, const char* name, DigitalOut& led)
{
    cdc.printf("\r\nPlease enter %s (maximum lenght: %d) and press ENTER.\r\n", name, maxLen);
    cdc.printf("[Default: %s]> ", defaultValue);
    bool lineOk = false;
    size_t numCharacters = 0;
    while(!lineOk || (numCharacters >= maxLen))
    {
        char newChar = cdc.getc();
        if((0x8 != newChar) && (0x7F != newChar))
        {
            cdc.putc(newChar);
        }
        led = !led;
        // handle backspace (0x8 or 0x7f)
        if((0x8 == newChar) || (0x7F == newChar))
        {
            cdc.putc('\b');
            cdc.putc(' ');
            cdc.putc('\b');
            if(0 != numCharacters)
            {
                buffer[numCharacters] = '\0';
                numCharacters--;
            }
            continue;
        }

        // '\n' or '\r' finishes
        if((0xA == newChar) ||  (0xD == newChar))
        {
            lineOk = true;
            continue;
        }
        buffer[numCharacters] = newChar;
        numCharacters++;
    }

    return lineOk;
}

void getCertDn(const uint8_t* cert, size_t certLen, char* buffer, size_t bufferLen)
{
    mbedtls_x509_crt ca;
    mbedtls_x509_crt_init(&ca);
    mbedtls_x509_crt_parse(&ca, cert, certLen + 1);
    mbedtls_x509_dn_gets(buffer, bufferLen, &ca.subject);
}

bool cloudWizard(MqttConfig& mqttConfig, TlsConfig& tlsConfig, DigitalOut& led)
{
    bool cloudOk = false;
    cdc.printf("\r\nNow we will setup communication with cloud.\r\n");
    cdc.printf("All addresses assume encrypted protocols (HTTPS/MQTT with TLS)!\r\n");

    // initialize defaults
    std::memcpy(mqttConfig.server, DEFAULT_SERVER, sizeof(DEFAULT_SERVER));
    char portText[6] = {0};
    std::snprintf(portText, sizeof(portText), "%lu", DEFAULT_REST_PORT);
    std::memcpy(&tlsConfig.caCert, DEFAULT_ROOT_CA, sizeof(DEFAULT_ROOT_CA));
    char rootCaDn[40] = {0};
    getCertDn(tlsConfig.caCert, sizeof(DEFAULT_ROOT_CA), rootCaDn, sizeof(rootCaDn));

    while(!cloudOk)
    {
        bool serverAddressOk = false;
        bool serverPortOk = false;
        bool certOk = false;
        while(!serverAddressOk)
        {
            serverAddressOk = getLine(mqttConfig.server,
                              sizeof(mqttConfig.server),
                              mqttConfig.server,
                              "Cloud server address",
                              led);
        }

        while(!serverPortOk)
        {
            serverPortOk = getLine(portText,
                              sizeof(portText),
                              portText,
                              "Cloud server port",
                              led);
            uint32_t port = std::atoi(portText);
            // port needs to be in range 1 - 65535 and cannot be between 47808 and 47823 (GS1500M limitation)
            if(port > 65535 || ((port >= 47808) && (port <= 47823)) || port == 0)
            {
                serverPortOk = false;
                cdc.printf("Port number has to be between 1 - 65535, and not in [47808, 47823] range!");
                continue;
            }
            mqttConfig.port = port;
        }

        while(!certOk)
        {
            certOk = getLine(reinterpret_cast<char*>(tlsConfig.caCert),
                              sizeof(tlsConfig.caCert),
                              rootCaDn,
                              "Cloud server certificate",
                              led);
        }


    }
    return true;
}

void onboard()
{
    // let's wait till user connects to CDC console
    DigitalOut led(LED1);
    while(-1 == cdc.putc('\n'))
    {
        wait(0.3);
        led = !led;
    }

    // configuration can be big (SSL certs) and we need stack for TLS, so allocate space for config on heap
    auto wunderConfig = std::make_unique<wunderbar::Configuration>();

    cdc.printf("Welcome to WunderBar V2 onboarding wizard!\r\n");
    cdc.printf("This app will guide you through the process of onboarding your device.\r\n");
    cdc.printf("Press ENTER to continue.\r\n");
    cdc.getc();
    // wizards are blocking till successful completion
    // wifiWizard(wunderConfig->wifi, led);
    cloudWizard(wunderConfig->proto, wunderConfig->tls, led);

    while(true)
    {
        wait(2);
        cdc.printf("Onboard state: %d\r\n", 1);
    }
}

void onboardLoop()
{
    rtos::Thread onboarder(osPriorityNormal, 0x4000);
    onboarder.start(&onboard);
    onboarder.join();
}

