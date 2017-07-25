#include "wifiwizard.h"
#include "loopsutil.h"

#include "cdc.h"
using usb::CDC;
extern CDC cdc;

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

bool selectSecurity(nsapi_security_t& security, DigitalOut& led)
{
    bool securityOK = false;
    security = NSAPI_SECURITY_WPA2;
    cdc.printf("Please select one of the following:\r\n");
    cdc.printf("0 - for open network\r\n");
    cdc.printf("1 - for WEP network\r\n");
    cdc.printf("2 - for WPA network\r\n");
    cdc.printf("3 - for WPA2 network\r\n");

    char securityText[2] = "";
    if(readField(securityText, 1, 1, "3", &validateSecurity, true, led))
    {
        security = static_cast<nsapi_security_t>(std::atoi(securityText));
        securityOK = true;
    }

    return securityOK;
}

void validCharactersBanner(const char* fieldName, const size_t minCharacters, const size_t maxCharacters)
{
    cdc.printf("\r\n%s must be between %d and %d characters long, and can contain only:\r\n", fieldName, minCharacters, maxCharacters);
    cdc.printf(" - digits from 0 to 9\r\n");
    cdc.printf(" - lower case letters from a to z\r\n");
    cdc.printf(" - upper case letters from A to Z\r\n");
    cdc.printf(" - special characters: <SPACE> ! # $ %% & ' ( ) * + , - . / : ; < = > ? @ [ \134 ] ^ _ ` { | } ~ \042 \r\n");
}

bool wifiWizard(WiFiInterface* net, wunderbar::WiFiConfig& config, DigitalOut& led)
{
    bool wifiConnected = false;
    char defaultPassword[20] = "";
    cdc.printf("\r\nFirst, we need to setup WiFi connection\r\n");
    while(!wifiConnected)
    {
        // bool ssidOk = false;
        // bool securityOk = false;
        // bool passOk = false;
        // // keep asking for SSID till valid name is entered
        // cdc.printf("\r\nPlease enter your WiFi SSID below and press ENTER.\r\n");
        // while(!ssidOk)
        // {
        //     validCharactersBanner("SSID", 1, 31);
        //     ssidOk = readField(config.ssid, 1, 31, config.ssid, &isCharPrintableAscii, true, led);
        // }

        // cdc.printf("Ok! WunderBar will try to use WiFi: %s\r\n\r\n", config.ssid);
        // // keep asking for security till valid one is selected
        // cdc.printf("\r\nNow, please select WiFi security and press ENTER.\r\n");
        // while(!securityOk)
        // {
        //     securityOk = selectSecurity(config.security, led);
        // }

        // // keep asking for password till valid one is entered
        // while(!passOk)
        // {
        //     validCharactersBanner("Password", 8, 63);
        //     cdc.printf("Characters entered for password will not be echoed!\r\n");
        //     passOk = readField(config.pass, 8, 63, defaultPassword, &isCharPrintableAscii, false, led);
        //     if(passOk)
        //     {
        //         std::memcpy(defaultPassword, "<previous password>", 19);
        //     }
        // }

        cdc.printf("\r\n\r\nOk! WunderBar will now try to connect to WiFi: %s\r\n", config.ssid);
        cdc.printf("Please be patient - this might take w while!\r\n");

        rtos::Thread progressBar(osPriorityNormal);
        progressBar.start(&progressDots);
        int status = net->connect(config.ssid,
                                            config.pass,
                                            config.security,
                                            config.channel);
        progressBar.terminate();
        wifiConnected = (status == NSAPI_ERROR_OK);
        if(!wifiConnected)
        {
            cdc.printf("\r\nCould not connect to WiFi: %s. \r\nPlease try configuration again.\r\n", config.ssid);
            continue;
        }
    }

    cdc.printf("\r\nGreat! WunderBar successfully connected to WiFi: %s\r\n", config.ssid);
    cdc.printf("WunderBar IP: %s\r\n", net->get_ip_address());
    return true;
}

