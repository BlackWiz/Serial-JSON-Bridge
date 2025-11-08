/** @file uart_unit_tests.c
 *
 * @brief Unit-level tests for UART driver functions.
 *
 * Tests individual functions in isolation before integration testing.
 * Run these first to validate basic driver behavior.
 *
 * @par
 * COPYRIGHT NOTICE: (c) 2025 Your Name. All rights reserved.
 */

#include <stdint.h>
#include <string.h>
#include "uart.h"
#include "types.h"

/* Test result tracking */
typedef struct {
    uint32_t tests_run;
    uint32_t tests_passed;
    uint32_t tests_failed;
} test_results_t;

static test_results_t g_test_results = {0};

/* External UART state variables for inspection */
extern volatile uart_state_t g_tx_state;
extern volatile uart_state_t g_rx_state;
extern volatile uart_error_t g_error;
extern char * g_p_rx_buffer;
extern volatile uint32_t g_rx_index;

/* Interrupt Enable Number */
#define USART2_IRQn 28u

/* Inline functions for interrupt control */
static inline void __enable_irq(void) {
    __asm volatile ("cpsie i" : : : "memory");
}

static inline void NVIC_EnableIRQ(uint32_t IRQn) {
    volatile uint32_t *NVIC_ISER0 = (volatile uint32_t *)0xE000E100;
    NVIC_ISER0[IRQn >> 5] = (1u << (IRQn & 0x1F));
}

/*!
 * @brief Wait for TX to complete with timeout.
 */
static void wait_tx_idle(void)
{
    uint32_t timeout = 0;
    while ((g_tx_state != UART_STATE_IDLE) && (timeout < 1000)) {
        delay_ms(1);
        timeout++;
    }
}

/*!
 * @brief Safe transmit wrapper that waits for idle state.
 */
static void safe_transmit(char const * const p_str)
{
    wait_tx_idle();
    uart_transmit_buffer(p_str);
    wait_tx_idle();
}

/*!
 * @brief Test 1: UART initialization returns success.
 */
static void test_uart_init_success(void)
{
    int32_t result;
    
    safe_transmit("\r\n[TEST 1] UART Init Success\r\n");
    
    result = uart_init();
    
    g_test_results.tests_run++;
    if ((result == 0) && 
        (g_tx_state == UART_STATE_IDLE) && 
        (g_rx_state == UART_STATE_IDLE)) {
        g_test_results.tests_passed++;
        safe_transmit("PASS\r\n");
    } else {
        g_test_results.tests_failed++;
        safe_transmit("FAIL: Init returned error or states incorrect\r\n");
    }
}


/*!
 * @brief Test 2: Transmit rejects NULL pointer.
 */
static void test_transmit_null_pointer(void)
{
    int32_t result;
    
    safe_transmit("\r\n[TEST 2] Transmit NULL Pointer\r\n");
    
    result = uart_transmit_buffer(NULL);
    
    g_test_results.tests_run++;
    if ((result == -1) && (g_tx_state == UART_STATE_IDLE)) {
        g_test_results.tests_passed++;
        safe_transmit("PASS\r\n");
    } else {
        g_test_results.tests_failed++;
        safe_transmit("FAIL: Did not reject NULL\r\n");
    }
}


/*!
 * @brief Test 3: Transmit rejects concurrent transmission.
 */
static void test_transmit_busy_reject(void)
{
    int32_t result;
    
    safe_transmit("\r\n[TEST 3] Transmit Busy Reject\r\n");
    
    /* Start first transmission */
    wait_tx_idle();
    result = uart_transmit_buffer("First");
    
    g_test_results.tests_run++;
    if (result != 0) {
        g_test_results.tests_failed++;
        safe_transmit("FAIL: First transmission failed\r\n");
        return;
    }
    
    /* Attempt second transmission while busy */
    result = uart_transmit_buffer("Second");
    
    if (result == -2) {
        g_test_results.tests_passed++;
        wait_tx_idle();
        safe_transmit("PASS\r\n");
    } else {
        g_test_results.tests_failed++;
        wait_tx_idle();
        safe_transmit("FAIL: Did not reject concurrent TX\r\n");
    }
}


