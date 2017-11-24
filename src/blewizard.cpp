#include "blewizard.h"
#include "loopsutil.h"
#include "wunderbarsensordatatypes.h"

bool bleWizard(IBleGateway& bleGate, BleConfig& config, mbed::DigitalOut& led, IStdInOut& log)
{
    bool bleDone = false;
    uint32_t discoveryTimeout = 40;
    while(!bleDone)
    {
        log.printf("\r\nSending base configuration to Bluetooth master\r\n");
        bleGate.setTimeout(discoveryTimeout);
        bleGate.configure(config);
        if(bleGate.storeConfig())
        {
            uint32_t accountedForSensors = 0;
            log.printf("Now, please put all Bluetooth sensors you want to use in onboarding mode by\r\n");
            log.printf("pressing & releasing button on sensor. Leds should start blinking.\r\n");
            log.printf("Onboarding mode will be active for %d seconds.\r\n", discoveryTimeout);
            log.printf("Press ENTER to continue.\r\n");
            waitForEnter(log);
            log.printf("Onboarding started, please wait...\r\n");
            ProgressBar progressBar(log, led, true, 100);
            progressBar.start();
            bleDone = bleGate.onboard(config);
            // reset timeout to wait forever
            bleGate.setTimeout(0);
            progressBar.terminate();
            led = 0;
            if(!bleDone)
            {
                log.printf("Following sensors were not found:\r\n");
                for(uint8_t sensorId = 0; sensorId < WUNDERBAR_SENSORS_NUM; ++sensorId)
                {
                    if(config.sensorAvailability[sensorId] == SensorAvailability::NOT_AVAILABLE)
                    {
                        log.printf("- %s\r\n", WunderbarSensorNames(sensorId).c_str());
                        log.printf("Would you like to exclude this sensor from onboarding? (Y/N)\r\n");
                        if(agree(log, led))
                        {
                            config.sensorAvailability[sensorId] = SensorAvailability::IGNORE;
                            accountedForSensors++;
                        }
                    }
                    else
                    {
                        accountedForSensors++;
                    }
                }
                // BLE setup is complete if all sensors are either onboarded or explicitly ignored
                bleDone = (accountedForSensors == WUNDERBAR_SENSORS_NUM);
                if(bleDone)
                {
                    // print overview of sensors
                    log.printf("\r\nAll sensors configured.\r\n");
                    log.printf("Following sensors will be available:\r\n");
                    for(uint8_t sensorId = 0; sensorId < WUNDERBAR_SENSORS_NUM; ++sensorId)
                    {
                        if(config.sensorAvailability[sensorId] == SensorAvailability::AVAILABLE)
                        {
                            log.printf("- %s\r\n", WunderbarSensorNames(sensorId).c_str());
                        }
                    }
                    log.printf("Following sensors will be ignored:\r\n");
                    for(uint8_t sensorId = 0; sensorId < WUNDERBAR_SENSORS_NUM; ++sensorId)
                    {
                        if(config.sensorAvailability[sensorId] == SensorAvailability::IGNORE)
                        {
                            log.printf("- %s\r\n", WunderbarSensorNames(sensorId).c_str());
                        }
                    }
                    log.printf("Bluetooth setup can be safely completed.\r\n");
                }

                log.printf("\r\nTry again? (Y/N)\r\n");
                if(!agree(log, led))
                {
                    bleDone = (accountedForSensors == WUNDERBAR_SENSORS_NUM);
                    break;
                }
            }
        }
        else
        {
            log.printf("Critical error! Persistent error at this step indicates hardware problem with Wunderbar.\r\n");
            log.printf("Try again? (Y/N)\r\n");
            if(!agree(log, led))
            {
                break;
            }
        }
    }

    return bleDone;
}
