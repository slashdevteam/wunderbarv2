#include "flash.h"

Flash::Flash()
    : config{//WiFiConfig
             {"",
              "",
              NSAPI_SECURITY_WPA2,
              0},
             // ProtocolConfig
             {"",
              8883}}
{

}