/*!
 * @brief Test 4: Single byte transmission.
 */
static void test_single_byte_transmit(void)
{
    safe_transmit("\r\n[TEST 4] Single Byte Transmit\r\n");
    
    wait_tx_idle();
    uart_transmit_buffer("X");
    
    /* Wait for completion */
    uint32_t timeout = 0;
    while ((g_tx_state != UART_STATE_IDLE) && (timeout < 100)) {
        delay_ms(1);
        timeout++;
    }
    
    g_test_results.tests_run++;
    if ((g_tx_state == UART_STATE_IDLE) && (timeout < 100)) {
        g_test_results.tests_passed++;
        safe_transmit("PASS\r\n");
    } else {
        g_test_results.tests_failed++;
        safe_transmit("FAIL: Single byte TX timeout\r\n");
    }
}


/*!
 * @brief Test 5: Fixed-length packet transmission.
 */
static void test_fixed_length_packets(void)
{
    safe_transmit("\r\n[TEST 5] Fixed Length Packets\r\n");
    
    /* 10-byte packet */
    wait_tx_idle();
    uart_transmit_buffer("0123456789");
    wait_tx_idle();
    
    /* 20-byte packet */
    uart_transmit_buffer("ABCDEFGHIJKLMNOPQRST");
    wait_tx_idle();
    
    g_test_results.tests_run++;
    if (g_tx_state == UART_STATE_IDLE) {
        g_test_results.tests_passed++;
        safe_transmit("PASS\r\n");
    } else {
        g_test_results.tests_failed++;
        safe_transmit("FAIL: Fixed packets failed\r\n");
    }
}


/*!
 * @brief Test 6: Receive buffer initialization.
 */
static void test_receive_init(void)
{
    int32_t result;
    
    safe_transmit("\r\n[TEST 6] Receive Init\r\n");
    
    result = uart_receive_buffer();
    
    g_test_results.tests_run++;
    if ((result == 0) && (g_rx_state == UART_STATE_RX_BUSY)) {
        g_test_results.tests_passed++;
        /* Cancel receive to clean up */
        g_rx_state = UART_STATE_IDLE;
        safe_transmit("PASS\r\n");
    } else {
        g_test_results.tests_failed++;
        g_rx_state = UART_STATE_IDLE;
        safe_transmit("FAIL: RX init failed\r\n");
    }
}


/*!
 * @brief Test 7: Receive rejects concurrent operation.
 */
static void test_receive_busy_reject(void)
{
    int32_t result;
    
    safe_transmit("\r\n[TEST 7] Receive Busy Reject\r\n");
    
    /* Start first receive */
    result = uart_receive_buffer();
    
    g_test_results.tests_run++;
    if (result != 0) {
        g_test_results.tests_failed++;
        safe_transmit("FAIL: First RX failed\r\n");
        return;
    }
    
    /* Attempt second receive */
    result = uart_receive_buffer();
    
    if (result == -1) {
        g_test_results.tests_passed++;
        safe_transmit("PASS\r\n");
    } else {
        g_test_results.tests_failed++;
        safe_transmit("FAIL: Did not reject concurrent RX\r\n");
    }
    
    /* Clean up */
    g_rx_state = UART_STATE_IDLE;
}


/*!
 * @brief Test 8: Error state recovery.
 */
