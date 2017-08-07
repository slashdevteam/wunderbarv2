#include "httpparser.h"
#include <cstring>
#include <cstdlib>

HttpParser::HttpParser(const char* buffer)
    : status(0),
      statusString(nullptr),
      body(nullptr),
      ok(parse(buffer))
{
}

HttpParser::operator bool() const
{
    return ok;
}

bool HttpParser::parse(const char* buffer)
{
    bool noError = false;
    // status line parsing
    // 8 chars for HTTP/1.1 + 1 space + 3 chars for status + 1 space + N chars for status string + \n
    int diff = std::strncmp(buffer, "HTTP/1.1", 8);
    status = std::atoi(buffer + 8 + 1);
    statusString = buffer + 8 + 1;

    noError = ((0 == diff) && ((status >= 200) && (status < 300)));

    if(noError)
    {
        // find end of status line and next character is beginning of headers
        size_t idx = 0;
        const char* headerName = std::strstr(buffer, "\n");
        while(headerName)
        {
            headerName++;
            if(*(headerName) == '\n' || *(headerName) == '\r')
            {
                // empty line, so next character is start of body
                body = headerName + 1;
                break;
            }
            // if we reach max headers, the rest will be ignored
            if(idx < MAX_HEADERS)
            {
                headers[idx] = headerName;
                idx++;
            }

            headerName = std::strstr(headerName, "\n");
        }
    }

    return noError && (nullptr != body);
}

const char* HttpParser::header(const char* name)
{
    const char* headerValue = nullptr;
    for(auto headerName : headers)
    {
        if(0 == std::strncmp(headerName, name, std::strlen(name)))
        {
            headerValue = std::strstr(headerName, ":") + 2;
            break;
        }
    }
    return headerValue;
}
