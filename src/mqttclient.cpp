#include "mqttclient.h"

#include <cstring>

MqttClient::MqttClient(IPubSub* _proto,
                     const std::string& _subtopic,
                     const std::string& _pubtopic)
    : Resource(_proto),
      subtopic(_subtopic),
      pubtopic(_pubtopic),
      acktopic(_subtopic + "ack")
{
    subscriber.start(mbed::callback(this, &MqttClient::subscribeThread));
    publisher.start(mbed::callback(this, &MqttClient::publishThread));
}

// mqtt
bool MqttClient::subscribe()
{
    return Resource::subscribe(subtopic,
                               mbed::callback(this, &MqttClient::subscribeDone),
                               mbed::callback(this, &MqttClient::subscribeCallback));
}

void MqttClient::subscribeDone(bool status)
{
    subscribed = status;
}

void MqttClient::subscribeCallback(const uint8_t* data, size_t len)
{
    command = std::make_unique<uint8_t[]>(len);
    std::memcpy(command.get(), data, len);
    subscriber.signal_set(NEW_SUB_SIGNAL);
}

void MqttClient::subscribeThread()
{
    while(1)
    {
        rtos::Thread::signal_wait(NEW_SUB_SIGNAL);
        if(subscribed)
        {
            std::string commandName = "Toggle";
            std::string code = "200";
            acknowledge(acktopic, commandName, code, mbed::callback(this, &MqttClient::ackDone));
            rtos::Thread::signal_wait(ACK_DONE_SIGNAL);
            command.reset();
        }
    }
}

void MqttClient::ackDone(bool status)
{
    subscriber.signal_set(ACK_DONE_SIGNAL);
}

void MqttClient::subscribtionWritten()
{
    subscriber.signal_set(SUB_DATA_WRITTEN);
}

void MqttClient::publishThread()
{
    while(1)
    {
        rtos::Thread::signal_wait(NEW_PUB_SIGNAL);
        Resource::publish(pubtopic, publishContent, mbed::callback(this, &MqttClient::publishDone));
        rtos::Thread::signal_wait(PUBLISH_DONE_SIGNAL);
    }
}

void MqttClient::publishDone(bool status)
{
    publisher.signal_set(PUBLISH_DONE_SIGNAL);
}

void MqttClient::publish()
{
    publisher.signal_set(NEW_PUB_SIGNAL);
}
