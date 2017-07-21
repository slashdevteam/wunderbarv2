#pragma once

#include <cstdint>

typedef struct __attribute__((packed))
{
    uint8_t sbl;
    int8_t  low;
    int8_t  high;
} 
threshold_int8_t;

typedef struct __attribute__((packed))
{
    uint16_t sbl;
    int16_t  low;
    int16_t  high;
} 
threshold_int16_t;

typedef struct __attribute__((packed))
{
    uint32_t sbl;
    int32_t  low;
    int32_t  high;
} 
threshold_int32_t;

typedef struct 
{
    float sbl;
    float low;
    float high;
}
threshold_float_t;

// HTU
typedef struct __attribute__((packed))
{
    threshold_int16_t  temperature;
    threshold_int16_t  humidity;
} 
sensor_htu_threshold_t;

typedef enum 
{
    HTU21D_RH_12_TEMP14 = 0,
    HTU21D_RH_8_TEMP12,
    HTU21D_RH_10_TEMP13,
    HTU21D_RH_11_TEMP11,
} 
sensor_htu_config_t;

typedef struct __attribute__((packed))
{
    int16_t temperature;
    int16_t humidity;
}
sensor_htu_data_t;

// GYRO
typedef struct __attribute__((packed))
{
    threshold_int32_t gyro;   
    threshold_int16_t acc;
} 
sensor_gyro_threshold_t;

typedef enum
{
    GYRO_FULL_SCALE_250DPS = 0,
    GYRO_FULL_SCALE_500DPS,
    GYRO_FULL_SCALE_1000DPS,
    GYRO_FULL_SCALE_2000DPS
}
sensor_gyro_GyroFullScale_t;

typedef enum
{
    ACC_FULL_SCALE_2G = 0,
    ACC_FULL_SCALE_4G,
    ACC_FULL_SCALE_8G,
    ACC_FULL_SCALE_16G
}
sensor_gyro_AccFullScale_t;


typedef struct __attribute__((packed))
{
    sensor_gyro_GyroFullScale_t gyro_full_scale;
    sensor_gyro_AccFullScale_t  acc_full_scale;
} 
sensor_gyro_config_t;

typedef struct __attribute__((packed))
{
    int32_t x;
    int32_t y;
    int32_t z;
}
sensor_gyro_GyroCoord_t;

typedef struct __attribute__((packed))
{
    int16_t x;
    int16_t y;
    int16_t z;
}
sensor_gyro_AccCoord_t;

typedef struct __attribute__((packed))
{
    sensor_gyro_GyroCoord_t gyro;
    sensor_gyro_AccCoord_t acc;
}
sensor_gyro_data_t;


//LIGHT-PROX
typedef struct __attribute__((packed))
{
    threshold_int16_t white;
    threshold_int16_t proximity;
}
sensor_lightprox_threshold_t;

typedef enum
{
    RGBC_GAIN_1     = 0x00, 
    RGBC_GAIN_4     = 0x01,
    RGBC_GAIN_16    = 0x02,
    RGBC_GAIN_60    = 0x03
}
sensor_lightprox_rgbc_gain_t;

typedef enum
{
    PROX_DRIVE_12_5_MA = 0xC0,
    PROX_DRIVE_25_MA   = 0x80,
    PROX_DRIVE_50_MA   = 0x40,
    PROX_DRIVE_100_MA  = 0x00   
}
sensor_lightprox_prox_drive_t;

typedef struct __attribute__((packed))
{
    sensor_lightprox_rgbc_gain_t    rgbc_gain;
    sensor_lightprox_prox_drive_t   prox_drive;
}
sensor_lightprox_config_t;

typedef struct 
{
    uint16_t r;
    uint16_t g;
    uint16_t b;
    uint16_t white;
    uint16_t proximity;
}
sensor_lightprox_data_t;

//IR-TX
typedef uint8_t sensor_ir_data_t;

//MICROPHONE
typedef struct 
{
    threshold_int16_t mic_level;
}
sensor_microphone_threshold_t;

typedef struct 
{
    int16_t mic_level;
}
sensor_microphone_data_t;

//BRIDGE
const uint32_t BRIDGE_PAYLOAD_SIZE = 19;
const uint32_t BRIDGE_HEDER_SIZE   = 2;
const uint32_t BRIDGE_CRC16_SIZE   = 2;
const uint32_t BRIDGE_PACKET_SIZE  = BRIDGE_PAYLOAD_SIZE + BRIDGE_HEDER_SIZE + BRIDGE_CRC16_SIZE;

typedef struct __attribute__((packed)) 
{
    uint8_t payload_length;
    uint8_t payload[BRIDGE_PAYLOAD_SIZE];
}
sensor_bridge_data_t;

typedef struct 
{
    uint32_t baud_rate;
}
sensor_bridge_config_t;