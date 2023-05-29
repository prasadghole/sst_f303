/*============================================================================
* Super-Simple Tasker (SST/C) Example for STM32 NUCLEO-L053R8
*
*                    Q u a n t u m  L e a P s
*                    ------------------------
*                    Modern Embedded Software
*
* Copyright (C) 2005 Quantum Leaps, <state-machine.com>.
*
* SPDX-License-Identifier: MIT
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
============================================================================*/
#include "sst.h"
#include "bsp.h"
#include "blinky.h"
#include "gpio.h"

//#include "stm32l0xx.h"  /* CMSIS-compliant header file for the MCU used */
#include "stm32f3xx.h"
/* add other drivers if necessary... */

//DBC_MODULE_NAME("bsp_nucleo-l053r8") /* for DBC assertions in this module */

/* Local-scope defines -----------------------------------------------------*/

/* test pins on GPIO PA */
#define LED_PIN   5U /* LED LD2-Green */

/* ISRs used in the application ============================================*/
void SysTick_Handler(void);  /* prototype */

void SysTick_Handler(void) { /* system clock tick ISR */
    SST_TimeEvt_tick(); /* process all SST time events */
}

/* Assertion handler ======================================================*/
DBC_NORETURN
void DBC_fault_handler(char const * const module, int const label) {
    /*
    * NOTE: add here your application-specific error handling
    */
    (void)module;
    (void)label;

    /* set PRIMASK to disable interrupts and stop SST right here */
    __asm volatile ("cpsid i");

#ifndef NDEBUG
    for (;;) { /* keep blinking LED2 */
        BSP_ledOn();  /* turn LED2 on */
        uint32_t volatile ctr;
        for (ctr = 10000U; ctr > 0U; --ctr) {
        }
        BSP_ledOff(); /* turn LED2 off */
        for (ctr = 10000U; ctr > 0U; --ctr) {
        }
    }
#endif
    NVIC_SystemReset();
}
/*..........................................................................*/
void assert_failed(char const * const module, int const label);/* prototype */
void assert_failed(char const * const module, int const label) {
    DBC_fault_handler(module, label);
}

/* SST task activations ====================================================*/
/* preprocessor switch to choose between regular and reserved IRQs */
#define REGULAR_IRQS

#ifdef REGULAR_IRQS
/* repurpose regular IRQs for SST Tasks */
/* prototypes */
void PVD_IRQHandler(void);

void PVD_IRQHandler(void)  { SST_Task_activate(AO_Blinky);  }

#else /* use reserved IRQs for SST Tasks */
/* prototypes */
void Reserved14_IRQHandler(void);

void Reserved14_IRQHandler(void) { SST_Task_activate(AO_Blinky);  }
#endif

/* BSP functions ===========================================================*/
void BSP_init(void) {
    /* Configure the MPU to prevent NULL-pointer dereferencing
    * see: www.state-machine.com/null-pointer-protection-with-arm-cortex-m-mpu
    */
    MPU->RBAR = 0x0U                          /* base address (NULL) */
                | MPU_RBAR_VALID_Msk          /* valid region */
                | (MPU_RBAR_REGION_Msk & 7U); /* region #7 */
    MPU->RASR = (7U << MPU_RASR_SIZE_Pos)     /* 2^(7+1) region */
                | (0x0U << MPU_RASR_AP_Pos)   /* no-access region */
                | MPU_RASR_ENABLE_Msk;        /* region enable */

    MPU->CTRL = MPU_CTRL_PRIVDEFENA_Msk       /* enable background region */
                | MPU_CTRL_ENABLE_Msk;        /* enable the MPU */
    __ISB();
    __DSB();

    /* assign IRQs to tasks. NOTE: critical for SST... */
#ifdef REGULAR_IRQS
    /* repurpose regular IRQs for SST Tasks */
    SST_Task_setIRQ(AO_Blinky,  PVD_IRQn);
#else
    /* use reserved IRQs for SST Tasks */
    SST_Task_setIRQ(AO_Blinky,  14U);
#endif

    /* enable GPIO port PA clock */
//    RCC->IOPENR |= (1U << 0U);

#if ORIG_SST
    /* set all used GPIOA pins as push-pull output, no pull-up, pull-down */
    GPIOA->MODER  &= ~(3U << 2U*LED_PIN);
    GPIOA->MODER  |=  (1U << 2U*LED_PIN);
    GPIOA->OTYPER &= ~(1U <<    LED_PIN);
    GPIOA->PUPDR  &= ~(3U << 2U*LED_PIN);
#else
    MX_GPIO_Init();
#endif

}

#if ORIG_SST
/*..........................................................................*/
void BSP_ledOn(void)  { GPIOA->BSRR = (1U << LED_PIN);         } /* LED2 */
void BSP_ledOff(void) { GPIOA->BSRR = (1U << (LED_PIN + 16U)); }
#else
void BSP_ledOn(void) {
	HAL_GPIO_WritePin(LD2_GPIO_Port, GPIO_PIN_5, 1);
} /* LED2 */
void BSP_ledOff(void) {
	HAL_GPIO_WritePin(LD2_GPIO_Port, GPIO_PIN_5, 0);
}

#endif

/* SST callbacks ===========================================================*/
void SST_onStart(void) {
    SystemCoreClockUpdate();

    /* set up the SysTick timer to fire at BSP_TICKS_PER_SEC rate */
    SysTick_Config((SystemCoreClock / BSP_TICKS_PER_SEC) + 1U);

    /* set priorities of ISRs used in the system */
    NVIC_SetPriority(SysTick_IRQn, 0U);
    /* ... */
}
/*..........................................................................*/
void SST_onIdle(void) {
#ifdef NDEBUG
    /* Put the CPU and peripherals to the low-power mode.
    * you might need to customize the clock management for your application,
    * see the datasheet for your particular Cortex-M MCU.
    */
    __WFI(); /* Wait-For-Interrupt */
#endif
}

