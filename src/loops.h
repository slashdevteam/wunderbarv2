#pragma once

#include "flash.h"
#include "istdinout.h"
#include "iblegateway.h"
#include "WiFiInterface.h"
#include "NetworkStack.h"
#include "resources.h"
#include "info.h"

void onboardLoop(Flash& flash,
                 IStdInOut& log,
                 IBleGateway& ble,
                 WiFiInterface& wifi,
                 NetworkStack& net,
                 const Resources& resources,
                 uint8_t* loopStack,
                 size_t loopStackSize);

void runLoop(const wunderbar::Configuration& config,
             IStdInOut& log,
             IBleGateway& ble,
             Info& info,
             WiFiInterface& wifi,
             NetworkStack& net,
             const Resources& resources,
             uint8_t* loopStack,
             size_t loopStackSize);
