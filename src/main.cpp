#include "mbed.h"

#include "cdc.h"

DigitalOut led1(LED1);
using usb::CDC;

int main(int argc, char **argv)
{
    CDC cdc;

    cdc.printf(" Welcome to WunderBar v2 mbed OS firmware\n");
    cdc.printf("Running at %d MHz\n", SystemCoreClock/1000000);

    while (true)
    {
        led1 = !led1;
        wait(2);
        cdc.printf("Led state: %d\r\n", !led1.read());
    }
}

