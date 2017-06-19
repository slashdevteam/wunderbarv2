#include "cdc.h"

const uint8_t CONTROLLER_ID = 0; //kUSB_ControllerKhci0

namespace wunderbar
{
    extern device_specific_descriptors cdcDescriptors;
}

namespace usb
{
CDC::CDC()
    : cdcDevice(CONTROLLER_ID, wunderbar::cdcDescriptors)
{
    cdcDevice.run();
}

void CDC::run()
{
    cdcDevice.run();
}

int CDC::_putc(int c)
{
    cdcDevice.send((uint8_t*)&c, 1);
    return 1;
}

int CDC::_getc()
{
    assert(false); // NOT IMPLEMENTED
    return 0;
}

void CDC::lock()
{
    mutex.lock();
}

void CDC::unlock()
{
    mutex.unlock();
}

}
