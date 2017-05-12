#include "mbed.h"

DigitalOut led1(LED1);

// main() runs in its own thread in the OS
int main(int argc, char **argv)
{
    while (true)
    {
        led1 = !led1;
        wait(2);
    }
}

