#pragma once

#include "jsmn.h"
#include <memory>

using Tokens = std::unique_ptr<jsmntok_t[]>;

class JsonDecode
{
public:
    JsonDecode(const char* buffer, size_t maxTokens);

    bool copyTo(const char* fieldName, char* fieldValue, size_t maxSize);
    const char* get(const char* fieldName);
    const char* get(const char* fieldName, size_t& size);
    bool isField(const char* fieldName);
    bool isField(const char* fieldName, const char*& value, size_t& size);
    // safe method for boolean ops on object
    explicit operator bool() const;

private:
    int tokenEquals(const char* json, jsmntok_t* tok, const char* s);

private:
    bool ok;
    const char* raw;
    jsmn_parser parser;
    Tokens tokens;
    size_t numKeys;
};
