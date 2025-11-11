#include <stdint.h>

/* Linker Script Symbol Declarations */
extern uint32_t _sidata;
extern uint32_t _etext;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;
extern uint32_t _top_of_stack;

/* Forward Declarations */
extern int main(void);
void Reset_Handler(void);
void Default_Handler(void);
extern void USART2_IRQHandler(void);
extern void SysTick_Handler(void);  /* NEW: For non-blocking delays */

/* Reset Handler */
void Reset_Handler(void) {
    uint32_t *src, *dst;

    /* Copy .data section from FLASH to RAM */
    src = &_sidata;
    dst = &_sdata;
    while (dst < &_edata) {
        *dst++ = *src++;
    }

    /* Clear .bss section in RAM */
    dst = &_sbss;
    while (dst < &_ebss) {
        *dst++ = 0;
    }

    /* Call main */
    main();

    /* Loop forever if main returns */
    while (1);
}

/* Vector Table */
__attribute__((section(".isr_vector")))
const uint32_t g_pfnVectors[] = {
    /* Core Cortex-M0+ vectors */
    (uint32_t)&_top_of_stack,    // 0. Initial Stack Pointer
    (uint32_t)&Reset_Handler,    // 1. Reset Handler
    (uint32_t)&Default_Handler,  // 2. NMI Handler
    (uint32_t)&Default_Handler,  // 3. HardFault Handler
    0,                           // 4. Reserved
    0,                           // 5. Reserved
    0,                           // 6. Reserved
    0,                           // 7. Reserved
    0,                           // 8. Reserved
    0,                           // 9. Reserved
    0,                           // 10. Reserved
    (uint32_t)&Default_Handler,  // 11. SVCall Handler
    0,                           // 12. Reserved
    0,                           // 13. Reserved
    (uint32_t)&Default_Handler,  // 14. PendSV Handler
    (uint32_t)&SysTick_Handler,  // 15. SysTick Handler (UPDATED)
    
    /* External peripheral interrupt handlers for STM32G071 */
    (uint32_t)&Default_Handler,  // 0. WWDG
    (uint32_t)&Default_Handler,  // 1. PVD
    (uint32_t)&Default_Handler,  // 2. RTC_TAMP
    (uint32_t)&Default_Handler,  // 3. FLASH
    (uint32_t)&Default_Handler,  // 4. RCC
    (uint32_t)&Default_Handler,  // 5. EXTI0_1
    (uint32_t)&Default_Handler,  // 6. EXTI2_3
    (uint32_t)&Default_Handler,  // 7. EXTI4_15
    0,                           // 8. Reserved
    (uint32_t)&Default_Handler,  // 9. DMA_Channel1
    (uint32_t)&Default_Handler,  // 10. DMA_Channel2_3
    (uint32_t)&Default_Handler,  // 11. DMA_Channel4_5_6_7
    (uint32_t)&Default_Handler,  // 12. ADC_COMP
    (uint32_t)&Default_Handler,  // 13. TIM1_BRK_UP_TRG_COM
    (uint32_t)&Default_Handler,  // 14. TIM1_CC
    (uint32_t)&Default_Handler,  // 15. TIM2
    (uint32_t)&Default_Handler,  // 16. TIM3
    (uint32_t)&Default_Handler,  // 17. TIM6_DAC
    (uint32_t)&Default_Handler,  // 18. TIM7
    (uint32_t)&Default_Handler,  // 19. TIM14
    (uint32_t)&Default_Handler,  // 20. TIM15
    (uint32_t)&Default_Handler,  // 21. TIM16
    (uint32_t)&Default_Handler,  // 22. TIM17
    (uint32_t)&Default_Handler,  // 23. I2C1
    (uint32_t)&Default_Handler,  // 24. I2C2
    (uint32_t)&Default_Handler,  // 25. SPI1
    (uint32_t)&Default_Handler,  // 26. SPI2
    (uint32_t)&Default_Handler,  // 27. USART1
    (uint32_t)&USART2_IRQHandler,// 28. USART2
    (uint32_t)&Default_Handler,  // 29. USART3_4_LPUART1
    (uint32_t)&Default_Handler,  // 30. CEC
    (uint32_t)&Default_Handler   // 31. AES_RNG
};

/* Default Interrupt Handler */
void Default_Handler(void) {
    while (1);
}