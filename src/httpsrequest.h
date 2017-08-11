#pragma once

#include "itransportlayer.h"

constexpr size_t maxBuffer = 512;

class HttpsRequest
{
public:
    HttpsRequest(ITransportLayer& _transport,
                 const char* _method,
                 const char* _server,
                 uint32_t _port,
                 const char* _url,
                 const char* _body);

    bool setHeader(const char* _name, const char* _value);
    bool send();
    bool recv(uint8_t* _resp, size_t _maxLen);

private:
    bool sendData(const char* buf, size_t len);

private:
    ITransportLayer& transport;
    char out[maxBuffer];
    const char* body;
    const char* server;
    const uint32_t port;
    size_t idx;
    // METHOD SP server:port/url HTTP/1.1 \n HEADERS \n
    const char* REQUEST_FORMAT = "%s https://%s:%d/%s HTTP/1.1\n";
    const char* HEADER_FORMAT = "%s: %s\n";

};
