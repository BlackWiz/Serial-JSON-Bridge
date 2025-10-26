#include <stdint.h>

/* -------------------------------------------------------------------------- */
/* Linker Script Symbol Declarations                     */
/* -------------------------------------------------------------------------- */
/* These symbols are defined by your Linker.ld file. */
extern uint32_t _sidata;        /* Start of .data in FLASH */
extern uint32_t _etext;         /* End of .text (code) in FLASH */
extern uint32_t _sdata;         /* Start of .data in RAM */
extern uint32_t _edata;         /* End of .data in RAM */
extern uint32_t _sbss;          /* Start of .bss in RAM */
extern uint32_t _ebss;          /* End of .bss in RAM */
extern uint32_t _top_of_stack;  /* Top of RAM, defined in Linker.ld */

/* -------------------------------------------------------------------------- */
/* Forward Declarations                                  */
/* -------------------------------------------------------------------------- */
extern int main(void);
void Reset_Handler(void);
void Default_Handler(void);
extern void USART2_IRQHandler(void);

/* -------------------------------------------------------------------------- */
/* The "Ignition Sequence" (Reset Handler)                   */
/* -------------------------------------------------------------------------- */
void Reset_Handler(void) {
    uint32_t *src, *dst;

    /* 1. Copy the .data section from FLASH to RAM */
    src = &_sidata; // Start of .data in FLASH
    dst = &_sdata; // Start of .data in RAM
    while (dst < &_edata) {
        *dst++ = *src++;
    }

    /* 2. Clear the .bss section in RAM */
    dst = &_sbss;
    while (dst < &_ebss) {
        *dst++ = 0;
    }

    /* 3. Call the application's main() function */
    main();

    /* 4. If main() ever returns, loop forever */
    while (1);
}

/* -------------------------------------------------------------------------- */
/* The Vector Table (Hardware's "Phonebook")                 */
/* -------------------------------------------------------------------------- */
/* This is a C array of function pointers. 
   The __attribute__((section(".isr_vector"))) tells the compiler to place
   this array in a special section named ".isr_vector".
   
   Your `Linker.ld` file then takes this section and places it at the
   very beginning of FLASH memory (0x08000000), which is where the
   STM32 CPU is hard-wired to look for it on boot.
*/
__attribute__((section(".isr_vector")))
const uint32_t g_pfnVectors[] = {
    // Core Cortex-M0+ vectors
    (uint32_t)&_top_of_stack,    // 0. Initial Stack Pointer
    (uint32_t)&Reset_Handler,   // 1. Reset Handler
    (uint32_t)&Default_Handler, // 2. NMI Handler
    (uint32_t)&Default_Handler, // 3. HardFault Handler
    0,                          // 4. Reserved
    0,                          // 5. Reserved
    0,                          // 6. Reserved
    0,                          // 7. Reserved
    0,                          // 8. Reserved
    0,                          // 9. Reserved
    0,                          // 10. Reserved
    (uint32_t)&Default_Handler, // 11. SVCall Handler
    0,                          // 12. Reserved
    0,                          // 13. Reserved
    (uint32_t)&Default_Handler, // 14. PendSV Handler
    (uint32_t)&Default_Handler, // 15. SysTick Handler
    
    // External peripheral interrupt handlers (IRQ) for STM32G071
    // You would replace `Default_Handler` with your real handlers here
    (uint32_t)&Default_Handler, // 0. WWDG
    (uint32_t)&Default_Handler, // 1. PVD
    (uint32_t)&Default_Handler, // 2. RTC_TAMP
    (uint32_t)&Default_Handler, // 3. FLASH
    (uint32_t)&Default_Handler, // 4. RCC
    (uint32_t)&Default_Handler, // 5. EXTI0_1
    (uint32_t)&Default_Handler, // 6. EXTI2_3
    (uint32_t)&Default_Handler, // 7. EXTI4_15
    0,                          // 8. Reserved
    (uint32_t)&Default_Handler, // 9. DMA_Channel1
    (uint32_t)&Default_Handler, // 10. DMA_Channel2_3
    (uint32_t)&Default_Handler, // 11. DMA_Channel4_5_6_7
    (uint32_t)&Default_Handler, // 12. ADC_COMP
    (uint32_t)&Default_Handler, // 13. TIM1_BRK_UP_TRG_COM
    (uint32_t)&Default_Handler, // 14. TIM1_CC
    (uint32_t)&Default_Handler, // 15. TIM2
    (uint32_t)&Default_Handler, // 16. TIM3
    (uint32_t)&Default_Handler, // 17. TIM6_DAC
    (uint32_t)&Default_Handler, // 18. TIM7
    (uint32_t)&Default_Handler, // 19. TIM14
    (uint32_t)&Default_Handler, // 20. TIM15
    (uint32_t)&Default_Handler, // 21. TIM16
    (uint32_t)&Default_Handler, // 22. TIM17
    (uint32_t)&Default_Handler, // 23. I2C1
    (uint32_t)&Default_Handler, // 24. I2C2
    (uint32_t)&Default_Handler, // 25. SPI1
    (uint32_t)&Default_Handler, // 26. SPI2
    (uint32_t)&Default_Handler, // 27. USART1
    (uint32_t)&USART2_IRQHandler, // 28. USART2
    (uint32_t)&Default_Handler, // 29. USART3_4_LPUART1
    (uint32_t)&Default_Handler, // 30. CEC
    (uint32_t)&Default_Handler  // 31. AES_RNG
};

/* -------------------------------------------------------------------------- */
/* Default Interrupt Handler                           */
/* -------------------------------------------------------------------------- */
/* A simple, endless loop for all unhandled interrupts. */
void Default_Handler(void) {
    while (1);
}
