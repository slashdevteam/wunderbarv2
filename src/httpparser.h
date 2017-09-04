#pragma once

#include <cstddef>
#include <array>

constexpr int MAX_HEADERS = 10;

class HttpParser
{
public:
    HttpParser(const char* buffer);

    // safe method for boolean ops on object
    explicit operator bool() const;
    const char* header(const char* name);

private:
    bool parse(const char* buffer);

public:
    int status;
    const char* statusString;
    const char* body;

private:
    std::array<const char*, MAX_HEADERS> headers;
    bool ok;

};
