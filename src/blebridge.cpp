#include "blebridge.h"

#include "wunderbarble.h"
#include <cstring>

BleBridge::BleBridge(IPubSub* _proto,
                     const std::string& _subtopic,
                     const std::string& _pubtopic,
                     IBleGateway& _gateway)
    : Resource(_proto),
      WunderbarSensor(_gateway,
                      "WunderbarBRIDG",
                      {0x30, 0x30, 0x30, 0x30, 0x30, 0x31, 0x01, 0x01},
                      mbed::callback(this, &BleBridge::bleEvent)),
      subtopic(_subtopic),
      pubtopic(_pubtopic),
      acktopic(_subtopic + "ack")
{
    subscriber.start(mbed::callback(this, &BleBridge::subscribeThread));
    publisher.start(mbed::callback(this, &BleBridge::publishThread));
}

// mqtt
bool BleBridge::subscribe()
{
    return Resource::subscribe(subtopic,
                               mbed::callback(this, &BleBridge::subscribeDone),
                               mbed::callback(this, &BleBridge::subscribeCallback));
}

void BleBridge::subscribeDone(bool status)
{
    subscribed = status;
}

void BleBridge::subscribeCallback(const uint8_t* data, size_t len)
{
    command = std::make_unique<uint8_t[]>(len);
    std::memcpy(command.get(), data, len);
    subscriber.signal_set(NEW_SUB_SIGNAL);
}

void BleBridge::subscribeThread()
{
    while(1)
    {
        rtos::Thread::signal_wait(NEW_SUB_SIGNAL);
        if(subscribed)
        {
            std::string commandName = "Toggle";
            std::string code = "200";
            acknowledge(acktopic, commandName, code, mbed::callback(this, &BleBridge::ackDone));
            rtos::Thread::signal_wait(ACK_DONE_SIGNAL);
            command.reset();
        }
    }
}

void BleBridge::ackDone(bool status)
{
    subscriber.signal_set(ACK_DONE_SIGNAL);
}

void BleBridge::publishThread()
{
    while(1)
    {
        rtos::Thread::signal_wait(BLE_SIGNAL);
        std::string data = "\"ble\":";
        publish(pubtopic, data, mbed::callback(this, &BleBridge::publishDone));
        rtos::Thread::signal_wait(PUBLISH_DONE_SIGNAL);
    }
}

void BleBridge::publishDone(bool status)
{
    publisher.signal_set(PUBLISH_DONE_SIGNAL);
}

// ble
void BleBridge::bleEvent(BleEvent event, const uint8_t* data, size_t len)
{
    if(registrationOk)
    {

    }
}
