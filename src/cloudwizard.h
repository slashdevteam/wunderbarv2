#pragma once

#include "DigitalOut.h"
#include "configuration.h"

class NetworkInterface;

bool cloudWizard(NetworkInterface* net, MqttConfig& mqttConfig, TlsConfig& tlsConfig, mbed::DigitalOut& led);
