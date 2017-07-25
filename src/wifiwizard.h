#pragma once

#include "DigitalOut.h"
#include "configuration.h"

class WiFiInterface;

bool wifiWizard(WiFiInterface* net, wunderbar::WiFiConfig& config, mbed::DigitalOut& led);
