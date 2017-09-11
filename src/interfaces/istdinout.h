#pragma once

class IStdInOut
{
public:
    IStdInOut() = default;
    virtual ~IStdInOut() = default;

    virtual int putc(int c)
    {
        return 0;
    }

    virtual int getc()
    {
        return 0;
    }

    virtual int puts(const char *str)
    {
        return 0;
    }

    virtual int printf(const char *format, ...)
    {
        return 0;
    }

    // make non-copyable C++11 style
    IStdInOut(const IStdInOut& other) = delete;
    IStdInOut& operator=(const IStdInOut&) = delete;
};
