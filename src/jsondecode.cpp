#include "jsondecode.h"
#include <cstring>

JsonDecode::JsonDecode(const char* buffer, size_t maxTokens)
    : ok(false),
      raw(buffer),
      tokens(std::make_unique<jsmntok_t[]>(maxTokens))
{
    jsmn_init(&parser);
    numKeys = jsmn_parse(&parser, raw, std::strlen(raw), tokens.get(), maxTokens);
    ok = (numKeys > 0);
}

JsonDecode::operator bool() const
{
    return ok;
}

bool JsonDecode::copyTo(const char* fieldName, char* fieldValue, size_t maxSize)
{
    bool enoughSpace = false;
    size_t fieldSize = 0;
    const char* fieldStart = get(fieldName, fieldSize);
    if((fieldSize <= maxSize) && (fieldStart != raw))
    {
        std::strncpy(fieldValue, fieldStart, fieldSize);
        enoughSpace = true;
    }

    return enoughSpace;
}

int JsonDecode::tokenEquals(const char* json, jsmntok_t* tok, const char* s)
{
    if(tok->type == JSMN_STRING
        && (int) strlen(s) == tok->end - tok->start
        &&  strncmp(json + tok->start, s, tok->end - tok->start) == 0)
    {
        return 0;
    }

    return -1;
}

bool JsonDecode::isField(const char* fieldName)
{
    size_t fieldSize;
    return raw != get(fieldName, fieldSize);
}

const char* JsonDecode::get(const char* fieldName)
{
    size_t fieldSize;
    return get(fieldName, fieldSize);
}

const char* JsonDecode::get(const char* fieldName, size_t& size)
{
    if(numKeys > 0)
    {
        for(size_t key = 1; key < numKeys; ++key)
        {
            if(tokenEquals(raw, &tokens[key], fieldName) == 0)
            {
                size = tokens[key+1].end - tokens[key+1].start;
                return &raw[tokens[key+1].start];
            }
        }
    }
    return raw;
}
