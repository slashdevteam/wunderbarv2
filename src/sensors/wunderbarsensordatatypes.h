#pragma once

#include <cstdint>
#include <cstdio>
#include "mbed.h"
#include <cstring>

const size_t MAX_SENSOR_PAYLOAD_LEN = 20;

using ServerName = std::string;

const ServerName sensorNameHtu        = "WunderbarHTU";
const ServerName sensorNameGyro       = "WunderbarGYRO";
const ServerName sensorNameLightProx  = "WunderbarLIGHT";
const ServerName sensorNameMicrophone = "WunderbarMIC";
const ServerName sensorNameBridge     = "WunderbarBRIDG";
const ServerName sensorNameInfraRed   = "WunderbarIR";

const ServerName WunderbarSensorNames[] = {
    sensorNameHtu,
    sensorNameGyro,
    sensorNameLightProx,
    sensorNameMicrophone,
    sensorNameBridge,
    sensorNameInfraRed
};

struct threshold_int8_t
{
    uint8_t sbl;
    int8_t  low;
    int8_t  high;
} __attribute__((packed));

struct threshold_int16_t
{
    uint16_t sbl;
    int16_t  low;
    int16_t  high;
} __attribute__((packed));

struct threshold_int32_t
{
    uint32_t sbl;
    int32_t  low;
    int32_t  high;
} __attribute__((packed));


struct threshold_float_t
{
    float sbl;
    float low;
    float high;
} __attribute__((packed));

