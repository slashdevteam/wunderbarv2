#pragma once

#include "configuration.h"
#include "flash_api.h"
#include "PlatformMutex.h"

using wunderbar::Configuration;

struct FlashStorage
{
    uint32_t marker; // if this is not aligned between code and flash, flash sectors will be cleared
    uint32_t configuration; // 0x004B1D00 means no configuration stored
    Configuration config;
} __attribute__ ((__packed__));

static_assert(sizeof(FlashStorage) <= FLASH_STORAGE_SECTORS * FLASH_SECTOR_SIZE,
              "Struct FlashStorage size has to be less than FLASH_STORAGE_SECTORS * FLASH_SECTOR_SIZE!");
static_assert(std::is_standard_layout<FlashStorage>::value,
              "Struct FlashStorage has to be POD!");

class Flash
{
public:
    Flash();
    ~Flash();

    const Configuration& getConfig() const
    {
        return storage->config;
    }

    const FlashStorage& getStorage() const
    {
        return *storage;
    }

    bool isOnboarded();
    void reset();
    void resetHeader();

    void store(const Configuration& config);

private:
    FlashStorage* storage;
    flash_t flashDriver;
    PlatformMutex mutex;
};
