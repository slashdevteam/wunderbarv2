#pragma once

#include <cstddef>

// free function because of mbed TLS C style interface
int pollEntropy(void *,
                unsigned char *output,
                size_t len,
                size_t *olen);
