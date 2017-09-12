#include "wifiwizard.h"
#include "loopsutil.h"

#include "WiFiInterface.h"
using mbed::DigitalOut;

bool validateSecurity(char c)
{
    bool valid = false;

    if((0x30 <= c) && (0x33 >= c))
    {
        valid = true;
    }

    return valid;
}

bool selectSecurity(IStdInOut& log, nsapi_security_t& security, DigitalOut& led)
{
    bool securityOK = false;
    security = NSAPI_SECURITY_WPA2;
    log.printf("Please select one of the following:\r\n");
    log.printf("0 - for open network\r\n");
    log.printf("1 - for WEP network\r\n");
    log.printf("2 - for WPA network\r\n");
    log.printf("3 - for WPA2 network\r\n");

    char securityText[2] = "3";
    if(readField(log, securityText, 1, 1, "3", &validateSecurity, true, led))
    {
        security = static_cast<nsapi_security_t>(std::atoi(securityText));
        securityOK = true;
    }

    return securityOK;
}

void validCharactersBanner(IStdInOut& log, const char* fieldName, const size_t minCharacters, const size_t maxCharacters)
{
    log.printf("\r\n%s must be between %d and %d characters long, and can contain only:\r\n", fieldName, minCharacters, maxCharacters);
    log.printf(" - digits from 0 to 9\r\n");
    log.printf(" - lower case letters from a to z\r\n");
    log.printf(" - upper case letters from A to Z\r\n");
    log.printf(" - special characters: <SPACE> ! # $ %% & ' ( ) * + , - . / : ; < = > ? @ [ \134 ] ^ _ ` { | } ~ \042 \r\n");
}

bool wifiWizard(WiFiInterface* net, wunderbar::WiFiConfig& config, DigitalOut& led, IStdInOut& log)
{
    bool wifiConnected = false;
    char defaultPassword[20] = "";
    log.printf("\r\nFirst, we need to setup WiFi connection\r\n");
    while(!wifiConnected)
    {
        bool ssidOk = false;
        bool securityOk = false;
        bool passOk = false;
        // keep asking for SSID till valid name is entered
        log.printf("\r\nPlease enter your WiFi SSID below and press ENTER.");
        while(!ssidOk)
        {
            validCharactersBanner(log, "SSID", 1, 31);
            ssidOk = readField(log, config.ssid, 1, 31, config.ssid, &isCharPrintableAscii, true, led);
        }

        log.printf("\r\nOk! WunderBar will try to use WiFi: %s\r\n", config.ssid);
        // keep asking for security till valid one is selected
        log.printf("\r\nNow, please select WiFi security and press ENTER.\r\n");
        while(!securityOk)
        {
            securityOk = selectSecurity(log, config.security, led);
        }

        if(config.security != NSAPI_SECURITY_NONE)
        {
            // keep asking for password till valid one is entered
            log.printf("\r\nPlease enter your WiFi password below and press ENTER.");
            while(!passOk)
            {
                validCharactersBanner(log, "Password", 8, 63);
                log.printf("Characters entered for password will not be echoed!\r\n");
                passOk = readField(log, config.pass, 8, 63, defaultPassword, &isCharPrintableAscii, false, led);
                if(passOk)
                {
                    std::memcpy(defaultPassword, "<previous password>", 19);
                }
            }
        }

        log.printf("\r\n\r\nOk! WunderBar will now try to connect to WiFi: %s\r\n", config.ssid);
        log.printf("Please be patient - this might take w while!\r\n");

        ProgressBar progressBar(log, led);
        progressBar.start();
        int status = net->connect(config.ssid,
                                  config.pass,
                                  config.security,
                                  config.channel);
        progressBar.terminate();
        wifiConnected = (status == NSAPI_ERROR_OK);
        if(!wifiConnected)
        {
            log.printf("\r\nCould not connect to WiFi: %s. \r\nPlease try configuration again.\r\n", config.ssid);
            continue;
        }
    }

    log.printf("\r\nGreat! WunderBar successfully connected to WiFi: %s\r\n", config.ssid);
    log.printf("WunderBar IP: %s\r\n", net->get_ip_address());
    return true;
}

