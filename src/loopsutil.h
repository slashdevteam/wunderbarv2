#pragma once

#include "DigitalOut.h"
#include "Callback.h"
#include "jsmn.h"
#include "istdinout.h"
#include "Thread.h"
#include <memory>

using CharacterValidator = mbed::Callback<bool(char c)>;

class ProgressBar
{

using ThreadHandle = std::unique_ptr<rtos::Thread>;

public:
    ProgressBar(IStdInOut& _log, mbed::DigitalOut& _led, bool _silent, uint32_t _period)
        : log(_log),
          led(_led),
          executor(nullptr),
          silent(_silent),
          period(_period)
    {};

    void start();
    void terminate();

private:
    void show();

private:
    IStdInOut& log;
    mbed::DigitalOut& led;
    ThreadHandle executor;
    bool silent;
    uint32_t period;
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

void waitForEnter(IStdInOut& log);

bool agree(IStdInOut& log, mbed::DigitalOut& led);
