#include "blewizard.h"
#include "cdc.h"
using usb::CDC;
extern CDC cdc;

bool bleWizard(IBleGateway& bleGate)
{
    cdc.printf("\r\nNow we will setup Bluetooth sensors\r\n");
    return bleGate.configure();;
}
