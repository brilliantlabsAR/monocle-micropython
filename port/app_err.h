#pragma once
#include "nrfx_log.h"

#define app_err(err)                                                       \
    do                                                                     \
    {                                                                      \
        if (err)                                                           \
        {                                                                  \
            LOG("App error code: 0x%x at %s:%u", err, __FILE__, __LINE__); \
            __BKPT(0);                                                     \
            NVIC_SystemReset();                                            \
        }                                                                  \
    } while (0);
