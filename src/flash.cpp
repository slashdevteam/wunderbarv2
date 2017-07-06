#include "flash.h"

const uint32_t FLASH_MARKER = 0xDEC0FFEE;
const uint32_t DEFAULT_CONFIGURATION = 0x004B1D00;
const uint32_t ONBOARDED_CONFIGURATION = 0X1ABE11ED;
const uint32_t DEFAULT_FLASH_HEADER[] = {FLASH_MARKER, DEFAULT_CONFIGURATION};

Flash::Flash()
  : storage(reinterpret_cast<FlashStorage*>(&__flash_storage))
{
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
