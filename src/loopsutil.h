#pragma once

#include "DigitalOut.h"
#include "Callback.h"
#include "jsmn.h"
#include "istdinout.h"
#include "Thread.h"

using CharacterValidator = mbed::Callback<bool(char c)>;

class ProgressBar
{
public:
    ProgressBar(IStdInOut& _log, mbed::DigitalOut& _led)
        : log(_log),
          led(_led),
          executor(osPriorityNormal, 0x500)
    {};

    void start();
    void terminate();

private:
    void show();

private:
    IStdInOut& log;
    mbed::DigitalOut& led;
    rtos::Thread executor;
    const int32_t KILL_SIG = 0x1;
};

void progressDots(IStdInOut& log);

bool readField(IStdInOut& log,
               char* field,
               size_t minCharacters,
               size_t maxCharacters,
               const char* defaultValue,
               CharacterValidator characterValidate,
               bool doEcho,
               mbed::DigitalOut& led);

bool isCharPrintableAscii(char c);
