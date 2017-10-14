#pragma once

#include "DigitalOut.h"
#include "configuration.h"
#include "istdinout.h"
#include "resources.h"

class NetworkStack;

bool cloudWizard(NetworkStack* net,
                 MqttConfig& mqttConfig,
                 TlsConfig& tlsConfig,
                 RestConfig& restConfig,
                 mbed::DigitalOut& led,
                 IStdInOut& log,
                 const Resources& resources);
