#include "flash.h"

const uint32_t FLASH_MARKER = 0xDEC0FFEE;
const uint32_t DEFAULT_CONFIGURATION = 0x004B1D00;
const uint32_t ONBOARDED_CONFIGURATION = 0X1ABE11ED;
const uint32_t DEFAULT_FLASH_HEADER[] = {FLASH_MARKER, DEFAULT_CONFIGURATION};

// storage is located at the end of flash
extern size_t configstart;

Flash::Flash()
  : storage(reinterpret_cast<FlashStorage*>((&configstart)))
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
        reset();
    }

    return (ONBOARDED_CONFIGURATION == storage->configuration);
}

void Flash::reset()
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

void Flash::resetHeader()
{
    mutex.lock();
    uint32_t storageStart = reinterpret_cast<uint32_t>(storage);
    FlashStorage oldContent;
    uint8_t* oldContentStart = reinterpret_cast<uint8_t*>(&oldContent);
    std::memcpy(oldContentStart, reinterpret_cast<uint8_t*>(storage), sizeof(oldContent));
    oldContent.marker = FLASH_MARKER;
    oldContent.configuration = DEFAULT_CONFIGURATION;
    flash_erase_sector(&flashDriver, storageStart);
    flash_program_page(&flashDriver,
                       storageStart,
                       oldContentStart,
                       sizeof(oldContent));
    mutex.unlock();
}

void Flash::store(const Configuration& config)
{
    mutex.lock();
    FlashStorage newContent;
    newContent.marker = FLASH_MARKER;
    newContent.configuration = ONBOARDED_CONFIGURATION;
    uint8_t* newConfigStart = reinterpret_cast<uint8_t*>(&newContent.config);
    std::memcpy(newConfigStart, reinterpret_cast<const uint8_t*>(&config), sizeof(config));
    uint32_t storageStart = reinterpret_cast<uint32_t>(storage);

    flash_erase_sector(&flashDriver, storageStart);
    flash_program_page(&flashDriver,
                       storageStart,
                       reinterpret_cast<uint8_t*>(&newContent),
                       sizeof(newContent));
    mutex.unlock();
}
