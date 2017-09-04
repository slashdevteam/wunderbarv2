#pragma once

#include "DigitalOut.h"
#include "configuration.h"

class NetworkStack;

bool cloudWizard(NetworkStack* net, MqttConfig& mqttConfig, TlsConfig& tlsConfig, mbed::DigitalOut& led);
