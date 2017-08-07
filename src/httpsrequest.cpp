#include "httpsrequest.h"

#include <cstdio>
#include <cstring>

HttpsRequest::HttpsRequest(ITransportLayer& _transport,
                           const char* _method,
                           const char* _server,
                           uint32_t _port,
                           const char* _url,
                           const char* _body)
    : transport(_transport),
      body(_body),
      server(_server),
      port(_port),
      idx(0)
{
    idx = std::snprintf(out, maxBuffer, REQUEST_FORMAT, _method, _server, _port, _url);
}

bool HttpsRequest::setHeader(const char* _name, const char* _value)
{
    bool headerOk = false;
    int printed = std::snprintf(out + idx, maxBuffer - idx - 1, HEADER_FORMAT, _name, _value);
    if(printed > 0)
    {
        idx += printed;
        headerOk = true;
    }

    return headerOk;
}

bool HttpsRequest::send()
{
    bool success = false;
    if(transport.connect(server, port))
    {
        out[idx] = '\n';
        success = sendData(out, idx+1);
        if(success && body)
        {
            success = sendData(body, std::strlen(body));
        }
    }

    return success;
}

bool HttpsRequest::recv(uint8_t* _resp, size_t _maxLen)
{
    return (transport.receive(_resp, _maxLen) != 0);
}

bool HttpsRequest::sendData(const char* buf, size_t len)
{
    size_t part = 0;
    size_t sent = 0;

    while(sent < len)
    {
        part = transport.send(reinterpret_cast<const uint8_t*>(buf), len);
        if(part == 0)
        {
            break;
        }
        sent += part;
    }

    return (sent == len);
}
