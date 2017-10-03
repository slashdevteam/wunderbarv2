#include "loopsutil.h"

void ProgressBar::start()
{
    executor.start(mbed::callback(this, &ProgressBar::show));
}

void ProgressBar::terminate()
{
    executor.signal_set(ProgressBar::KILL_SIG);
}

void ProgressBar::show()
{
    while(true)
    {
        if(!silent)
        {
            log.printf(".");
        }
        led = !led;
        osEvent evt = rtos::Thread::signal_wait(ProgressBar::KILL_SIG, period);
        if(evt.status == osEventSignal)
        {
            break;
        }
    }
}

bool readField(IStdInOut& log,
               char* field,
               size_t minCharacters,
               size_t maxCharacters,
               const char* defaultValue,
               CharacterValidator characterValidate,
               bool doEcho,
               mbed::DigitalOut& led)
{
    log.printf("[Default: %s]> ", defaultValue);

    size_t defaultLength = std::strlen(defaultValue);
    size_t numCharacters = 0;

    bool gotNewline = false;
    bool fieldOk = true;
    while((numCharacters <= maxCharacters))
    {
        char newChar = log.getc();
        if(doEcho && !((0x8 == newChar) ||  (0x7F == newChar)))
        {
            // echo back to terminal
            log.putc(newChar);
        }
        else
        {
            log.putc('*');
        }
        // blink led on character entry
        led = !led;

        if(characterValidate(newChar))
        {
            field[numCharacters] = newChar;
            numCharacters++;
        }
        else
        {
            // '\n' or '\r' finishes
            if((0xA == newChar) ||  (0xD == newChar))
            {
                gotNewline = true;
                // ensure '\0' at the end
                // user used default value
                if(0 == numCharacters)
                {
                    // there is still space left for '\0' in buffer
                    if(defaultLength < maxCharacters)
                    {
                        field[defaultLength] = '\0';
                    }
                }
                else if((numCharacters < maxCharacters))
                {
                    field[numCharacters] = '\0';
                }
                break;
            }

            // handle backspace (0x8 or 0x7f)
            if((0x8 == newChar) ||  (0x7F == newChar))
            {
                log.putc('\b');
                log.putc(' ');
                log.putc('\b');
                if(0 != numCharacters)
                {
                    numCharacters--;
                }
                continue;
            }

            // must ignore empty read
            if(0x0 != newChar)
            {
                // invalid character received
                log.printf("\r\nInvalid character 0x%x!\r\n", newChar);
                break;
            }
        }
    }

    // check if any character was given
    if(0 == numCharacters && 0 == defaultLength)
    {
        fieldOk = false;
        log.printf("\r\nNo characters were given!\r\n");
    }
    // check if password was not too long
    else if(((numCharacters < minCharacters) && (defaultLength < minCharacters)) && gotNewline)
    {
        fieldOk = false;
        log.printf("\r\nEntry too short!\r\n");
    }
    // check if password was not too long
    else if((numCharacters == maxCharacters) && !gotNewline)
    {
        fieldOk = false;
        log.printf("\r\nEntry too long!\r\n");
    }
    // invalid character
    else if(!gotNewline)
    {
        fieldOk = false;
        log.printf("\r\nInvalid characters entered!\r\n");
    }

    return fieldOk;
}

bool isCharPrintableAscii(char c)
{
    bool valid = false;

    // 0 - 9
    if((0x30 <= c) && (0x39 >= c))
    {
        valid = true;
    }

    // A - Z
    if((0x41 <= c) && (0x5A >= c))
    {
        valid = true;
    }

    // a - z
    if((0x61 <= c) && (0x7A >= c))
    {
        valid = true;
    }

    // SP ! " # $ % & ' ( ) * + , - . /
    if((0x20 <= c) && (0x2F >= c))
    {
        valid = true;
    }

    // : ; < = > ? @
    if((0x3A <= c) && (0x40 >= c))
    {
        valid = true;
    }

    // [ \ ] ^ _ `
    if((0x5B <= c) && (0x60 >= c))
    {
        valid = true;
    }

    // { | } ~
    if((0x7B <= c) && (0x7E >= c))
    {
        valid = true;
    }

    return valid;
}

void waitForEnter(IStdInOut& log)
{
    char userInput = 0;
    do
    {
        userInput = log.getc();
    }
    while('\n' != userInput && '\r' != userInput);
    log.printf("\r\n");
}

bool validateDecision(char c)
{
    bool valid = false;

    if(('Y' == c) || ('N' == c))
    {
        valid = true;
    }

    return valid;
}

bool agree(IStdInOut& log, mbed::DigitalOut& led)
{
    bool agreed = false;
    char decision[2] = "N";
    bool decisionValid = false;
    while(!decisionValid)
    {
        decisionValid = readField(log,
                                  decision,
                                  1,
                                  1,
                                  decision,
                                  &validateDecision,
                                  true,
                                  led);
    }
    if(0 == std::strncmp(decision, "Y", 1))
    {
        agreed = true;
    }
    return agreed;
}
