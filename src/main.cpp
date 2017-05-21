#include "mbed.h"

DigitalOut led1(LED1);
Serial pc(USBTX, USBRX);

#include "deviceclass.h"
#define CONTROLLER_ID 0 //kUSB_ControllerKhci0

usb::DeviceClass* gDevice;
namespace wunderbar
{
    extern device_specific_descriptors cdcDescriptors;
}

void task()
{
    while(1)
    {
        gDevice->echo();
    }
}

// main() runs in its own thread in the OS
int main(int argc, char **argv)
{
    {
        DigitalIn gsPD(PTD5);
        while(!gsPD.read())
        {};
    }
    DigitalOut gsPD(PTD5);
    gsPD = 0;

    usb::DeviceClass deviceclass(CONTROLLER_ID, wunderbar::cdcDescriptors);
    gDevice = &deviceclass;
    deviceclass.run();
    Thread cdcecho;
    cdcecho.start(mbed::callback(task));

    while (true)
    {
        led1 = !led1;
        wait(2);
        pc.printf("A\n");
    }
}

