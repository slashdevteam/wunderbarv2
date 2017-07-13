#include "flash.h"

const uint32_t FLASH_MARKER = 0xDEC0FFEE;
const uint32_t DEFAULT_CONFIGURATION = 0x004B1D00;
const uint32_t ONBOARDED_CONFIGURATION = 0X1ABE11ED;
const uint32_t DEFAULT_FLASH_HEADER[] = {FLASH_MARKER, DEFAULT_CONFIGURATION};

// storage is located at the end of flash
extern size_t __uvisor_flash_end;
extern size_t __etext;

Flash::Flash()
  : storage(reinterpret_cast<FlashStorage*>((&__uvisor_flash_end - (FLASH_STORAGE_SECTORS * FLASH_SECTOR_SIZE))))
{
    // this unfortunately cannot be asserted during compilation
    MBED_ASSERT((FLASH_STORAGE_SECTORS *FLASH_SECTOR_SIZE) <= (&__uvisor_flash_end - &__etext));
    mutex.lock();
    flash_init(&flashDriver);

    mutex.unlock();
}

Flash::~Flash()
{
    mutex.lock();
    flash_free(&flashDriver);
    mutex.unlock();
}

bool Flash::isOnboarded()
{
    if(FLASH_MARKER != storage->marker)
    {
        mutex.lock();
        uint32_t storageStart = reinterpret_cast<uint32_t>(storage);
        flash_erase_sector(&flashDriver, storageStart);
        flash_program_page(&flashDriver,
                           storageStart,
                           reinterpret_cast<const uint8_t*>(DEFAULT_FLASH_HEADER),
                           sizeof(DEFAULT_FLASH_HEADER));
        mutex.unlock();
    }

    return (ONBOARDED_CONFIGURATION == storage->configuration);
}
