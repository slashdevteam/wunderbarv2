#pragma once

#include "DigitalOut.h"
#include "configuration.h"
#include "istdinout.h"

class WiFiInterface;

bool wifiWizard(WiFiInterface* net, wunderbar::WiFiConfig& config, mbed::DigitalOut& led, IStdInOut& log);
