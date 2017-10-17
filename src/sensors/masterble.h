#pragma once

#include <cstdio>
#include "mbed.h"

const char* KINETIS_FIRMWARE_REV = "2.0.0";
const char* MAIN_BOARD_HW_REV = "1.02";

const char* jsonMqttKinetisFwRevFormat = "{\"ts\":%ld,\"kinetis\":\"%s\",\"master ble\":\"%s\"}";
const char* jsonMqttWbHwRevFormat = "{\"ts\":%ld,\"hardware\":\"%s\"}";

inline int createJsonKinetisFwRev(char* outputString, size_t maxLen, char* masterBleFwVerString)
{
    return std::snprintf(outputString, maxLen, jsonMqttKinetisFwRevFormat, time(NULL), KINETIS_FIRMWARE_REV, masterBleFwVerString);
}

inline int createJsonWbHwRev(char* outputString, size_t maxLen)
{
    return std::snprintf(outputString, maxLen, jsonMqttWbHwRevFormat, time(NULL), MAIN_BOARD_HW_REV);
}
