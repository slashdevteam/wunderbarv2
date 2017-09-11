#pragma once

#include "flash.h"
#include "istdinout.h"
#include "iblegateway.h"
#include "WiFiInterface.h"
#include "NetworkStack.h"
#include "resources.h"

void onboardLoop(Flash& flash,
                 IStdInOut& log,
                 IBleGateway& ble,
                 WiFiInterface& wifi,
                 NetworkStack& net);
void runLoop(const wunderbar::Configuration& config,
             IStdInOut& log,
             IBleGateway& ble,
             WiFiInterface& wifi,
             NetworkStack& net,
             const Resources& resources);
