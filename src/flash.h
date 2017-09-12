#pragma once

#include "configuration.h"
#include "flash_api.h"
#include "PlatformMutex.h"

using wunderbar::Configuration;

constexpr size_t BLOCK_WRITE_UNIT_SIZE = 8; // MCU specific!
constexpr size_t FLASH_STORAGE_BASE = (8 + sizeof(Configuration)); // marker + config = 2 x uint32 = 8 bytes
constexpr size_t FLASH_STORAGE_PADDING = BLOCK_WRITE_UNIT_SIZE - (FLASH_STORAGE_BASE % BLOCK_WRITE_UNIT_SIZE);
struct FlashStorage
{
    uint32_t marker; // if this is not aligned between code and flash, flash sectors will be cleared
    uint32_t configuration; // 0x004B1D00 means no configuration stored
    Configuration config;
    uint8_t PADDING_FOR_WRITE[FLASH_STORAGE_PADDING];
} __attribute__ ((__packed__));


static_assert(sizeof(FlashStorage) <= FLASH_STORAGE_SECTORS * FLASH_SECTOR_SIZE,
              "Struct FlashStorage size has to be less than FLASH_STORAGE_SECTORS * FLASH_SECTOR_SIZE!");
static_assert((sizeof(FlashStorage) & (BLOCK_WRITE_UNIT_SIZE - 1)) == 0,
              "Struct FlashStorage size has to be aligned to BLOCK_WRITE_UNIT_SIZE");
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

    // make non-copyable C++11 style
    Flash(const Flash& other) = delete;
    Flash& operator=(const Flash&) = delete;

private:
    FlashStorage* storage;
    flash_t flashDriver;
    PlatformMutex mutex;
};
