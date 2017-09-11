#include "blewizard.h"

bool bleWizard(IBleGateway& bleGate, IStdInOut& log)
{
    log.printf("\r\nNow we will setup Bluetooth sensors\r\n");
    log.printf("Please put all Bluetooth sensors in onboarding mode by\r\n");
    log.printf("pressing & releasing button on sensor\r\n");
    log.printf("Leds should start blinking.\r\n");
    log.printf("Now press ENTER to continue.\r\n");
    log.getc();
    return bleGate.configure();;
}
