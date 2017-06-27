#pragma once

#include "resource.h"
#include "bleserver.h"
#include "Thread.h"

class BleBridge : public Resource, public BleServer
{

const int32_t NEW_SUB_SIGNAL = 0x1;
const int32_t ACK_DONE_SIGNAL = 0x2;
const int32_t BLE_SIGNAL = 0x4;
const int32_t PUBLISH_DONE_SIGNAL = 0x8;

public:
    BleBridge(IPubSub* _proto,
              const std::string& _subtopic,
              const std::string& _pubtopic,
              IBleGateway& _gateway);
    virtual ~BleBridge() {};

    bool subscribe();

private:
    // mqtt
    void publishThread();
    void publishDone(bool status);
    void subscribeThread();
    void subscribeCallback(const uint8_t* data, size_t len);
    void subscribeDone(bool status);
    void ackDone(bool status);

    // ble
    void bleEvent(const uint8_t* data, size_t len);

private:
    const std::string subtopic;
    const std::string pubtopic;
    const std::string acktopic;
    rtos::Thread subscriber;
    rtos::Thread publisher;
    std::unique_ptr<uint8_t[]> command;
    volatile bool subscribed;
};
