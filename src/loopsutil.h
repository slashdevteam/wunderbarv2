#pragma once

#include "DigitalOut.h"
#include "Callback.h"
#include "jsmn.h"

using CharacterValidator = mbed::Callback<bool(char c)>;

void progressDots();

bool readField(char* field,
               size_t minCharacters,
               size_t maxCharacters,
               const char* defaultValue,
               CharacterValidator characterValidate,
               bool doEcho,
               mbed::DigitalOut& led);

bool isCharPrintableAscii(char c);