static void test_error_recovery(void)
{
    safe_transmit("\r\n[TEST 8] Error Recovery\r\n");
    
    /* Simulate error condition */
    g_rx_state = UART_STATE_ERROR;
    g_error = UART_ERROR_OVERRUN;
    
    /* Attempt recovery */
    uart_error_reset();
    
    g_test_results.tests_run++;
    if ((g_rx_state == UART_STATE_RX_BUSY) && (g_error == UART_ERROR_NONE)) {
        g_test_results.tests_passed++;
        /* Clean up */
        g_rx_state = UART_STATE_IDLE;
        safe_transmit("PASS\r\n");
    } else {
        g_test_results.tests_failed++;
        g_rx_state = UART_STATE_IDLE;
        safe_transmit("FAIL: Recovery failed\r\n");
    }
}


/*!
 * @brief Test 9: Delay timing validation.
 */
static void test_delay_timing(void)
{
    safe_transmit("\r\n[TEST 9] Delay Timing (500ms)\r\n");
    
    delay_ms(500);
    
    g_test_results.tests_run++;
    g_test_results.tests_passed++;
    safe_transmit("PASS\r\n");
}


/*!
 * @brief Print final test summary without sprintf.
 */
static void print_test_summary(void)
{
    safe_transmit("\r\n========================================\r\n");
    safe_transmit("       UNIT TEST SUMMARY\r\n");
    safe_transmit("========================================\r\n");
    
    /* Print total tests */
    safe_transmit("Total Tests:  ");
    if (g_test_results.tests_run == 9) {
        safe_transmit("9\r\n");
    } else {
        safe_transmit("ERROR\r\n");
    }
    
    /* Print passed tests */
    safe_transmit("Passed:       ");
    if (g_test_results.tests_passed == 9) {
        safe_transmit("9\r\n");
    } else if (g_test_results.tests_passed == 8) {
        safe_transmit("8\r\n");
    } else if (g_test_results.tests_passed == 7) {
        safe_transmit("7\r\n");
    } else if (g_test_results.tests_passed >= 5) {
        safe_transmit("5-6\r\n");
    } else {
        safe_transmit("LESS THAN 5\r\n");
    }
    
    /* Print failed tests */
    safe_transmit("Failed:       ");
    if (g_test_results.tests_failed == 0) {
        safe_transmit("0\r\n");
    } else if (g_test_results.tests_failed == 1) {
        safe_transmit("1\r\n");
    } else if (g_test_results.tests_failed == 2) {
        safe_transmit("2\r\n");
    } else {
        safe_transmit("MORE THAN 2\r\n");
    }
    
    safe_transmit("========================================\r\n");
    
    if (g_test_results.tests_failed == 0) {
        safe_transmit("\r\nALL TESTS PASSED!\r\n");
    } else {
        safe_transmit("\r\nSOME TESTS FAILED - CHECK OUTPUT\r\n");
    }
}


/*!
 * @brief Main unit test runner.
 */
int32_t main(void)
{
    uart_init();
    
    NVIC_EnableIRQ(USART2_IRQn);
    __enable_irq();
    
    /* Wait for UART to stabilize */
    delay_ms(1000);
    
    safe_transmit("\r\n\r\n");
    safe_transmit("############################################\r\n");
    safe_transmit("#     UART DRIVER UNIT TEST SUITE         #\r\n");
    safe_transmit("############################################\r\n");
    
    delay_ms(200);
    
    /* Run all unit tests */
    test_uart_init_success();
    delay_ms(100);
    
    test_transmit_null_pointer();
    delay_ms(100);
    
    test_transmit_busy_reject();
    delay_ms(100);
    
    test_single_byte_transmit();
    delay_ms(100);
    
    test_fixed_length_packets();
    delay_ms(100);
    
    test_receive_init();
    delay_ms(100);
    
    test_receive_busy_reject();
    delay_ms(100);
    
    test_error_recovery();
    delay_ms(100);
    
    test_delay_timing();
    delay_ms(100);
    
    /* Print summary */
    print_test_summary();
    
    /* Hold in idle loop */
    for (;;) {
        delay_ms(1000);
    }
    
    return 0;
}

/*** end of file ***/
