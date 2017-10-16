#include "randompasskey.h"
#include "mbed.h"

PassKey randomPassKey()
{
    PassKey randKey;

    uint32_t rdm = 0;

    rdm = time(nullptr);
    srand(rdm);

    for(size_t i = 0; i < PASS_KEY_LEN; ++i)
    {
        randKey.data()[i] = rand() & 0xFF;
    }

    return randKey;
}
