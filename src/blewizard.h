#pragma once

#include "iblegateway.h"
#include "istdinout.h"
#include "configuration.h"
#include "DigitalOut.h"

bool bleWizard(IBleGateway& bleGate, BleConfig& config, mbed::DigitalOut& led, IStdInOut& log);
