#include "pollentropy.h"
#include <cstdint>
#include "mbed.h"

// free function because of mbed TLS C style interface
int pollEntropy(void *,
                unsigned char *output,
                size_t len,
                size_t *olen)
{
    uint32_t rdm = 0;

    rdm = time(nullptr);
    srand(rdm);

    for(uint16_t i = 0; i < len; ++i)
    {
        output[i] = rand() & 0xFF;
    }
    *olen = len;

    return 0;
}
