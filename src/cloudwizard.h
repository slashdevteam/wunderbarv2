#pragma once

#include "DigitalOut.h"
#include "configuration.h"
#include "istdinout.h"

class NetworkStack;

bool cloudWizard(NetworkStack* net, MqttConfig& mqttConfig, TlsConfig& tlsConfig, mbed::DigitalOut& led, IStdInOut& log);
