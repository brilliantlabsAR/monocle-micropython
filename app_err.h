#pragma once
#include "nrfx_log.h"

#define app_err(err)                                                           \
    do                                                                         \
    {                                                                          \
        if (0x0000FFFF & err)                                                  \
        {                                                                      \
            LOG("App error code: 0x%x at %s:%u\r\n", err, __FILE__, __LINE__); \
            if (CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk)              \
            {                                                                  \
                __BKPT();                                                      \
            }                                                                  \
            NVIC_SystemReset();                                                \
        }                                                                      \
    } while (0)
