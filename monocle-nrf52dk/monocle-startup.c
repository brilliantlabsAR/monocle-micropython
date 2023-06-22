/*
 * This file is part of the MicroPython for Monocle project:
 *      https://github.com/brilliantlabsAR/monocle-micropython
 *
 * Authored by: Josuah Demangeon (me@josuah.net)
 *              Raj Nakarja / Brilliant Labs Ltd. (raj@itsbrilliant.co)
 *
 * ISC Licence
 *
 * Copyright © 2023 Brilliant Labs Ltd.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include "monocle.h"

extern uint32_t _stack_top;
extern uint32_t _sidata;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;

typedef void (*func)(void);
extern void main(void) __attribute__((noreturn));
extern void SystemInit(void);

void Default_Handler(void)
{
    app_err(0xDEAD0000);
}

void HardFault_Handler(void)
{
    app_err(0xDEAD0001);
}

void BusFault_Handler(void)
{
    app_err(0xDEAD0002);
}

void UsageFault_Handler(void)
{
    app_err(0xDEAD0003);
}

void Reset_Handler(void)
{
    // Initialise the RAM
    uint32_t *p_src = &_sidata;
    uint32_t *p_dest = &_sdata;

    while (p_dest < &_edata)
    {
        *p_dest++ = *p_src++;
    }

    uint32_t *p_bss = &_sbss;
    uint32_t *p_bss_end = &_ebss;
    while (p_bss < p_bss_end)
    {
        *p_bss++ = 0ul;
    }

    // Set up chip level errata fixes from Nordic
    SystemInit();

    main();
}

/**
 * @brief Specific handlers and interrupt vector table for the nRF52832
 */

void NMI_Handler(void) __attribute__((weak, alias("Default_Handler")));
void MemoryManagement_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SVC_Handler(void) __attribute__((weak, alias("Default_Handler")));
void DebugMon_Handler(void) __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler(void) __attribute__((weak, alias("Default_Handler")));
void POWER_CLOCK_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void RADIO_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void UARTE0_UART0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void SPIM0_SPIS0_TWIM0_TWIS0_SPI0_TWI0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void SPIM1_SPIS1_TWIM1_TWIS1_SPI1_TWI1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void NFCT_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void GPIOTE_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void SAADC_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TIMER0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TIMER1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TIMER2_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void RTC0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TEMP_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void RNG_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void ECB_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void CCM_AAR_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void WDT_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void RTC1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void QDEC_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void COMP_LPCOMP_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void SWI0_EGU0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void SWI1_EGU1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void SWI2_EGU2_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void SWI3_EGU3_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void SWI4_EGU4_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void SWI5_EGU5_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TIMER3_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TIMER4_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void PWM0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void PDM_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void MWU_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void PWM1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void PWM2_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void SPIM2_SPIS2_SPI2_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void RTC2_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void I2S_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void FPU_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));

const func __Vectors[] __attribute__((section(".isr_vector"), used)) = {
    (func)&_stack_top,
    Reset_Handler,
    NMI_Handler,
    HardFault_Handler,
    MemoryManagement_Handler,
    BusFault_Handler,
    UsageFault_Handler,
    0,
    0,
    0,
    0,
    SVC_Handler,
    DebugMon_Handler,
    0,
    PendSV_Handler,
    SysTick_Handler,

    /* External Interrupts */
    POWER_CLOCK_IRQHandler,
    RADIO_IRQHandler,
    UARTE0_UART0_IRQHandler,
    SPIM0_SPIS0_TWIM0_TWIS0_SPI0_TWI0_IRQHandler,
    SPIM1_SPIS1_TWIM1_TWIS1_SPI1_TWI1_IRQHandler,
    NFCT_IRQHandler,
    GPIOTE_IRQHandler,
    SAADC_IRQHandler,
    TIMER0_IRQHandler,
    TIMER1_IRQHandler,
    TIMER2_IRQHandler,
    RTC0_IRQHandler,
    TEMP_IRQHandler,
    RNG_IRQHandler,
    ECB_IRQHandler,
    CCM_AAR_IRQHandler,
    WDT_IRQHandler,
    RTC1_IRQHandler,
    QDEC_IRQHandler,
    COMP_LPCOMP_IRQHandler,
    SWI0_EGU0_IRQHandler,
    SWI1_EGU1_IRQHandler,
    SWI2_EGU2_IRQHandler,
    SWI3_EGU3_IRQHandler,
    SWI4_EGU4_IRQHandler,
    SWI5_EGU5_IRQHandler,
    TIMER3_IRQHandler,
    TIMER4_IRQHandler,
    PWM0_IRQHandler,
    PDM_IRQHandler,
    0,
    0,
    MWU_IRQHandler,
    PWM1_IRQHandler,
    PWM2_IRQHandler,
    SPIM2_SPIS2_SPI2_IRQHandler,
    RTC2_IRQHandler,
    I2S_IRQHandler,
    FPU_IRQHandler,
};
