#include "randompasskey.h"
#include "mbed.h"
#include "mbedtls/entropy_poll.h"

PassKey randomPassKey()
{
    PassKey randKey;

    for(size_t i = 0; i < PASS_KEY_LEN; ++i)
    {
        uint32_t result = 0;
        size_t len;
        mbedtls_hardware_poll(nullptr, (uint8_t*)&result, sizeof(result), &len);
        randKey.data()[i] = result & 0xFF;
    }

    return randKey;
}
