#include "nouartfix.h"

#include "istdinout.h"
#include <cstdarg>
#include <cstdio>
#include "mbed.h"
#include "rtx_os.h"
#include "mbed_rtx.h"
#include "mbed_rtx_fault_handler.h"

IStdInOut* stdioRetarget = nullptr;


int cdcVPrintf(const char* format, va_list args)
{
    int len = 0;
    if(nullptr != stdioRetarget)
    {
        char internalBuffer[256];
        // ARMCC microlib does not properly handle a size of 0.
        // As a workaround supply a dummy buffer with a size of 1.
        char dummy_buf[1];
        len = std::vsnprintf(dummy_buf, sizeof(dummy_buf), format, args);
        if(static_cast<size_t>(len) < sizeof(internalBuffer))
        {
            std::vsprintf(internalBuffer, format, args);
            stdioRetarget->puts(internalBuffer);
        }
        else
        {
            char* temp = new char[len + 1];
            std::vsprintf(temp, format, args);
            stdioRetarget->puts(temp);
            delete[] temp;
        }
    }

    return len;
}

// avoid prints to non-existent UART forced by mbed
static uint8_t error_in_progress = 0;
extern "C" void error(const char* format, ...)
{
    // Prevent recursion if error is called again
    if(error_in_progress)
    {
        return;
    }
    error_in_progress = 1;
    std::va_list arg;
    va_start(arg, format);
    cdcVPrintf(format, arg);
    va_end(arg);
    exit(1);
}

int cdcPrintfRetarget(const char* format, ...)
{
    std::va_list arg;
    va_start(arg, format);
    int len = cdcVPrintf(format, arg);
    va_end(arg);
    return len;
}

// mbed_fault_handler from mbed_rtx_fault_handler.c
// attempts to use UART and results in infinite error recursion
// when handling faults (not connected UART triggers new fault that
// again tries to initialize UART)
// fault_print_str function is visible only in mbed_rtx_fault_handler.c
// translation unit, so it cannot be wrapped by linker. That is why
// whole mbed_fault_handler has to be wrapped
extern "C"
{
//Structure to capture the context
void _no_uart_fault_print_str(const char* fmtstr, uint32_t* values);
void hex_to_str(uint32_t value, char* hex_star);
void print_context_info(void);
void print_threads_info(osRtxThread_t* );
void print_thread(osRtxThread_t* thread);
void print_register(char* regtag, uint32_t regval);

mbed_fault_context_t mbed_fault_context;

__NO_RETURN void __wrap_mbed_fault_handler(uint32_t fault_type, void* mbed_fault_context_in, void* osRtxInfoIn)
{
    _no_uart_fault_print_str("\n++ MbedOS Fault Handler ++\n\nFaultType: ", NULL);

    switch(fault_type)
    {
      case HARD_FAULT_EXCEPTION:
        _no_uart_fault_print_str("HardFault", NULL);
        break;
      case MEMMANAGE_FAULT_EXCEPTION:
        _no_uart_fault_print_str("MemManageFault", NULL);
        break;
      case BUS_FAULT_EXCEPTION:
        _no_uart_fault_print_str("BusFault", NULL);
        break;
      case USAGE_FAULT_EXCEPTION:
        _no_uart_fault_print_str("UsageFault", NULL);
        break;
      default:
        _no_uart_fault_print_str("Unknown Fault", NULL);
        break;
    }
    _no_uart_fault_print_str("\n\nContext:", NULL);
    print_context_info();

    _no_uart_fault_print_str("\n\nThread Info:\nCurrent:", NULL);
    print_thread(((osRtxInfo_t*)osRtxInfoIn)->thread.run.curr);

    _no_uart_fault_print_str("\nNext:", NULL);
    print_thread(((osRtxInfo_t*)osRtxInfoIn)->thread.run.next);

    _no_uart_fault_print_str("\nWait Threads:", NULL);
    osRtxThread_t* threads = ((osRtxInfo_t*)osRtxInfoIn)->thread.wait_list;
    print_threads_info(threads);

    _no_uart_fault_print_str("\nDelay Threads:", NULL);
    threads = ((osRtxInfo_t*)osRtxInfoIn)->thread.delay_list;
    print_threads_info(threads);

    _no_uart_fault_print_str("\nIdle Thread:", NULL);
    threads = ((osRtxInfo_t*)osRtxInfoIn)->thread.idle;
    print_threads_info(threads);

    _no_uart_fault_print_str("\n\n-- MbedOS Fault Handler --\n\n", NULL);

    exit(1);
}

void _no_uart_fault_print_str(const char* fmtstr, uint32_t* values)
{
    int i = 0;
    int idx = 0;
    int vidx = 0;
    char hex_str[9]={0};

    while(fmtstr[i] != '\0')
    {
        if(fmtstr[i] == '\n' || fmtstr[i] == '\r')
        {
            stdioRetarget->putc(fmtstr[i]);
        }
        else
        {
            if(fmtstr[i]=='%')
            {
                hex_to_str(values[vidx++], hex_str);
                for(idx=7; idx>=0; idx--)
                {
                    stdioRetarget->putc(hex_str[idx]);
                }
            }
            else
            {
                stdioRetarget->putc(fmtstr[i]);
            }
        }
        i++;
    }
}
}
