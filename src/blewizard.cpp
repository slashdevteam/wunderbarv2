#include "blewizard.h"

bool bleWizard(IBleGateway& bleGate, IStdInOut& log)
{
    return bleGate.configure();;
}
