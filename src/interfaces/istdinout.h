#pragma once

class IStdInOut
{
public:
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

};
