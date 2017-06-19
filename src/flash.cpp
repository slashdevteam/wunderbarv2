#include "flash.h"

Flash::Flash()
    : config{//WiFiConfig
             {"",
             "",
              NSAPI_SECURITY_WPA2,
              0},
             // TLS Config
             {"",
              // Root CA
              "-----BEGIN CERTIFICATE-----\n"
              "-----END CERTIFICATE-----\0",
              // Device CERT
              "-----BEGIN CERTIFICATE-----\n"
              "-----END CERTIFICATE-----\0",
              // Device Key
              "-----BEGIN RSA PRIVATE KEY-----\n"
              "-----END RSA PRIVATE KEY-----\0"},
              // MQTT Config
              {"",
                8883,
               "",
               "",
               ""}}
{

}
