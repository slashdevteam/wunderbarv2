#pragma once

#include <cstdint>

struct threshold_int8_t;
{
    uint8_t sbl;
    int8_t  low;
    int8_t  high;
} __attribute__((packed))

struct threshold_int16_t;
{
    uint16_t sbl;
    int16_t  low;
    int16_t  high;
} __attribute__((packed))

struct threshold_int32_t;
{
    uint32_t sbl;
    int32_t  low;
    int32_t  high;
} __attribute__((packed))


struct threshold_float_t;
{
    float sbl;
    float low;
    float high;
} __attribute__((packed))

// HTU
struct sensor_htu_threshold_t;
{
    threshold_int16_t  temperature;
    threshold_int16_t  humidity;
} __attribute__((packed))

enum sensor_htu_config_t;
{
    HTU21D_RH_12_TEMP14 = 0,
    HTU21D_RH_8_TEMP12,
    HTU21D_RH_10_TEMP13,
    HTU21D_RH_11_TEMP11,
} __attribute__((packed))

struct sensor_htu_data_t;
{
    int16_t temperature;
    int16_t humidity;
} __attribute__((packed))

// GYRO
struct sensor_gyro_threshold_t;
{
    threshold_int32_t gyro;   
    threshold_int16_t acc;
} __attribute__((packed))

enum sensor_gyro_GyroFullScale_t;
{
    GYRO_FULL_SCALE_250DPS = 0,
    GYRO_FULL_SCALE_500DPS,
    GYRO_FULL_SCALE_1000DPS,
    GYRO_FULL_SCALE_2000DPS
}

enum sensor_gyro_AccFullScale_t;
{
    ACC_FULL_SCALE_2G = 0,
    ACC_FULL_SCALE_4G,
    ACC_FULL_SCALE_8G,
    ACC_FULL_SCALE_16G
}

struct sensor_gyro_config_t;
{
    sensor_gyro_GyroFullScale_t gyro_full_scale;
    sensor_gyro_AccFullScale_t  acc_full_scale;
} __attribute__((packed))

struct sensor_gyro_GyroCoord_t;
{
    int32_t x;
    int32_t y;
    int32_t z;
} __attribute__((packed))

struct sensor_gyro_AccCoord_t;
{
    int16_t x;
    int16_t y;
    int16_t z;
} __attribute__((packed))

struct sensor_gyro_data_t;
{
    sensor_gyro_GyroCoord_t gyro;
    sensor_gyro_AccCoord_t acc;
} __attribute__((packed))

#define Template_gyro_data "{\"ts\":%s,\"gyro\":{\"x\":%s,\"y\":%s,\"z\":%s},\"accel\":{\"x\":%s,\"y\":%s,\"z\":%s}}"


//LIGHT-PROX
struct sensor_lightprox_threshold_t;
{
    threshold_int16_t white;
    threshold_int16_t proximity;
} __attribute__((packed))


enum sensor_lightprox_rgbc_gain_t;
{
    RGBC_GAIN_1     = 0x00, 
    RGBC_GAIN_4     = 0x01,
    RGBC_GAIN_16    = 0x02,
    RGBC_GAIN_60    = 0x03
}


enum sensor_lightprox_prox_drive_t;
{
    PROX_DRIVE_12_5_MA = 0xC0,
    PROX_DRIVE_25_MA   = 0x80,
    PROX_DRIVE_50_MA   = 0x40,
    PROX_DRIVE_100_MA  = 0x00   
}

struct sensor_lightprox_config_t;
{
    sensor_lightprox_rgbc_gain_t    rgbc_gain;
    sensor_lightprox_prox_drive_t   prox_drive;
} __attribute__((packed))


struct sensor_lightprox_data_t;
{
    uint16_t r;
    uint16_t g;
    uint16_t b;
    uint16_t white;
    uint16_t proximity;
} __attribute__((packed))

#define Template_light_data "{\"ts\":%s,\"light\":%d,\"clr\":{\"r\":%d,\"g\":%d,\"b\":%d},\"prox\":%d}"


//IR-TX
uint8_t sensor_ir_data_t;

//MICROPHONE
struct sensor_microphone_threshold_t;
{
    threshold_int16_t mic_level;
} __attribute__((packed))

struct sensor_microphone_data_t;
{
    int16_t mic_level;
} __attribute__((packed))

#define Template_sound_data "{\"ts\":%s,\"snd_level\":%d}"

//BRIDGE
const uint32_t BRIDGE_PAYLOAD_SIZE = 19;
const uint32_t BRIDGE_HEDER_SIZE   = 2;
const uint32_t BRIDGE_CRC16_SIZE   = 2;
const uint32_t BRIDGE_PACKET_SIZE  = BRIDGE_PAYLOAD_SIZE + BRIDGE_HEDER_SIZE + BRIDGE_CRC16_SIZE;

struct sensor_bridge_data_t;
{
    uint8_t payload_length;
    uint8_t payload[BRIDGE_PAYLOAD_SIZE];
} __attribute__((packed))

struct sensor_bridge_config_t;
{
    uint32_t baud_rate;
} __attribute__((packed))

#define Template_bridge_data "{\"ts\":%s,\"up_ch_payload\":[%s]}"

void Sensors_FormBridgeArray(sensor_bridge_data_t* bridgeData, char* txt){
	char* ptr = txt;
	int i;

	for (i = 0; i < bridgeData->payload_length; i ++)
	{
		sprintf(ptr, "%d,", (int) bridgeData->payload[i]);
		ptr = strchr(ptr, ',');
		ptr ++;
	}
	*(ptr - 1) = 0;
}


#define Template_main_firmware_rev   "{\"ts\":%s,\"kinetis\":\"%s\",\"master ble\":\"%s\"}"
#define Template_main_hardware_rev   "{\"ts\":%s,\"hardware\":\"%s\"}"
